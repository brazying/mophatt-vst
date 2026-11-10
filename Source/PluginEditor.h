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
    void chooseEbl();
    void refreshBankList();

    MoPhattVSTAudioProcessor& processor;
    juce::TextButton addBankButton { "ADD BANK" };
    juce::TextButton loadEblButton { "LOAD EBL PREVIEW" };
    juce::TextEditor bankList;
    juce::Label title { {}, "MO'PHATT" };
    juce::Label subtitle { {}, "E-MU BANK SAMPLER  /  VST3" };
    juce::Label previewLabel { {}, "NO SAMPLE LOADED" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoPhattVSTAudioProcessorEditor)
};
