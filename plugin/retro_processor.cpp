#include "retro_processor.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"

namespace a125 {
using namespace Steinberg;
using namespace Steinberg::Vst;

RetroProcessor::RetroProcessor() = default;

tresult PLUGIN_API RetroProcessor::initialize(FUnknown* context) {
  auto r=SingleComponentEffect::initialize(context);
  if(r!=kResultOk) return r;
  addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  parameters.addParameter(STR16("Enable"),nullptr,1,enabled_,ParameterInfo::kCanAutomate,kEnable);
  parameters.addParameter(STR16("Size"),nullptr,0,size_,ParameterInfo::kCanAutomate,kSize);
  parameters.addParameter(STR16("Damping"),nullptr,0,damping_,ParameterInfo::kCanAutomate,kDamping);
  parameters.addParameter(STR16("Width"),nullptr,0,width_,ParameterInfo::kCanAutomate,kWidth);
  parameters.addParameter(STR16("Mix"),nullptr,0,mix_,ParameterInfo::kCanAutomate,kMix);
  apply();
  return kResultOk;
}

void RetroProcessor::apply() noexcept {
  VocaidReferenceReverb::Controls c;
  c.enabled=(enabled_>=.5f);
  c.size=size_;
  c.damping=damping_;
  c.width=width_;
  c.mix=mix_;
  model_.setControls(c);
}

tresult PLUGIN_API RetroProcessor::setBusArrangements(
  SpeakerArrangement* in, int32 numIn,
  SpeakerArrangement* out, int32 numOut) {
  if(numIn!=1 || numOut!=1 || !in || !out) return kResultFalse;
  const int inChannels=SpeakerArr::getChannelCount(in[0]);
  const int outChannels=SpeakerArr::getChannelCount(out[0]);
  // Two-channel output from mono speech can preserve the historical stereo spread.
  if(!((inChannels==2 && outChannels==2) ||
       (inChannels==1 && outChannels==1) ||
       (inChannels==1 && outChannels==2))) return kResultFalse;
  auto* inputBus=FCast<AudioBus>(audioInputs.at(0));
  auto* outputBus=FCast<AudioBus>(audioOutputs.at(0));
  if(!inputBus || !outputBus) return kResultFalse;
  inputBus->setArrangement(in[0]);
  outputBus->setArrangement(out[0]);
  return kResultTrue;
}

tresult PLUGIN_API RetroProcessor::setupProcessing(ProcessSetup& setup) {
  if(!model_.prepare(setup.sampleRate)) return kResultFalse; // allocation off audio callback
  return SingleComponentEffect::setupProcessing(setup);
}

tresult PLUGIN_API RetroProcessor::canProcessSampleSize(int32 size) {
  return size==kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API RetroProcessor::setActive(TBool state) {
  if(state) model_.reset();
  return SingleComponentEffect::setActive(state);
}

uint32 PLUGIN_API RetroProcessor::getTailSamples() {
  // 15 seconds covers a conservative -60dB tail at maximum room size.
  return static_cast<uint32>(15.0 * model_.sampleRate());
}

tresult PLUGIN_API RetroProcessor::process(ProcessData& data) {
  if(auto* changes=data.inputParameterChanges) {
    const int32 n=changes->getParameterCount();
    for(int32 i=0;i<n;++i) {
      auto* queue=changes->getParameterData(i);
      if(!queue || queue->getPointCount()<=0)continue;
      int32 sampleOffset=0;ParamValue value=0;
      if(queue->getPoint(queue->getPointCount()-1,sampleOffset,value)!=kResultTrue)continue;
      const float v=static_cast<float>(std::clamp(value,0.0,1.0));
      switch(queue->getParameterId()){
        case kEnable: enabled_=(v>=.5f)?1.f:0.f;break;
        case kSize: size_=v;break;
        case kDamping: damping_=v;break;
        case kWidth: width_=v;break;
        case kMix: mix_=v;break;
        default:break;
      }
    }
    apply();
  }
  if(data.numInputs<1 || data.numOutputs<1 || data.numSamples<=0)return kResultOk;
  if(data.symbolicSampleSize!=kSample32 || !model_.isReady())return kResultFalse;
  const auto& input=data.inputs[0];auto& output=data.outputs[0];
  const int nIn=input.numChannels,nOut=output.numChannels;
  if(!((nIn==2 && nOut==2)||(nIn==1 && nOut==1)||(nIn==1 && nOut==2)))
    return kResultFalse;
  if(!input.channelBuffers32 || !output.channelBuffers32 ||
     !output.channelBuffers32[0] || (nOut==2 && !output.channelBuffers32[1]))return kResultFalse;

  // A silent VST3 channel is not guaranteed to contain zeroed audio memory.
  // Supply literal zeros, but still render the effect's previously stored tail.
  constexpr int kChunk=256;
  const std::array<float,kChunk> zeros{};
  output.silenceFlags=0;
  for(int offset=0;offset<data.numSamples;offset+=kChunk) {
    const auto count=static_cast<std::size_t>(std::min(kChunk,data.numSamples-offset));
    const auto inPtr=[&](int ch)->const float* {
      if((input.silenceFlags & (uint64{1}<<ch))!=0 || !input.channelBuffers32[ch])
        return zeros.data();
      return input.channelBuffers32[ch]+offset;
    };
    auto* outL=output.channelBuffers32[0]+offset;
    if(nIn==2) {
      model_.processStereo(inPtr(0),inPtr(1),outL,output.channelBuffers32[1]+offset,count);
    } else if(nOut==2) {
      model_.processMonoToStereo(inPtr(0),outL,output.channelBuffers32[1]+offset,count);
    } else {
      model_.processMono(inPtr(0),outL,count);
    }
  }
  // Set no silence flags while enabled: the internal network may be ringing.
  if(enabled_<.5f && input.silenceFlags==((uint64{1}<<nIn)-1))
    output.silenceFlags=(uint64{1}<<nOut)-1;
  return kResultOk;
}

tresult PLUGIN_API RetroProcessor::getEditorState(IBStream*) {return kResultOk;}
tresult PLUGIN_API RetroProcessor::setEditorState(IBStream*) {return kResultOk;}

tresult PLUGIN_API RetroProcessor::getState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  if(!s.writeInt32(0x125A0001))return kResultFalse;
  for(float v : {enabled_,size_,damping_,width_,mix_})
    if(!s.writeFloat(v))return kResultFalse;
  return kResultOk;
}

tresult PLUGIN_API RetroProcessor::setState(IBStream* stream) {
  if(!stream)return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  int32 version=0;
  if(!s.readInt32(version) || version!=0x125A0001)return kResultFalse;
  float values[5]{};
  for(auto& v:values)if(!s.readFloat(v)||!std::isfinite(v))return kResultFalse;
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
  model_.reset();
  return kResultOk;
}
} // namespace a125
