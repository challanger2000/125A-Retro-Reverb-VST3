# 125A Retro Reverb – P001

Native Windows x64 VST3 based on Jezar's **original public-domain Freeverb** C++ filter network (see `third_party/freeverb/Components`). This is a distinct, independently packaged plugin and a starting point for reproducing the vintage reverb timbre heard in Virtual Vocaid.

**Status (P001): source candidate, NOT a verified Virtual Vocaid clone.** The original filter network is present and regression tests exist. P001 contains one documented reset-only correction to `comb::mute()` so the internal low-pass feedback is cleared. Windows VST3 compilation, host validation and actual Virtual Vocaid A/B output comparisons must be demonstrated by CI and reference renders. No GUI beyond host-provided generic parameter controls in P001.

## Targets

- **Windows x64 VST3 only** (Steinberg SDK 3.8.1, VS2022, C++23)
- Same original DSP source for a future 125A speech plugin, via the reusable `freeverb_core` target
- Parameters: Enable, Size, Damping, Width, Mix
- No unnecessary extra EQ, predelay or modulation until the reference sound is reproduced

## Build

Read [START-HERE.md](START-HERE.md) for Windows builds and tests. The build does **not** require JUCE. Use `-DA125_BUILD_VST3=OFF` to test only the Freeverb core without the SDK.

## Fidelity caveats

The original Freeverb tuning assumes 44.1 kHz; at 48/96 kHz its delay lengths must eventually be scaled and then checked against actual Virtual Vocaid output. P001 retains the original numbers deliberately. The reference Mix routing sets Wet = Mix, Dry = 1 - Mix at the Freeverb API level, where the underlying wet and dry scales are **3** and **2**, respectively. Therefore 0% Mix yields 2x dry amplitude: this is a *reference-mode hypothesis*, not a recommended production bypass rule. Enable=off always passes signal unchanged.

The binary fingerprint of the user-supplied Virtual Vocaid remains a guide; the copyrighted VST3 and samples are not included. No claim of sample-identical sound without an actual measured A/B comparison.

## Licenses

Jezar's original source explicitly declares public domain. The independently written wrapper is intended for use under the repository's future project license. Steinberg VST3 SDK 3.8.1 is MIT licensed and must be obtained separately. See [THIRD_PARTY.md](THIRD_PARTY.md).

## P002 – Virtual Vocaid reference-mode engine

The actual VST3 effect now uses `src/reference_reverb.hpp`, a standalone C++23 implementation of the **JUCE 6.1.6-style** Freeverb network identified in the original Vocaid binary. It includes 44.1/48/96k sample-rate scaling, 10 ms parameter smoothing, Stereo/Stereo and Mono/Mono or Mono/Stereo routing, and no audio callback heap allocations. The historical Jezar Freeverb remains in `third_party` as a reference but is **not** the active VST3 DSP.

The comparison is **architectural/static**, not yet a verified 1:1 audio reproduction. See `docs/REFERENCE-DSP.md` for evidence and remaining audio A/B gate.
