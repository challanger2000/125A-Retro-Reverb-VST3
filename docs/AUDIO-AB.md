# A/B measurement procedure – real Virtual Vocaid vs 125A

\`tools/compare_vocaid.py\` and \`vocaid_ab_renderer.exe\` provide a reproducible WAV-to-WAV acoustic comparison. They operate **offline**, on **local** recordings. No Virtual Vocaid DLL or sample bank is uploaded to GitHub.

## Capture requirement (critical)

Virtual Vocaid is an instrument, not an insertable audio reverb. Feed it the **same MIDI sequence/preset** twice, record at 44,100 Hz/24-bit WAV in a DAW/test host, and save:
- \`dry.wav\`: Reverb enabled, Mix **0%**, all other reverb controls identical.
- \`wet.wav\`: Reverb enabled, Mix **100%**, same MIDI and same render length, other effects disabled.

Choose a note with clear attack and a 5+ second tail; export with identical pre-roll and starting sample. Render the whole file twice, including 1 second before the first note to allow parameters to settle. Exact equality of source playback across renders must be checked; random modulation and instrument envelopes can limit achievable match.

## Compare

From a Windows VS2022 build of this repository:

\`\`\`bat
python tools\compare_vocaid.py --renderer build-win\Release\vocaid_ab_renderer.exe --dry dry.wav --wet wet.wav --size 0.5 --damping 0.5 --width 1 --out-wav candidate.wav
\`\`\`

Output includes correlation, absolute RMS delta, *non-normalized* error RMS, relative error and level difference in dB. The candidate WAV can be auditioned against wet.wav. **No automatic gain compensation, reverb normalization or timing alignment** is performed.

The inferred JUCE 6.1.6/Vocaid parameter routing uses \`wet=Mix\` and \`dry=1−Mix\`, plus internal \`wetScale=3\`, \`dryScale=2\`. The comparator treats the captured dry=0% audio divided by 2 as the dry source signal. If the instrument has additional gain/mixer processing, this hypothesis may be wrong; the measured discrepancy then tells us what needs tracing.

To check that the measurement pipeline works before recording anything:
\`\`\`bat
python tools\compare_vocaid.py --renderer build-win\Release\vocaid_ab_renderer.exe --selftest
\`\`\`

### Current evidence

Confirmed **by static reverse engineering**: filter table, integer sample-rate scaling, parameters Size/Damping/Width/Wet/Dry and JUCE 6.1.6-style implementation. Confirmed by our own CI: generated DSP test vector and WAV compare pipeline. **Not yet confirmed:** actual input/output WAV audio from Virtual Vocaid. Until such a recording exists, a claimed PCM/audible 1:1 match would be misleading.

## Important separation: original Vocaid vs 125A production mix

The *original Vocaid* capture method above currently assumes that its internal `dryLevel=1-Mix` flows through JUCE's x2 dry scale and thus divides **original** dry.wav by 2 to infer the source PCM. This is a **research hypothesis**, not our target gain law. The active **125A Retro Reverb** uses a deliberate hard-zero rule: `Mix=0` returns unity-gain dry (0 dB) with no tail, including immediately after a parameter change; `Mix=100%` returns wet only. The comparator's Vocaid-only /2 factor must not be copied back into the 125A production dry path.
