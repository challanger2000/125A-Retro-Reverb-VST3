# START HERE — 125A Retro Reverb

**Current branch scope:** P004.5 / Windows x64 VST3 **0.3.5**, technical development candidate, **not a commercially approved final release**.

Read [README.md](README.md) for the current plugin contract and [docs/P0045-TECHNICAL-QA.md](docs/P0045-TECHNICAL-QA.md) for the tested revision, QA evidence and remaining gaps. Follow the applicable central `125A-Engineering` rules for new development.

## Build

Windows / Visual Studio 2022 Desktop C++, CMake ≥3.25, Python 3 and Git:

```bat
build_windows.bat
```

The script checks the exact Steinberg VST3 SDK commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` and reuses the pinned Ugly Reverb GUI. Five CTests run, including the **real production processor** audio/state regression. Windows Actions additionally runs Steinberg Validator, independent P74/P98 technical baseline and 125A Plugin Tester v0.3.2.

**Not yet run:** real Fender Studio Pro / physical ASIO and complete hands-on GUI/sonic acceptance. `TECHNICAL_BASELINE_PASS` is not `release_approved=true`.

**Original DSP:** Jezar Freeverb, unchanged fixed delays originally designed for 44.1 kHz. User-facing 125A Mix changes only the wet/dry law: 0% dry unity, 100% fully wet.
