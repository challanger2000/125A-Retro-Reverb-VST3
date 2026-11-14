#include "retro_processor.hpp"
#include "public.sdk/source/main/pluginfactory.h"

namespace a125 {
static const Steinberg::FUID kRetroProcessorCID(0x15A0D241,0xFE4241DD,0xA0B5C427,0xE8C00125);
}
using namespace Steinberg;
BEGIN_FACTORY_DEF("125A", "https://github.com/challanger2000/125A-Retro-Reverb-VST3", "")
  DEF_CLASS2(INLINE_UID_FROM_FUID(a125::kRetroProcessorCID),
    PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    "125A Retro Reverb",
    0,
    "Fx|Reverb",
    "0.3.2",
    kVstVersionString,
    a125::RetroProcessor::createInstance)
END_FACTORY
