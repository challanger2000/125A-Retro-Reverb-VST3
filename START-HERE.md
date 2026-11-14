# START HERE — 125A Retro Reverb original Freeverb port

This branch builds a native **Windows x64 VST3** powered by Jezar's original Freeverb (\`freeverb_core\`). It requires Windows/Visual Studio 2022 Desktop C++, CMake >= 3.25 and Git.

## Build

\`\`\`bat
build_windows.bat
\`\`\`

This checks out the pinned Steinberg VST3 SDK 3.8.1, builds \`A125_Retro_Reverb\`, runs the original Freeverb tests, Mix-law tests and source-research comparisons, then CTest.

## Automated CI gates

- **Steinberg VST3 validator 47/47**
- **125A Plugin Test Lab P74/P98**, pinned to a known source SHA, for real VST3 audio/state stress
- **Original Freeverb tests:** unchanged filter-path delay timings, 101 Mix positions, unity dry at 0%, silence after return to 0%, stereo
- **Offline 44.1 kHz measurement tool:** actual Jezar Freeverb DSP only (NOT the JUCE research DSP), see \`docs/AUDIO-AB.md\`

The original source is published as Public Domain. See \`THIRD_PARTY.md\` for the documented build/reset changes. No Virtual Vocaid binary or sample banks are packaged.

## Quality gates still open

Direct comparison of the actual supplied wet/dry WAVs against Jezar Freeverb output, final graphic interface, Studio One/ASIO hands-on test. No claim of Vocaid 1:1 sound until that comparison is measured.

**Original tuning means fixed sample delays for 44.1 kHz even at other host sample rates.** This behavior is intentional. The 125A exception is limited to the user-facing Mix dry gain: 0% means unchanged dry.
