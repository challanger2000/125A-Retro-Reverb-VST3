@echo off
setlocal
cd /d "%~dp0"
if not exist "external\vst3sdk\CMakeLists.txt" (
  git clone --recursive --depth 1 --branch v3.8.1_build_84 https://github.com/steinbergmedia/vst3sdk.git external/vst3sdk
  if errorlevel 1 exit /b 1
)
if not exist "external\ugly-gui\source\UglyControls.cpp" (
  git clone --filter=blob:none --no-checkout https://github.com/challanger2000/125A-Ugly-Reverb-Final.git external/ugly-gui
  if errorlevel 1 exit /b 1
  git -C external/ugly-gui checkout --detach 631c79b5fddfb19b916d927f0918134576a3a031
  if errorlevel 1 exit /b 1
)
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DA125_BUILD_VST3=ON -DSMTG_CREATE_PLUGIN_LINK=OFF
if errorlevel 1 exit /b 1
cmake --build build-win --config Release --target 125A_Retro_Reverb reverb_tests freeverb_mix_tests vocaid_reference_tests juce_616_differential_tests vocaid_ab_renderer retro_native_processor_tests --parallel 4
if errorlevel 1 exit /b 1
ctest --test-dir build-win -C Release --output-on-failure
if errorlevel 1 exit /b 1
echo P004 original Freeverb with reused Ugly Reverb VSTGUI built and tested.
endlocal
