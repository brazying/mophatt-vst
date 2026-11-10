#pragma once

#include <JuceHeader.h>
#include "ExbBank.h"
#include "EblSampler.h"
#include <vector>

class MoPhattVSTAudioProcessor : public juce::AudioProcessor
{
public:
    MoPhattVSTAudioProcessor();
    ~MoPhattVSTAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool addBank(const juce::File& file, juce::String& message);
    bool loadEblPreview(const juce::File& file, juce::String& message);
    bool loadPreset(int bankIndex, int presetIndex, juce::String& message);
    const std::vector<ExbBankInfo>& getBanks() const { return banks; }
    juce::String getPreviewName() const { return previewName; }
    juce::String getPreviewInfo() const { return previewInfo; }

private:
    std::vector<ExbBankInfo> banks;
    juce::Synthesiser synth;
    juce::File previewFile;
    juce::String previewName;
    juce::String previewInfo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoPhattVSTAudioProcessor)
};
