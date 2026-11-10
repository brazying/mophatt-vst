#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

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

    MoPhattVSTAudioProcessor& processor;
    juce::TextButton addBankButton { "ADD BANK" };
    juce::TextEditor bankList;
    juce::Label title { {}, "MO'PHATT" };
    juce::Label subtitle { {}, "E-MU BANK SAMPLER  /  VST3" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoPhattVSTAudioProcessorEditor)
};
