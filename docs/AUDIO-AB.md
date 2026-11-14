# Offline audio comparison — actual Virtual Vocaid recordings

The user supplied **One Reverb OFF.wav**, **One Reverb 100.wav**, and **One(1).wav**. The data is stereo, 44.1 kHz, 32-bit float. The 100% wet recording contains a long tail; the dry and default recordings are shorter. These captures are not committed to GitHub.

The supplied default rendering satisfies **default ≈ OFF + 0.5 × wet100**, with residuals at floating-point rounding scale. Thus **the original Vocaid captures do not support the earlier +6 dB dry hypothesis**. The production 125A Mix rule also requires unity dry at 0%, but that is a separate specification.

## Tool

\`tools/vocaid_ab_renderer.cpp\` links directly to original Jezar \`freeverb_core\` (not the JUCE research engine). Its interface is

\`\`\`bat
build-win\Release\vocaid_ab_renderer.exe 44100 0.5 0.5 1 1 input.f32 output.f32
\`\`\`

\`tools/compare_vocaid.py\` accepts matched 44.1kHz 16/24/32-bit integer PCM WAV files:

\`\`\`bat
python tools\compare_vocaid.py --renderer build-win\Release\vocaid_ab_renderer.exe --dry "One Reverb OFF.wav" --wet "One Reverb 100.wav" --size 0.5 --damping 0.5 --width 1 --out-wav freeverb-candidate.wav
\`\`\`

**Do not infer an exact numerical preset parameter from a knob image**. Use the Vocaid controls' actual settings for Size/Damping/Width if available; the values above are only example arguments. The comparator does not normalize gain or shift timing. Compare the common overlap (wet100 can be longer than dry) and render extra silence to measure the original tail beyond the dry clip.

At 48 and 96 kHz, the original 2000 Freeverb uses unchanged delay counts; the A/B tool intentionally limits this measurement to 44.1 kHz to avoid accidental comparisons to differently sampled signals.

## Acceptance rule

Do **not** claim a 1:1 Virtual Vocaid clone until the audio error has been measured and explained. Matching an algorithm family or the 1116/1139-sample onset alone is insufficient. The only non-original behavior in the production VST3 is the required unity dry Mix law and host adapter.
