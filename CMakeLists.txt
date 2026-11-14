cmake_minimum_required(VERSION 3.25)
project(125A_Retro_Reverb VERSION 0.3.0 LANGUAGES CXX)
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
  set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
  set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
  add_subdirectory(external/vst3sdk "${CMAKE_BINARY_DIR}/vst3sdk")

  # Pin and reuse the working Ugly Reverb GUI. The original source is
  # compiled as a control library, NEVER as another audio plugin.
  set(UGLY_GUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/ugly-gui")
  if(NOT EXISTS "${UGLY_GUI_ROOT}/source/UglyControls.cpp"
     OR NOT EXISTS "${UGLY_GUI_ROOT}/resource/fender_amp1.png")
    message(FATAL_ERROR "Run build_windows.bat to fetch the pinned Ugly Reverb GUI")
  endif()
  file(READ "${UGLY_GUI_ROOT}/resource/125A_Logo_Master_FINAL.svg" UGLY_BRANDING_SVG)
  string(REGEX MATCHALL "<path d=\\\"[^\\\"]+\\\" fill=\\\"#[0-9A-Fa-f]+\\\"/>"
         UGLY_BRANDING_PATH_TAGS "${UGLY_BRANDING_SVG}")
  list(LENGTH UGLY_BRANDING_PATH_TAGS UGLY_BRANDING_PATH_COUNT)
  if(NOT UGLY_BRANDING_PATH_COUNT EQUAL 30)
    message(FATAL_ERROR "125A logo source unexpectedly changed")
  endif()
  set(UGLY_BRANDING_PATHS "")
  foreach(TAG IN LISTS UGLY_BRANDING_PATH_TAGS)
    if(NOT TAG MATCHES "^<path d=\\\"([^\\\"]+)\\\" fill=\\\"([^\\\"]+)\\\"/>$")
      message(FATAL_ERROR "Cannot parse original branding")
    endif()
    set(UGLY_BRANDING_D "${CMAKE_MATCH_1}")
    if("${CMAKE_MATCH_2}" STREQUAL "#D71920")
      set(UGLY_BRANDING_RED "true")
    elseif("${CMAKE_MATCH_2}" STREQUAL "#D9D9D9")
      set(UGLY_BRANDING_RED "false")
    else()
      message(FATAL_ERROR "Unexpected 125A logo color")
    endif()
    string(APPEND UGLY_BRANDING_PATHS "    {\"${UGLY_BRANDING_D}\", ${UGLY_BRANDING_RED}},\n")
  endforeach()
  configure_file("${UGLY_GUI_ROOT}/source/branding_master.h.in"
                 "${CMAKE_BINARY_DIR}/branding_master.h" @ONLY)

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  set(RETRO_GUI_GENERATED "${CMAKE_BINARY_DIR}/retro_gui_resources")
  file(MAKE_DIRECTORY "${RETRO_GUI_GENERATED}")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${UGLY_GUI_ROOT}/scripts/generate_knob_strips.py"
            "${UGLY_GUI_ROOT}/resource/fender_amp1.png"
            "${RETRO_GUI_GENERATED}"
    RESULT_VARIABLE RETRO_GUI_RESULT
    OUTPUT_VARIABLE RETRO_GUI_LOG
    ERROR_VARIABLE RETRO_GUI_ERROR)
  if(NOT RETRO_GUI_RESULT EQUAL 0)
    message(FATAL_ERROR "Original Ugly Reverb knob generation failed: ${RETRO_GUI_ERROR}")
  endif()
  message(STATUS "${RETRO_GUI_LOG}")
  smtg_add_vst3plugin(125A_Retro_Reverb
    external/vst3sdk/public.sdk/source/main/dllmain.cpp
    external/vst3sdk/public.sdk/source/vst/vstsinglecomponenteffect.cpp
    plugin/retro_processor.cpp
    plugin/retro_processor.hpp
    plugin/retro_gui.cpp
    plugin/factory.cpp
    resource/retro_reverb.uidesc
    "${UGLY_GUI_ROOT}/source/UglyControls.cpp"
    "${UGLY_GUI_ROOT}/source/UglyControls.h")
  target_include_directories(125A_Retro_Reverb PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${UGLY_GUI_ROOT}/source"
    "${CMAKE_BINARY_DIR}")
  target_link_libraries(125A_Retro_Reverb PRIVATE sdk freeverb_core vstgui_support)
  smtg_target_add_plugin_resources(125A_Retro_Reverb
    RESOURCES
      resource/retro_reverb.uidesc
      "${RETRO_GUI_GENERATED}/ugly_knob_62.png"
      "${RETRO_GUI_GENERATED}/ugly_knob_62#2.0x.png"
      "${RETRO_GUI_GENERATED}/ugly_knob_64.png"
      "${RETRO_GUI_GENERATED}/ugly_knob_64#2.0x.png"
      "${RETRO_GUI_GENERATED}/ugly_knob_66.png"
      "${RETRO_GUI_GENERATED}/ugly_knob_66#2.0x.png")
  target_compile_features(125A_Retro_Reverb PRIVATE cxx_std_23)
endif()
