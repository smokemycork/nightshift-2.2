cmake_minimum_required(VERSION 3.22)

# macOS: build for Apple Silicon + Intel, support macOS 10.13+
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project(NightShift VERSION 1.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(NS_BUILD_TESTS "Build the headless render test" OFF)
option(NS_LINUX_MINIMAL "Disable optional X11/ALSA features (only for minimal Linux build machines)" OFF)

# ---------------------------------------------------------------------------
# JUCE: use a local checkout if JUCE_DIR is given, otherwise download it
# ---------------------------------------------------------------------------
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(NS_FORMATS VST3 AU Standalone)
elseif(UNIX)
    set(NS_FORMATS VST3)
else()
    set(NS_FORMATS VST3 Standalone)
endif()

juce_add_plugin(NightShift
    COMPANY_NAME "NightShift Audio"
    COMPANY_WEBSITE "https://github.com"
    BUNDLE_ID com.nightshiftaudio.nightshift
    PRODUCT_NAME "NightShift"
    PLUGIN_MANUFACTURER_CODE Nsft
    PLUGIN_CODE Nsy1
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS ${NS_FORMATS})

set(NS_SOURCES
    Source/Params.cpp
    Source/Wavetables.cpp
    Source/SynthVoice.cpp
    Source/Effects.cpp
    Source/Presets.cpp
    Source/FactoryPresets.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(NightShift PRIVATE ${NS_SOURCES})

set(NS_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0)

if(NS_LINUX_MINIMAL)
    list(APPEND NS_DEFINES JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XRENDER=0 JUCE_USE_XCURSOR=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_compile_definitions(NightShift PUBLIC ${NS_DEFINES})

target_link_libraries(NightShift
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Headless test: loads every factory preset, plays notes, checks the output
# ---------------------------------------------------------------------------
if(NS_BUILD_TESTS)
    juce_add_console_app(NightShiftTests PRODUCT_NAME "NightShiftTests")
    target_sources(NightShiftTests PRIVATE Tests/RenderTest.cpp ${NS_SOURCES})
    target_compile_definitions(NightShiftTests PRIVATE ${NS_DEFINES}
        JucePlugin_Name="NightShift" JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(NightShiftTests PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
