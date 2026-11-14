cmake_minimum_required(VERSION 3.25)
project(A125_Retro_Reverb VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(freeverb_core STATIC
  third_party/freeverb/Components/allpass.cpp
  third_party/freeverb/Components/comb.cpp
  third_party/freeverb/Components/revmodel.cpp)
target_include_directories(freeverb_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/freeverb/Components")
set_target_properties(freeverb_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Reusable 125A voice/reverb DSP model matching JUCE 6.1.6 architecture.
add_library(vocaid_reference_core INTERFACE)
target_include_directories(vocaid_reference_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

enable_testing()
add_executable(reverb_tests tests/reverb_tests.cpp)
target_link_libraries(reverb_tests PRIVATE freeverb_core)
add_test(NAME reverb_core_tests COMMAND reverb_tests)
add_executable(freeverb_mix_tests tests/freeverb_mix_tests.cpp)
target_include_directories(freeverb_mix_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(freeverb_mix_tests PRIVATE freeverb_core)
add_test(NAME freeverb_mix_tests COMMAND freeverb_mix_tests)
add_executable(vocaid_reference_tests tests/reference_reverb_tests.cpp)
target_link_libraries(vocaid_reference_tests PRIVATE vocaid_reference_core)
add_test(NAME vocaid_reference_tests COMMAND vocaid_reference_tests)
add_executable(juce_616_differential_tests tests/juce_616_differential_tests.cpp)
target_include_directories(juce_616_differential_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests" "${CMAKE_CURRENT_SOURCE_DIR}/third_party/juce_616")
target_link_libraries(juce_616_differential_tests PRIVATE vocaid_reference_core)
add_test(NAME juce_616_differential_tests COMMAND juce_616_differential_tests)
add_executable(vocaid_ab_renderer tools/vocaid_ab_renderer.cpp)
target_include_directories(vocaid_ab_renderer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(vocaid_ab_renderer PRIVATE freeverb_core)

option(A125_BUILD_VST3 "Build the native Windows VST3 (Steinberg SDK)" OFF)
if(A125_BUILD_VST3)
  if(NOT WIN32)
    message(FATAL_ERROR "This project only produces Windows-x64 VST3 plugins")
  endif()
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk/CMakeLists.txt")
    message(FATAL_ERROR "Clone the pinned Steinberg VST3 SDK with submodules into external/vst3sdk first; see START-HERE.md")
  endif()
  set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
  set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
  add_subdirectory(external/vst3sdk "${CMAKE_BINARY_DIR}/vst3sdk")
  smtg_add_vst3plugin(A125_Retro_Reverb
    external/vst3sdk/public.sdk/source/vst/vstsinglecomponenteffect.cpp
    plugin/retro_processor.cpp
    plugin/retro_processor.hpp
    plugin/factory.cpp)
  target_include_directories(A125_Retro_Reverb PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(A125_Retro_Reverb PRIVATE sdk freeverb_core)
  target_compile_features(A125_Retro_Reverb PRIVATE cxx_std_23)
endif()
