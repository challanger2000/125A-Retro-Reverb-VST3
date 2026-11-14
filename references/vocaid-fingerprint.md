# Virtual Vocaid reference – static fingerprint (not audio sign-off)

These findings come from an earlier static examination of the user-supplied Windows VST3. They are provided for reproducibility, **not** as proof of a 1:1 clone.

- Original file size: 6,326,784 bytes. SHA-256: `32a8f6fe068bfeaf743a00a4b14caa65bf0287ea3964240ddbb78db4547ed2c2`.
- UI parameter strings: Reverb Enable, Size, Damping, Width, Mix.
- 8 classic comb lengths in contiguous table: 1116,1188,1277,1356,1422,1491,1557,1617.
- Freeverb-related scalar candidates: roomScale .28, roomOffset .7, dampingScale .4, wetScale 3, dryScale 2, gain .015.
- Candidate parameter routing: Wet=Mix, Dry=1−Mix.
- No plain RIFF/WAVE or FORM/AIFF headers found. Presence of an FFT convolver class does not establish that reverb uses an IR.

**Unresolved:** actual sample-for-sample signal path, additional coloration, exact parameter routing and 48/96 kHz behavior. In particular, the original fixed-length Freeverb core is NOT automatically equivalent to the sample-rate-aware JUCE implementation in Virtual Vocaid.

Audio A/B gate: capture impulse + spoken signal from Vocaid and this plugin at 44.1 kHz using controlled matching settings, compare onset, wet/dry level, stereo separation, energy decay and spectral response; do not normalize away genuine level differences.
