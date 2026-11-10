#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <utility>
#include <vector>

class MoPhattVSTAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MoPhattVSTAudioProcessorEditor(MoPhattVSTAudioProcessor&);
    ~MoPhattVSTAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void chooseBank();
    void chooseEbl();
    void loadSelectedPreset();
    void refreshBankList();

    MoPhattVSTAudioProcessor& processor;
    juce::TextButton addBankButton { "ADD BANK" };
    juce::TextButton loadEblButton { "LOAD EBL PREVIEW" };
    juce::TextButton loadPresetButton { "LOAD PRESET" };
    juce::ComboBox presetSelector;
    juce::TextEditor bankList;
    juce::Label title { {}, "MO'PHATT" };
    juce::Label subtitle { {}, "E-MU BANK SAMPLER  /  VST3" };
    juce::Label previewLabel { {}, "NO PRESET OR SAMPLE LOADED" };
    std::vector<std::pair<int, int>> presetItems;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoPhattVSTAudioProcessorEditor)
};
