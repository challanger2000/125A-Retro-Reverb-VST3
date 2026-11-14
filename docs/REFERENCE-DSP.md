# Vocaid reference DSP: controlled reconstruction, NOT an acoustic sign-off

Source: independently implemented reference core in \`src/reference_reverb.hpp\`.
Analysis basis:
- User-supplied **Virtual Vocaid v1.0** contains the classic Freeverb filter tunings and JUCE 6.1.6 code.
- Original Vocaid static disassembly near \`0x180299D1B\`: Size, Damping, Mix, Width transferred to juce::Reverb; Wet=Mix, Dry=1−Mix.
- Original Vocaid code at \`0x180059D01\`: sample-rate-scaled delay allocation, using integer \`(int(sampleRate)*tuning)/44100\`.
- Reference from JUCE 6.1.6, \`juce_Reverb.h\`, published under the **ISC license**: 8 combs + 4 allpasses per channel; +23 right-channel offset; smooth-time 10 ms using \`floor(rate*0.01)\`.
- The original **Jezar** C++ Freeverb has **fixed delay lengths** and **immediate parameter updates**; it remains in \`third_party/freeverb\` as the historically important comparison baseline.

Test scope: 44.1/48/96 kHz plus Steinberg Validator exotic sample rates (1234.568 and 1234567.8 Hz), impulse onset, Wet/Dry and Enable law, stereo character, fragmentation, 1200 allocation-free callbacks, no IR.
We have NOT rendered the original proprietary instrument under a Windows VST3 host. A claim of audible or PCM 1:1 equivalence is therefore **not permitted**.
A true audio A/B additionally needs the original instrument's sample and MIDI playback at fixed preset settings, dry and wet captures, and subtraction or convolution-system identification of the reverb component.
