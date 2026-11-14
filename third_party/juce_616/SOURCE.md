# Original JUCE 6.1.6 cross-check – test only

`juce_Reverb.h` is copied **unchanged** from the JUCE 6.1.6 tag:
https://github.com/juce-framework/JUCE/blob/6.1.6/modules/juce_audio_basics/utilities/juce_Reverb.h

The file explicitly permits redistribution under the **ISC license** with preservation of its copyright and permission notice. The test-only `juce_reverb_shim.hpp` supplies minimal stand-ins for JUCE's buffer and parameter smoothing helpers, so no full JUCE runtime or paid license is required for the production plugin.

`tests/juce_616_differential_tests.cpp` processes the same deterministic mono and stereo audio through the original published JUCE reverb source and `src/reference_reverb.hpp` at 44.1, 48, 96, 22.05 and 12.345678 kHz, and compares samples without normalization. This is a **source-level DSP conformance test**, not recorded audio from the Virtual Vocaid instrument. Vocaid additional processing and parameter mapping are still separate verification questions.

The new code is included only in the CTest target. It does not enter the shipping Windows VST3.
