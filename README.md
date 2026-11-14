# 125A Retro Reverb — original Jezar Freeverb VST3 port

**Windows x64 VST3 · C++23 · Steinberg VST3 SDK 3.8.1 · No JUCE runtime**

This plugin deliberately uses the classic **Freeverb by Jezar at Dreampoint (2000)**. The active stereo DSP is the original public-domain \`revmodel\` from \`third_party/freeverb/Components\`, not a new reverb implementation. There are **no additional filters, predelay, modulation, EQ, saturation, or invented sound-character adjustments**.

**The sole intentional control-law exception:** The user-facing **Mix** knob must follow 125A's rule: **0% = original dry signal exactly at unity gain, no wet; 100% = fully wet, no dry.** Because original Freeverb internally applies a dry gain of 2, the VST3 wrapper calls \`setdry((1-Mix)/2)\` and \`setwet(Mix)\`. The original filter formulas remain unchanged. Switching Mix to zero discards an old wet tail rather than allowing it to reappear when Mix is increased again.

\`Enable\` is a true unity-gain bypass. \`Size\`, \`Damping\`, and \`Width\` call the original setters directly.

## Supported audio layouts

Stereo → Stereo (the original Freeverb stereo algorithm), Mono → Stereo (duplicate input into original stereo network), Mono → Mono (original left reverberation channel). The original Freeverb mixes L/R for the wet excitation and uses different filter delays for left/right output. Dry stereo channels retain their original separation.

## Deliberately original behavior

- **Original Freeverb fixed sample-delay lengths** assume 44.1 kHz. This port does **not** alter the delays at 48 or 96 kHz; higher-rate decay time will therefore be shorter in seconds. This is a conscious 1:1-source decision, not a bug fix or alternative reverb model.
- Freeverb's original Wet scale (3), Damping scale (0.4), Room size scale (0.28) and offset (0.7) remain intact.
- Minimal compiler compatibility and defined reset/constructor-state corrections are documented in [THIRD_PARTY.md](THIRD_PARTY.md); the musical filter-processing equations are unchanged.
- No custom GUI yet: host generic parameters are available.

## Build and tests

Read [START-HERE.md](START-HERE.md). \`build_windows.bat\` builds the Windows VST3, runs CTest, the Steinberg Validator, and independent 125A Plugin Test Lab P74/P98 in GitHub Actions. The VST3 links only the **\`freeverb_core\`** DSP library; JUCE-like code under \`src/reference_reverb.hpp\` and tests under \`third_party/juce_616\` are archived comparative research and not linked into the plugin.

The offline comparison command in [docs/AUDIO-AB.md](docs/AUDIO-AB.md) tests actual supplied Virtual Vocaid WAVs against the unchanged Jezar Freeverb algorithm at 44.1 kHz, without normalizing away any level mismatch. Klanggleichheit mit Virtual Vocaid ist bis zum erfolgreichen A/B-Vergleich nicht behauptet.

**Current state:** candidate VST3, not a final commercial release; studio/ASIO practical test and GUI remain open.

## P004 — GUI-Wiederverwendung aus Ugly Reverb

Die VSTGUI-Oberfläche verwendet **dieselben bestehenden Klassen** `UglyKnob`, `UglyToggle`, `UglyLogo`, `UglyZoomControl` und denselben 101-Frame-Regler-Spritesheet samt HiDPI-Buildskript aus [125A-Ugly-Reverb-Final](https://github.com/challanger2000/125A-Ugly-Reverb-Final), gepinnt auf Commit `631c79b5fddfb19b916d927f0918134576a3a031`. Es werden **keine eigenen Regler** neu entwickelt. Das reduzierte Layout ist 650 × 298 px groß, mit Size, Damping, Width, Mix, Enable und den vorhandenen numerischen Anzeigen. Nur das kompakte Frontplattenlayout ist neu. Der Freeverb-DSP bleibt unverändert; Ugly Reverb DSP wird nicht eingebunden.
