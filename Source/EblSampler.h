#pragma once

#include <JuceHeader.h>
#include "EblSample.h"
#include <memory>

class EblSamplerSound : public juce::SynthesiserSound
{
public:
    explicit EblSamplerSound(std::shared_ptr<EblSample> dataIn) : data(std::move(dataIn)) {}

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }

    std::shared_ptr<EblSample> data;
};

class EblSamplerVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int controllerNumber, int newValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

private:
    std::shared_ptr<EblSample> sample;
    double position = 0.0;
    double playbackRatio = 1.0;
    float noteLevel = 0.0f;
    int releaseSamplesRemaining = 0;
    int releaseSamplesTotal = 1;
    bool releasing = false;
};
