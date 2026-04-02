cmake_minimum_required(VERSION 3.16)
project(Microcosm VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.3
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(JUCE)

# Plugin
juce_add_plugin(Microcosm
    COMPANY_NAME "Kuuhaku"
    PLUGIN_MANUFACTURER_CODE Kuhk
    PLUGIN_CODE Micr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Microcosm"
)

target_sources(Microcosm
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/GranularEngine.cpp
        Source/CircularBuffer.cpp
        Source/ProcessingModule.cpp
        Source/ActivityModules.cpp
        Source/EffectModules.cpp
        Source/ModulationSystem.cpp
        Source/ModulationMatrix.cpp
        Source/ModulationUI.cpp
        Source/EQModule.cpp
)

target_compile_definitions(Microcosm
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Microcosm
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
