#include "retro_processor.hpp"
#include "src/freeverb_mix.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"

namespace a125 {
using namespace Steinberg;
using namespace Steinberg::Vst;

RetroProcessor::RetroProcessor() = default;

tresult PLUGIN_API RetroProcessor::initialize(FUnknown* context) {
  const auto result=SingleComponentEffect::initialize(context);
  if(result!=kResultOk) return result;
  addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);
  // The generic numeric toggle cannot parse its displayed "On" back to 1.
  // Keep toggle metadata (stepCount=1, kCanAutomate) with an exact text oracle.
  auto* enable=new StringListParameter(STR16("Enable"),kEnable,nullptr,
                                       ParameterInfo::kCanAutomate);
  enable->appendString(STR16("Off"));
  enable->appendString(STR16("On"));
  enable->getInfo().defaultNormalizedValue=enabled_;
  enable->setNormalized(enabled_);
  parameters.addParameter(enable);
  // Reuse Ugly Reverb's CTextEdit-based percentage displays.
  auto addPercent=[&](const TChar* label,ParamID id,float initial) {
    auto* p=new RangeParameter(label,id,STR16("%"),0.0,100.0,initial*100.0);
    p->setPrecision(0);
    parameters.addParameter(p);
  };
  addPercent(STR16("Size"),kSize,size_);
  addPercent(STR16("Damping"),kDamping,damping_);
  addPercent(STR16("Width"),kWidth,width_);
  addPercent(STR16("Mix"),kMix,mix_);
  apply();
  return kResultOk;
}

void RetroProcessor::apply() noexcept {
  model_.setroomsize(size_);
  model_.setdamp(damping_);
  model_.setwidth(width_);
  applyMix(model_,mix_);
}

tresult PLUGIN_API RetroProcessor::setBusArrangements(
  SpeakerArrangement* in,int32 numIn,SpeakerArrangement* out,int32 numOut) {
  if(numIn!=1 || numOut!=1 || !in || !out) return kResultFalse;
  const int inputChannels=SpeakerArr::getChannelCount(in[0]);
  const int outputChannels=SpeakerArr::getChannelCount(out[0]);
  if(!((inputChannels==2 && outputChannels==2) ||
       (inputChannels==1 && outputChannels==1) ||
       (inputChannels==1 && outputChannels==2))) return kResultFalse;
  if(audioInputs.empty() || audioOutputs.empty())return kResultFalse;
  auto* inBus=FCast<AudioBus>(audioInputs.at(0));
  auto* outBus=FCast<AudioBus>(audioOutputs.at(0));
  if(!inBus || !outBus)return kResultFalse;
  inBus->setArrangement(in[0]);
  outBus->setArrangement(out[0]);
  return kResultTrue;
}

tresult PLUGIN_API RetroProcessor::setupProcessing(ProcessSetup& setup) {
  if(!std::isfinite(setup.sampleRate) || setup.sampleRate<=0.0)
    return kResultFalse;
  sampleRate_=setup.sampleRate;
  // Do NOT scale original Jezar Freeverb delay lengths by sample rate.
  return SingleComponentEffect::setupProcessing(setup);
}

tresult PLUGIN_API RetroProcessor::canProcessSampleSize(int32 size) {
  return size==kSample32?kResultTrue:kResultFalse;
}

tresult PLUGIN_API RetroProcessor::setActive(TBool active) {
  if(active) {
    apply(); // Consume state configured before host activation.
    model_.mute();
    pendingStateRestore_.store(false,std::memory_order_release);
  }
  return SingleComponentEffect::setActive(active);
}

tresult PLUGIN_API RetroProcessor::setProcessing(TBool) {
  // Required by VST3 host contract. No audio-thread allocation.
  return kResultOk;
}

uint32 PLUGIN_API RetroProcessor::getTailSamples() {
  const double frames=std::clamp(sampleRate_*15.0,0.0,static_cast<double>(std::numeric_limits<uint32>::max()));
  return static_cast<uint32>(frames);
}

