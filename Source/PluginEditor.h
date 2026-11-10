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
    void refreshBankList();
    void refreshPresetList();
    void loadSelectedPreset();
    void stepPreset(int direction);

    MoPhattVSTAudioProcessor& processor;
    juce::TextButton addBankButton { "+  ADD BANK" };
    juce::ComboBox bankSelector;
    juce::ComboBox presetSelector;
    juce::TextButton previousPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::Label title { {}, "MO'PHATT" };
    juce::Label subtitle { {}, "E-MU BANK SAMPLER  /  VST3" };
    juce::Label bankLabel { {}, "BANK" };
    juce::Label presetLabel { {}, "PRESET" };
    juce::Label statusLabel { {}, "LOAD AN EXB BANK TO GET STARTED" };
    std::vector<std::pair<int, int>> presetItems;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoPhattVSTAudioProcessorEditor)
};
