#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <memory>
#include <vector>
#include <array>

struct ExbEnvelopeStage
{
    float rate = 0.0f;
    float level = 0.0f;
};

struct ExbEnvelope
{
    std::array<ExbEnvelopeStage, 6> stages {};
    bool valid = false;
};

struct ExbFilterParameters
{
    int type = 0;
    float frequency = 0.0f;
    float resonance = 0.0f;
    bool valid = false;
};

struct ExbModulationCord
{
    int source = 0;
    int destination = 0;
    float amount = 0.0f;
};

struct ExbVoiceParameters
{
    ExbFilterParameters filter;
    std::array<ExbEnvelope, 3> envelopes;
    std::vector<ExbModulationCord> modulationCords;
    std::vector<std::vector<juce::uint8>> lfoData;
    std::vector<std::vector<juce::uint8>> functionGeneratorData;
    float amplitude = 1.0f;
    bool hasAmplitude = false;
};

struct ExbEnvelope
{
    std::array<float, 6> rates {};
    std::array<float, 6> levels {};
    bool valid = false;
};

struct ExbModCord
{
    juce::uint8 source = 0;
    juce::uint8 destination = 0;
    float amount = 0.0f;
};

struct ExbVoiceParameters
{
    float ampGainDb = 0.0f;
    int filterType = 0;
    float filterFrequency = 1.0f;
    float filterResonance = 0.0f;
    std::array<ExbEnvelope, 3> envelopes {};
    std::vector<ExbModCord> modulationCords;
};

struct ExbZone
{
    int sampleIndex = 0;
    int rootNote = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 0;
    int velocityHigh = 127;
    std::shared_ptr<const ExbVoiceParameters> voiceParameters;
};

struct ExbPreset
{
    juce::String name;
    int number = -1;
    std::vector<ExbZone> zones;
};

struct ExbBankInfo
{
    juce::File file;
    juce::String bankName;
    juce::String error;
    int sampleCount = 0;
    std::vector<ExbPreset> presets;

    bool isValid() const { return error.isEmpty(); }
};

class ExbBank
{
public:
    static ExbBankInfo inspect(const juce::File& file);

private:
    static juce::String readUtf16Le(const juce::uint8* bytes, size_t byteCount);
};
