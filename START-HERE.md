# START HERE – P001 Windows x64

## 1. Build the native VST3

Requirements: Windows, Visual Studio 2022 with Desktop C++, CMake >=3.25, Git.

Run `build_windows.bat` from this repository. It clones the **pinned** VST3 SDK 3.8.1 with submodules into ignored `external/vst3sdk`, configures VS2022 x64, builds the plugin and runs CTest.

Manual alternative:

```bat
git clone --recursive --depth 1 --branch v3.8.1_build_84 https://github.com/steinbergmedia/vst3sdk.git external/vst3sdk
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DA125_BUILD_VST3=ON -DSMTG_CREATE_PLUGIN_LINK=OFF
cmake --build build-win --config Release --target A125_Retro_Reverb reverb_tests
ctest --test-dir build-win -C Release --output-on-failure
```

The GitHub Actions workflow builds on `windows-2022`, packages the resulting .vst3 folder, and stores the ZIP as a CI artifact.

## 2. DSP-only checks

```bat
cmake -S . -B build-core -DA125_BUILD_VST3=OFF
cmake --build build-core --config Release
ctest --test-dir build-core -C Release --output-on-failure
```

## 3. Status discipline

- Core algorithm/source provenance: original Jezar Freeverb, public-domain release.
- Windows VST3: candidate until CI build artifact + validator passes.
- Audio match against Virtual Vocaid: NOT YET MEASURED.
- Studio-One/ASIO practice test: at end after DSP and GUI approval.
- Do not distribute or commit any original Virtual Vocaid binary, banks, or user ROMs.

## 4. Next engineering steps

Compare actual impulse responses at matching Size/Damping/Width/Mix and 44.1 kHz. Locate additional color/level processing in Vocaid, then add samplerate-scaled network and measurable no-allocation callback tests. Integrate the resulting shared core into the 125A speech processor only after the reverb A/B sign-off.


## Independent P74/P98 test-lab gate

The Windows CI checks out a pinned SHA of `challanger2000/125A-Plugin-Test-Lab` and builds the independent native `TestLabGUI`, `TestLabHost` and `TestLabStateStress` executables. The shared P74 pipeline uses the real source-built effect VST3 without changing its source, performs read-only ABI discovery followed by an **explicitly invoked** native audio/state/stress technical baseline, and fails unless `TECHNICAL_BASELINE_PASS` is independently confirmed. No GUI clicks, live ASIO or commercial DAW tests are implied. See `external/test-lab/docs/P98-STEREO-EFFECT-INPUT-STRESS.md`.

P002 also runs an actual copy of the **ISC-licensed JUCE 6.1.6 Reverb source** as a test-only golden reference, reporting samplewise comparisons at multiple rates. Neither the original Virtual Vocaid DLL nor its sample banks are committed to GitHub.

## P002.1 correction – 0% really means off

0% Mix bypasses the wet network and outputs **unaltered unity-gain dry**; it is NOT the +6.02 dB historical Freeverb dry mapping. Changing Mix to 0% clears the old effect tail instantly and avoids a 10 ms smoothing residue. Tests enforce the hard-zero behavior at stereo, mono and mono-to-stereo routes. At other Mix values, compare against JUCE 6.1.6 using `dryLevel = (1-Mix)/2` to cancel the original x2 dry multiplier. Our explicit user-facing Mix rule has priority over presumed Virtual Vocaid internal gain choices.
