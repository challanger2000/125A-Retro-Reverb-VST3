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
  parameters.addParameter(STR16("Enable"),nullptr,1,enabled_,ParameterInfo::kCanAutomate,kEnable);
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
  if(active)model_.mute();
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
  if(auto* changes=data.inputParameterChanges) {
    for(int32 i=0;i<changes->getParameterCount();++i) {
      auto* queue=changes->getParameterData(i);
      if(!queue || queue->getPointCount()<=0) continue;
      int32 offset=0;ParamValue value=0;
      if(queue->getPoint(queue->getPointCount()-1,offset,value)!=kResultTrue)continue;
      const float v=static_cast<float>(std::clamp(value,0.0,1.0));
      switch(queue->getParameterId()) {
        case kEnable:enabled_=v>=.5f?1.f:0.f;break;
        case kSize:size_=v;break;
        case kDamping:damping_=v;break;
        case kWidth:width_=v;break;
        case kMix: {
          // 0% is a hard "effect off" boundary: discard accumulated wet
          // samples, never boost the dry path or resurrect a stale tail.
          if(v==0.f && mix_!=0.f) model_.mute();
          mix_=v;
          break;
        }
        default:break;
      }
    }
    apply();
  }
  if(data.numInputs<1 || data.numOutputs<1 || data.numSamples<=0)return kResultOk;
  if(data.symbolicSampleSize!=kSample32)return kResultFalse;
  const auto& in=data.inputs[0];auto& out=data.outputs[0];
  const int nIn=in.numChannels,nOut=out.numChannels;
  if(!((nIn==2 && nOut==2)||(nIn==1 && nOut==1)||(nIn==1 && nOut==2)))return kResultFalse;
  if(!in.channelBuffers32 || !out.channelBuffers32 || !out.channelBuffers32[0] ||
     (nOut==2 && !out.channelBuffers32[1]))return kResultFalse;

  constexpr int kChunk=256;
  std::array<float,kChunk> leftIn{},rightIn{},leftOut{},rightOut{};
  const bool bypass=(enabled_<.5f || mix_==0.f);
  for(int start=0;start<data.numSamples;start+=kChunk) {
    const int n=std::min(kChunk,data.numSamples-start);
    // Host silence flags don't guarantee its input memory is zero-filled.
    for(int k=0;k<n;++k) {
      const auto sample=[&](int ch)->float {
        if((in.silenceFlags&(uint64{1}<<ch)) || !in.channelBuffers32[ch])return 0.f;
        return in.channelBuffers32[ch][start+k];
      };
      leftIn[k]=sample(0);
      rightIn[k]=nIn==2?sample(1):leftIn[k];
    }

    if(bypass) {
      // Exact original PCM, no 6 dB boost and no hidden wet at Mix=0.
      for(int k=0;k<n;++k){leftOut[k]=leftIn[k];rightOut[k]=rightIn[k];}
    } else {
      // The ONLY reverberation calculations here are Jezar's original
      // revmodel::processreplace(), with its unmodified filter equations.
      model_.processreplace(leftIn.data(),rightIn.data(),
                            leftOut.data(),rightOut.data(),n,1);
    }
    for(int k=0;k<n;++k) {
      out.channelBuffers32[0][start+k]=leftOut[k];
      if(nOut==2)out.channelBuffers32[1][start+k]=rightOut[k];
    }
  }

  out.silenceFlags=0;
  for(int ch=0;ch<nOut;++ch) {
    bool isSilent=true;
    for(int k=0;k<data.numSamples;++k)
      if(out.channelBuffers32[ch][k]!=0.f){isSilent=false;break;}
    if(isSilent)out.silenceFlags|=uint64{1}<<ch;
  }
  return kResultOk;
}


tresult PLUGIN_API RetroProcessor::getState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  if(!s.writeInt32(0x125A0001))return kResultFalse;
  for(float v:{enabled_,size_,damping_,width_,mix_})
    if(!s.writeFloat(v))return kResultFalse;
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
  apply();
  model_.mute();
  return kResultOk;
}
} // namespace a125
