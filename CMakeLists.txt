cmake_minimum_required(VERSION 3.22)
project(MoPhattVST VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MoPhattVST
    COMPANY_NAME "brazying"
    BUNDLE_ID "com.brazying.mophattvst"
    PLUGIN_MANUFACTURER_CODE Brzy
    PLUGIN_CODE Mpht
    FORMATS VST3 Standalone
    PRODUCT_NAME "Mo'Phatt"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(MoPhattVST)

target_sources(MoPhattVST PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ExbBank.cpp
    Source/EblSample.cpp
    Source/EblSampler.cpp
)

target_compile_features(MoPhattVST PRIVATE cxx_std_17)
target_compile_definitions(MoPhattVST PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(MoPhattVST PRIVATE Source)

target_link_libraries(MoPhattVST PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
