#pragma once

#include <JuceHeader.h>
#include "EblSample.h"
#include "ExbBank.h"
#include <memory>
#include <utility>

class EblSamplerSound : public juce::SynthesiserSound
{
public:
    EblSamplerSound(std::shared_ptr<EblSample> dataIn,
                    int keyLowIn = 0, int keyHighIn = 127,
                    int velocityLowIn = 0, int velocityHighIn = 127,
                    int rootNoteIn = 60,
                    std::shared_ptr<const ExbVoiceParameters> parametersIn = {})
        : data(std::move(dataIn)),
          keyLow(keyLowIn), keyHigh(keyHighIn),
          velocityLow(velocityLowIn), velocityHigh(velocityHighIn),
          rootNote(rootNoteIn), parameters(std::move(parametersIn)) {}

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
    std::shared_ptr<const ExbVoiceParameters> parameters;
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
    void updatePlaybackRatio();
    float nextAmplitudeEnvelopeSample();
    void beginAmplitudeRelease();

    std::shared_ptr<EblSample> sample;
    double position = 0.0;
    double basePlaybackRatio = 1.0;
    double playbackRatio = 1.0;
    float noteLevel = 1.0f;
    float attackLevel = 0.0f;
    bool releasing = false;
    int pitchWheel = 8192;
    int releaseSamplesRemaining = 0;
    int releaseSamplesTotal = 1;
    int attackSamplesElapsed = 0;
    int attackSamplesTotal = 1;
    ExbEnvelope amplitudeEnvelope;
    int envelopeStage = 0;
    int envelopeStageSamples = 1;
    int envelopeStageElapsed = 0;
    float envelopeStageStart = 0.0f;
    float envelopeLevel = 0.0f;
    bool useProgramEnvelope = false;
    float programGain = 1.0f;
};
