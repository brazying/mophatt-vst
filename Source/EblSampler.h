#pragma once

#include <JuceHeader.h>
#include "EblSample.h"
#include <memory>
#include <utility>

class EblSamplerSound : public juce::SynthesiserSound
{
public:
    EblSamplerSound(std::shared_ptr<EblSample> dataIn,
                    int keyLowIn = 0, int keyHighIn = 127,
                    int velocityLowIn = 0, int velocityHighIn = 127,
                    int rootNoteIn = 60)
        : data(std::move(dataIn)),
          keyLow(keyLowIn), keyHigh(keyHighIn),
          velocityLow(velocityLowIn), velocityHigh(velocityHighIn),
          rootNote(rootNoteIn) {}

    bool appliesToNote(int midiNoteNumber) override
    {
        return midiNoteNumber >= keyLow && midiNoteNumber <= keyHigh;
    }

    bool appliesToChannel(int) override { return true; }

    std::shared_ptr<EblSample> data;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 0;
    int velocityHigh = 127;
    int rootNote = 60;
};

class EblSamplerVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples) override;

private:
    std::shared_ptr<EblSample> sample;
    double position = 0.0;
    double playbackRatio = 1.0;
    float noteLevel = 1.0f;
    bool releasing = false;
    int releaseSamplesRemaining = 0;
    int releaseSamplesTotal = 1;
};
