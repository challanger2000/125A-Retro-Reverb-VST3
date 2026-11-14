#pragma once
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "revmodel.hpp"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/uidescription/uiattributes.h"

namespace a125 {
enum Param : Steinberg::Vst::ParamID {
  kEnable=100, kSize=101, kDamping=102, kWidth=103, kMix=104
};

class RetroProcessor final : public Steinberg::Vst::SingleComponentEffect,
                             public VSTGUI::VST3EditorDelegate {
public:
  RetroProcessor();
  static Steinberg::FUnknown* createInstance(void*) {
    return static_cast<Steinberg::Vst::IAudioProcessor*>(new RetroProcessor);
  }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
  Steinberg::tresult PLUGIN_API setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
    Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
  Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
  Steinberg::uint32 PLUGIN_API getTailSamples() override;
  Steinberg::tresult PLUGIN_API getEditorState(Steinberg::IBStream*) override;
  Steinberg::IPlugView* PLUGIN_API createView(const char* name) override;
  void didOpen(VSTGUI::VST3Editor* editor) override;
  void willClose(VSTGUI::VST3Editor* editor) override;
  VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr name,
      const VSTGUI::UIAttributes& attr, const VSTGUI::IUIDescription* desc,
      VSTGUI::VST3Editor* editor) override;
  Steinberg::tresult PLUGIN_API setEditorState(Steinberg::IBStream*) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
private:
  void apply() noexcept;
  float enabled_=1.f;
  float size_=.5f;
  float damping_=.5f;
  float width_=1.f;
  float mix_=1.f/3.f;  // Jezar's initial wet (original had separate dry=0)
  double sampleRate_=44100.0; // VST host accounting only, NOT a DSP retuning
  revmodel model_;
  int guiZoomIndex_=0;
  VSTGUI::VST3Editor* openEditor_=nullptr;
};
} // namespace a125
