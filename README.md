# 125A Retro Reverb — original Freeverb VST3

**Windows x64 VST3 · P004.5 / v0.3.5 technical candidate · C++23 · Steinberg SDK 3.8.1 · no JUCE runtime**

The effect uses **Jezar at Dreampoint's original Freeverb** comb/allpass algorithms, unchanged under `third_party/freeverb/Components`. The VST3 wrapper handles audio buffers, parameter automation, project recall, dry/wet mixing and numerical input safety. There is no additional EQ, modulation, predelay or original 125A reverb algorithm.

## Controls

- **Enable:** Off gives unity dry output; the existing reverb tank advances on silence so its tail does not freeze and reappear later.
- **Size, Damping, Width:** original Freeverb setters.
- **Mix:** 0% is **exact dry at unity**, with the old wet tail cleared; 100% is fully wet. The intermediate dry gain compensates for Freeverb's internal ×2 dry multiplier.
- Five VST3 parameter IDs **100–104** and the existing 24-byte component state format remain stable. All host automation points are consumed at their sample offsets.

Stereo → stereo, mono → stereo and mono → mono are supported in **32-bit float**. Non-finite samples are prevented from entering the recursive DSP. For finite inputs, the dry/bypass output remains unity and unaltered.

**Original Freeverb limitation:** delay lengths are fixed for 44.1 kHz. As in the original, higher sample rates shorten delays/decays in seconds. No retuning of the original Freeverb filter equations.

## GUI

Fixed **930 × 410 px** VSTGUI editor, **four 125 px UglyKnob controls** from [125A Ugly Reverb Final](https://github.com/challanger2000/125A-Ugly-Reverb-Final), pinned at `631c79b5fddfb19b916d927f0918134576a3a031`. The Enable switch uses a transparent two-state PNG and VSTGUI `COnOffButton`. **No zoom UI.** Primary Size/Damping/Width/Mix/Enable labels use explicit **20-unit bold** VSTGUI fonts; numeric values use **18-unit bold** fonts. These avoid reliance on the small host default at fixed 100% size; control/knob geometry remains unchanged. The other plugin's DSP is not linked.

## Building / verification

Run `build_windows.bat` with Visual Studio 2022 C++, CMake ≥3.25, Git and Python 3 on Windows. The build verifies the Steinberg SDK at exact commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` (`v3.8.1_build_84`), builds `125A_Retro_Reverb.vst3` and runs five CTests, including the native production-processor audio/state regression.

The Windows Actions workflow also runs the official Steinberg Validator, independent **125A Plugin Test Lab P74/P98**, and the exact **125A Plugin Tester v0.3.2** source revision used in the owner's original warning report. It checks the actual packaged VST3 class/bundle versions.

**Verified November 14, 2026, source `0509aa8`:** 5/5 CTests, 47/47 Steinberg Validator, P74/P98 `TECHNICAL_BASELINE_PASS`, and 125A Plugin Tester **45 PASS / 0 WARNING / 0 FAIL**. See [P004.5 technical QA](docs/P0045-TECHNICAL-QA.md) and [repair PR #2](https://github.com/challanger2000/125A-Retro-Reverb-VST3/pull/2). The automated tests establish only their named contracts.

## Product acceptance

**Not a final commercial release.** Direct Fender Studio Pro / physical ASIO, real GUI interaction and representative musical level-matched A/B are **NOT RUN / NOT VERIFIED**. P74 correctly reports `release_approved=false`. The [Virtual Vocaid A/B pipeline](docs/AUDIO-AB.md) self-test is not a claim of Virtual Vocaid sonic equivalence.

For original source and licensing notes see [THIRD_PARTY.md](THIRD_PARTY.md).
