@echo off
setlocal
cd /d "%~dp0"
if not exist "external\vst3sdk\CMakeLists.txt" (
  git clone --recursive --depth 1 --branch v3.8.1_build_84 https://github.com/steinbergmedia/vst3sdk.git external/vst3sdk
  if errorlevel 1 exit /b 1
)
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DA125_BUILD_VST3=ON -DSMTG_CREATE_PLUGIN_LINK=OFF
if errorlevel 1 exit /b 1
cmake --build build-win --config Release --target A125_Retro_Reverb reverb_tests --parallel 4
if errorlevel 1 exit /b 1
ctest --test-dir build-win -C Release --output-on-failure
if errorlevel 1 exit /b 1
echo P001 source built and CTest completed. Acoustic A/B approval still pending.
endlocal
