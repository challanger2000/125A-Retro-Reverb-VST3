#pragma once
#include <array>
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "third_party/freeverb/Components/revmodel.hpp"

namespace a125 {
enum Param : Steinberg::Vst::ParamID { kEnable=100, kSize=101, kDamping=102, kWidth=103, kMix=104 };

class RetroProcessor final : public Steinberg::Vst::SingleComponentEffect {
public:
  RetroProcessor();
  static Steinberg::FUnknown* createInstance(void*) {
    return static_cast<Steinberg::Vst::IAudioProcessor*>(new RetroProcessor);
  }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs,
    Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* stream) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* stream) override;
private:
  void apply() noexcept;
  float enabled_{1.0f};
  float size_{0.5f};
  float damping_{0.5f};
  float width_{1.0f};
  float mix_{1.0f/3.0f};
  revmodel model_;
};
} // namespace a125
