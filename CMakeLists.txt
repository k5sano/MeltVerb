cmake_minimum_required(VERSION 3.22)
project(MeltVerb VERSION 1.0.0)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

if(NOT DEFINED JUCE_DIR)
    if(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR $ENV{JUCE_DIR})
    else()
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
    endif()
endif()

add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

juce_add_plugin(MeltVerb
    PLUGIN_MANUFACTURER_CODE K5sn
    PLUGIN_CODE MlVb
    COMPANY_NAME "K5SANO"
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "MeltVerb"
    PLUGIN_MANUFACTURER_EMAIL "contact@k5sano.com"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

juce_generate_juce_header(MeltVerb)

target_sources(MeltVerb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PresetManager.cpp
    Source/DSP/MeltEngine.cpp
    Source/DSP/ReverbTank.cpp
)

target_include_directories(MeltVerb PRIVATE
    Source
    Source/DSP
)

target_compile_features(MeltVerb PRIVATE cxx_std_17)

target_compile_definitions(MeltVerb PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

if(MSVC)
    target_compile_definitions(MeltVerb PRIVATE _USE_MATH_DEFINES)
endif()

target_link_libraries(MeltVerb
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_audio_plugin_client
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
)

# --- Tests ---
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        add_executable(MeltVerbTests
            Tests/MeltEngineTests.cpp
            Source/DSP/MeltEngine.cpp
            Source/DSP/ReverbTank.cpp
        )
        set_target_properties(MeltVerbTests PROPERTIES OSX_ARCHITECTURES "arm64")
        target_include_directories(MeltVerbTests PRIVATE
            Source Source/DSP
        )
        target_compile_features(MeltVerbTests PRIVATE cxx_std_17)
        target_link_libraries(MeltVerbTests PRIVATE Catch2::Catch2WithMain)
        if(MSVC)
            target_compile_definitions(MeltVerbTests PRIVATE _USE_MATH_DEFINES)
        endif()
        include(CTest)
        include(Catch)
        catch_discover_tests(MeltVerbTests)
    endif()
endif()
