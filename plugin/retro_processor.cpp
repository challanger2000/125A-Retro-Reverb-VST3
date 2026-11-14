#include "retro_processor.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstspeaker.h"

namespace a125 {
using namespace Steinberg;
using namespace Steinberg::Vst;

RetroProcessor::RetroProcessor() = default;

tresult PLUGIN_API RetroProcessor::initialize(FUnknown* context) {
  auto r = SingleComponentEffect::initialize(context);
  if (r != kResultOk) return r;
  addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  parameters.addParameter(STR16("Enable"), nullptr, 1, enabled_, ParameterInfo::kCanAutomate, kEnable);
  parameters.addParameter(STR16("Size"), nullptr, 0, size_, ParameterInfo::kCanAutomate, kSize);
  parameters.addParameter(STR16("Damping"), nullptr, 0, damping_, ParameterInfo::kCanAutomate, kDamping);
  parameters.addParameter(STR16("Width"), nullptr, 0, width_, ParameterInfo::kCanAutomate, kWidth);
  parameters.addParameter(STR16("Mix"), nullptr, 0, mix_, ParameterInfo::kCanAutomate, kMix);
  apply();
  return kResultOk;
}

void RetroProcessor::apply() noexcept {
  model_.setroomsize(size_);
  model_.setdamp(damping_);
  model_.setwidth(width_);
  model_.setwet(mix_);
  model_.setdry(1.0f - mix_);
}

tresult PLUGIN_API RetroProcessor::setBusArrangements(SpeakerArrangement* ins, int32 nIn,
  SpeakerArrangement* outs, int32 nOut) {
  if(nIn != 1 || nOut != 1 || ins == nullptr || outs == nullptr ||
     ins[0] != SpeakerArr::kStereo || outs[0] != SpeakerArr::kStereo) return kResultFalse;
  return SingleComponentEffect::setBusArrangements(ins, nIn, outs, nOut);
}

tresult PLUGIN_API RetroProcessor::canProcessSampleSize(int32 size) {
  return size == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API RetroProcessor::setActive(TBool state) {
  if(state) model_.mute();
  return SingleComponentEffect::setActive(state);
}

tresult PLUGIN_API RetroProcessor::process(ProcessData& data) {
  if (auto* changes=data.inputParameterChanges) {
    const int32 n=changes->getParameterCount();
    for (int32 i=0;i<n;++i) {
      auto* q=changes->getParameterData(i);
      if(!q || q->getPointCount()<=0) continue;
      int32 offset=0; ParamValue value=0;
      if(q->getPoint(q->getPointCount()-1,offset,value)!=kResultTrue) continue;
      const float v=static_cast<float>(std::clamp(value,0.0,1.0));
      switch(q->getParameterId()){
        case kEnable: enabled_=v >= 0.5f ? 1.0f:0.0f; break;
        case kSize: size_=v; break;
        case kDamping: damping_=v; break;
        case kWidth: width_=v; break;
        case kMix: mix_=v; break;
        default: break;
      }
    }
    apply();
  }
  if(data.numInputs<1 || data.numOutputs<1 || data.numSamples<=0) return kResultOk;
  if(data.symbolicSampleSize!=kSample32) return kResultFalse;
  const auto& in=data.inputs[0]; auto& out=data.outputs[0];
  if(in.numChannels!=2 || out.numChannels!=2 || !in.channelBuffers32 || !out.channelBuffers32)
    return kResultFalse;
  float* il=in.channelBuffers32[0]; float* ir=in.channelBuffers32[1];
  float* ol=out.channelBuffers32[0]; float* orr=out.channelBuffers32[1];
  if(!il || !ir || !ol || !orr) return kResultFalse;
  if(enabled_<0.5f){
    if(il!=ol) std::memcpy(ol,il,static_cast<std::size_t>(data.numSamples)*sizeof(float));
    if(ir!=orr) std::memcpy(orr,ir,static_cast<std::size_t>(data.numSamples)*sizeof(float));
    out.silenceFlags=in.silenceFlags;
    return kResultOk;
  }
  // Process silence too: Freeverb may have a reverb tail even when input is silent.
  out.silenceFlags=0;
  model_.processreplace(il,ir,ol,orr,data.numSamples,1);
  return kResultOk;
}

tresult PLUGIN_API RetroProcessor::getState(IBStream* stream) {
  if(!stream) return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  if(!s.writeInt32(0x125A0001)) return kResultFalse;
  for(float v : {enabled_,size_,damping_,width_,mix_})
    if(!s.writeFloat(v)) return kResultFalse;
  return kResultOk;
}

tresult PLUGIN_API RetroProcessor::setState(IBStream* stream) {
  if(!stream) return kInvalidArgument;
  IBStreamer s(stream,kLittleEndian);
  int32 version=0;
  if(!s.readInt32(version) || version!=0x125A0001) return kResultFalse;
  float values[5]{};
  for(auto& v:values) if(!s.readFloat(v) || !std::isfinite(v)) return kResultFalse;
  enabled_=values[0]>=0.5f?1.0f:0.0f;
  size_=std::clamp(values[1],0.0f,1.0f);
  damping_=std::clamp(values[2],0.0f,1.0f);
  width_=std::clamp(values[3],0.0f,1.0f);
  mix_=std::clamp(values[4],0.0f,1.0f);
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
