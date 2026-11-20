cmake_minimum_required(VERSION 3.22)
# macOS: universal binary (Apple Silicon + Intel), macOS 11+.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
project(CreamGritVST VERSION 0.6.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

# JUCE: vst/external/JUCE (git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE external/JUCE)
add_subdirectory(external/JUCE)

if(APPLE)
  set(CREAM_FORMATS VST3 AU Standalone)
else()
  set(CREAM_FORMATS VST3 Standalone)
endif()

option(CREAM_COPY_PLUGIN "Copy VST3 to the system plug-in folder after build" OFF)

function(cream_plugin target product code master)
  juce_add_plugin(${target}
    COMPANY_NAME "ufotone"
    PLUGIN_MANUFACTURER_CODE U4gr
    PLUGIN_CODE ${code}
    PRODUCT_NAME "${product}"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Delay Distortion
    FORMATS ${CREAM_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD ${CREAM_COPY_PLUGIN})
  target_sources(${target} PRIVATE plugin/CreamGritProcessor.cpp)
  target_compile_definitions(${target} PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
  if(master)
    target_compile_definitions(${target} PUBLIC CREAM_GRIT_MASTER=1)
  endif()
  target_link_libraries(${target} PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endfunction()

cream_plugin(CreamGritM "Cream Grit M" Crg2 ON)

# Null test: original drumlogue DSP vs. this port (bit-exact at 48 kHz).
add_executable(null_test_delay tests/null_test.cpp)
add_executable(null_test_master tests/null_test.cpp)
target_compile_definitions(null_test_master PRIVATE CREAM_GRIT_MASTER=1)

add_executable(echo_probe tests/echo_probe.cpp)

if(WIN32)
# Loopback FX Host: WASAPI loopback of a Windows output -> VST3 -> another output.
juce_add_gui_app(LoopbackFxHost PRODUCT_NAME "Loopback FX Host" COMPANY_NAME "ufotone")
target_sources(LoopbackFxHost PRIVATE host/Main.cpp)
target_compile_definitions(LoopbackFxHost PRIVATE
  JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_APPLICATION_NAME_STRING="Loopback FX Host" JUCE_APPLICATION_VERSION_STRING="0.1.0")
target_link_libraries(LoopbackFxHost PRIVATE juce::juce_audio_utils ole32
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
target_compile_options(LoopbackFxHost PRIVATE /utf-8)
endif()

# Bit-exact comparison needs identical float rounding in both DSP copies (no FMA contraction).
if(NOT MSVC)
  target_compile_options(null_test_delay PRIVATE -ffp-contract=off)
  target_compile_options(null_test_master PRIVATE -ffp-contract=off)
  target_compile_definitions(null_test_delay PRIVATE NULL_TOLERANCE=1e-4)
  target_compile_definitions(null_test_master PRIVATE NULL_TOLERANCE=1e-4)
endif()
add_executable(latency_probe tests/latency_probe.cpp)
add_executable(ring_level_probe tests/ring_level_probe.cpp)
