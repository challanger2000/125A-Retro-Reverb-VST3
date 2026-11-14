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
