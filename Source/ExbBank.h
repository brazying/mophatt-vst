#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct ExbPreset
{
    juce::String name;
    int number = -1;
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