tresult PLUGIN_API RetroProcessor::process(ProcessData& data) {
  if(pendingStateRestore_.exchange(false,std::memory_order_acq_rel)) {
    // Host recall may arrive from a different thread. Only the process thread
    // is allowed to mutate the recursive Freeverb DSP while active.
    apply();
    model_.mute();
  }
  // A parameter queue is ordered in sample offsets. Consume EVERY point,
  // applying it at its exact frame, rather than grabbing the last point and
  // applying that value to the entire block.
  struct Cursor {
    IParamValueQueue* queue=nullptr;
    int32 index=0;
  };
  std::array<Cursor,5> cursors{};
  if(auto* changes=data.inputParameterChanges) {
    for(int32 i=0;i<changes->getParameterCount();++i) {
      auto* queue=changes->getParameterData(i);
      if(!queue)continue;
      const ParamID id=queue->getParameterId();
      if(id<kEnable || id>kMix)continue;
      auto& cursor=cursors[id-kEnable];
      if(!cursor.queue)cursor.queue=queue;
    }
  }

  auto applyPoint=[&](ParamID id,ParamValue value) noexcept {
    if(!std::isfinite(value))return false;
    const float v=static_cast<float>(std::clamp(value,0.0,1.0));
    switch(id) {
      case kEnable: enabled_=v>=.5f?1.f:0.f;return true;
      case kSize: size_=v;return true;
      case kDamping: damping_=v;return true;
      case kWidth: width_=v;return true;
      case kMix:
        // Deliberate 125A 0%-Mix rule: discard the wet tail.
        if(v==0.f && mix_!=0.f)model_.mute();
        mix_=v;
        return true;
      default:return false;
    }
  };

  // Steinberg permits parameter flushes without audio buffers or samples.
  if(data.numSamples<=0 || data.numInputs<1 || data.numOutputs<1) {
    bool changed=false;
    for(ParamID id=kEnable;id<=kMix;++id) {
      auto* q=cursors[id-kEnable].queue;
      if(!q)continue;
      for(int32 j=0;j<q->getPointCount();++j) {
        int32 offset=0;ParamValue value=0;
        if(q->getPoint(j,offset,value)==kResultTrue && offset>=0)
          changed|=applyPoint(id,value);
      }
    }
    if(changed)apply();
    return kResultOk;
  }
  if(data.symbolicSampleSize!=kSample32)return kResultFalse;
  if(!data.inputs || !data.outputs)return kResultFalse;
  const auto& in=data.inputs[0];
  auto& out=data.outputs[0];
  const int nIn=in.numChannels,nOut=out.numChannels;
  if(!((nIn==2 && nOut==2)||(nIn==1 && nOut==1)||(nIn==1 && nOut==2)))
    return kResultFalse;
  if(!in.channelBuffers32 || !out.channelBuffers32 || !out.channelBuffers32[0] ||
     (nOut==2 && !out.channelBuffers32[1]))return kResultFalse;

  constexpr int kChunk=256;
  std::array<float,kChunk> leftIn{},rightIn{},leftOut{},rightOut{},zeros{};

  const auto render=[&](int first,int count) {
    for(int start=first;start<first+count;start+=kChunk) {
      const int n=std::min(kChunk,first+count-start);
      for(int k=0;k<n;++k) {
        const auto sample=[&](int ch)->float {
          if((in.silenceFlags&(uint64{1}<<ch)) || !in.channelBuffers32[ch])
            return 0.f;
          const float x=in.channelBuffers32[ch][start+k];
          // Invalid input must never poison recursive DSP. Retain every
          // finite sample unmodified on the exact-unity dry bypass paths.
          return std::isfinite(x)?x:0.f;
        };
        leftIn[k]=sample(0);
        rightIn[k]=nIn==2?sample(1):leftIn[k];
      }
      if(mix_==0.f) {
        // The tank was muted at the 0%-Mix boundary: pure unity dry.
        for(int k=0;k<n;++k) {
          leftOut[k]=leftIn[k];rightOut[k]=rightIn[k];
        }
      } else if(enabled_<.5f) {
        // Bypass outputs dry audio but advances an existing tail on silence.
        // No new audio enters the tank; no frozen old tail can reappear.
        model_.processreplace(zeros.data(),zeros.data(),
                              leftOut.data(),rightOut.data(),n,1);
        for(int k=0;k<n;++k) {
          leftOut[k]=leftIn[k];rightOut[k]=rightIn[k];
        }
      } else {
        // Unchanged original Jezar Freeverb filter processing. Bound only
        // pathological wet excitation; keep the dry contribution at full
        // finite signal level in the output.
        std::array<float,kChunk> wetL{},wetR{};
        for(int k=0;k<n;++k) {
          wetL[k]=std::clamp(leftIn[k],-16.f,16.f);
          wetR[k]=std::clamp(rightIn[k],-16.f,16.f);
        }
        model_.processreplace(wetL.data(),wetR.data(),
                              leftOut.data(),rightOut.data(),n,1);
        const float dry=1.f-mix_.load(std::memory_order_relaxed);
        if(dry!=0.f)for(int k=0;k<n;++k) {
          leftOut[k]+=dry*(leftIn[k]-wetL[k]);
          rightOut[k]+=dry*(rightIn[k]-wetR[k]);
        }
      }
      for(int k=0;k<n;++k) {
        out.channelBuffers32[0][start+k]=leftOut[k];
        if(nOut==2)out.channelBuffers32[1][start+k]=rightOut[k];
      }
    }
  };

  int pos=0;
  while(pos<data.numSamples) {
    int next=data.numSamples;
    // Skip malformed/obsolete points and locate the next boundary.
    for(auto& c:cursors) {
      if(!c.queue)continue;
      while(c.index<c.queue->getPointCount()) {
        int32 offset=0;ParamValue value=0;
        if(c.queue->getPoint(c.index,offset,value)!=kResultTrue ||
           offset<pos || offset>=data.numSamples || !std::isfinite(value)) {
          ++c.index;
          continue;
        }
        next=std::min(next,static_cast<int>(offset));
        break;
      }
    }
    if(next>pos)render(pos,next-pos);
    pos=next;
    if(pos==data.numSamples)break;
    bool changed=false;
    for(ParamID id=kEnable;id<=kMix;++id) {
      auto& c=cursors[id-kEnable];
      if(!c.queue)continue;
      while(c.index<c.queue->getPointCount()) {
        int32 offset=0;ParamValue value=0;
        if(c.queue->getPoint(c.index,offset,value)!=kResultTrue ||
           offset<pos || offset>=data.numSamples || !std::isfinite(value)) {
          ++c.index;
          continue;
        }
        if(offset!=pos)break;
        changed|=applyPoint(id,value);
        ++c.index;
      }
    }
    if(changed)apply();
  }

  out.silenceFlags=0;
  for(int ch=0;ch<nOut;++ch) {
    bool silent=true;
    for(int k=0;k<data.numSamples;++k) {
      if(out.channelBuffers32[ch][k]!=0.f) {silent=false;break;}
    }
    if(silent)out.silenceFlags|=uint64{1}<<ch;
  }
  return kResultOk;
}

