#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct ExbZone
{
    int sampleIndex = 0;
    int rootNote = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 0;
    int velocityHigh = 127;
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
