#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

struct EblSample
{
    juce::String name;
    int sampleRate = 44100;
    int rootNote = 60;
    bool hasLoop = false;
    int loopStart = 0;
    int loopEnd = 0;
    juce::AudioBuffer<float> audio;

    static bool load(const juce::File& file, EblSample& result, juce::String& error);
};