tresult PLUGIN_API RetroProcessor::getState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  if(!s.writeInt32(0x125A0001))return kResultFalse;
  for(float v:{enabled_.load(),size_.load(),damping_.load(),
               width_.load(),mix_.load()})
    if(!s.writeFloat(v))return kResultFalse;
  return kResultOk;
}


tresult PLUGIN_API RetroProcessor::setComponentState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  // Separate controller synchronization during a host preset/project recall.
  // Do not touch DSP feedback state here: the host invokes setState for that.
  IBStreamer s(stream,kLittleEndian);
  int32 version=0;
  if(!s.readInt32(version) || version!=0x125A0001)return kResultFalse;
  float values[5]{};
  for(float& v:values)
    if(!s.readFloat(v)||!std::isfinite(v))return kResultFalse;
  setParamNormalized(kEnable,values[0]>=.5f?1.:0.);
  setParamNormalized(kSize,std::clamp(static_cast<double>(values[1]),0.,1.));
  setParamNormalized(kDamping,std::clamp(static_cast<double>(values[2]),0.,1.));
  setParamNormalized(kWidth,std::clamp(static_cast<double>(values[3]),0.,1.));
  setParamNormalized(kMix,std::clamp(static_cast<double>(values[4]),0.,1.));
  return kResultOk;
}

tresult PLUGIN_API RetroProcessor::setState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  int32 version=0;
  if(!s.readInt32(version)||version!=0x125A0001)return kResultFalse;
  float values[5]{};
  for(float& v:values)if(!s.readFloat(v)||!std::isfinite(v))return kResultFalse;
  enabled_=values[0]>=.5f?1.f:0.f;
  size_=std::clamp(values[1],0.f,1.f);
  damping_=std::clamp(values[2],0.f,1.f);
  width_=std::clamp(values[3],0.f,1.f);
  mix_=std::clamp(values[4],0.f,1.f);
  setParamNormalized(kEnable,enabled_);
  setParamNormalized(kSize,size_);
  setParamNormalized(kDamping,damping_);
  setParamNormalized(kWidth,width_);
  setParamNormalized(kMix,mix_);
  // Updating Freeverb internals here would race with process().
  // Defer the one-time reset/coefficient change to the next audio callback.
  pendingStateRestore_.store(true,std::memory_order_release);
  return kResultOk;
}
} // namespace a125
