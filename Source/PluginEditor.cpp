#include "PluginEditor.h"

MoPhattVSTAudioProcessorEditor::MoPhattVSTAudioProcessorEditor(MoPhattVSTAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(700, 500);

    title.setFont(juce::Font(27.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    subtitle.setFont(juce::Font(11.0f));
    subtitle.setColour(juce::Label::textColourId, juce::Colour(0xff9298a3));
    addAndMakeVisible(subtitle);

    addBankButton.onClick = [this] { chooseBank(); };
    addBankButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffd6ff3f));
    addBankButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff111111));
    addAndMakeVisible(addBankButton);

    loadEblButton.onClick = [this] { chooseEbl(); };
    loadEblButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff303640));
    loadEblButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(loadEblButton);

    previewLabel.setFont(juce::Font(12.0f));
    previewLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd6ff3f));
    addAndMakeVisible(previewLabel);

    bankList.setMultiLine(true);
    bankList.setReadOnly(true);
    bankList.setScrollbarsShown(true);
    bankList.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff17191e));
    bankList.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    bankList.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff30343c));
    addAndMakeVisible(bankList);

    refreshBankList();
}

void MoPhattVSTAudioProcessorEditor::refreshBankList()
{
    juce::String text;
    for (const auto& bank : processor.getBanks())
    {
        text += bank.bankName + "  [" + juce::String(static_cast<int>(bank.presets.size()))
             + " presets / " + juce::String(bank.sampleCount) + " samples]\n";
        for (const auto& preset : bank.presets)
            text += "    " + preset.name + "\n";
        text += "\n";
    }

    if (text.isEmpty())
        text = "No EXB banks loaded.\n\nClick ADD BANK to import an Emulator X bank.";

    bankList.setText(text, juce::dontSendNotification);

    const auto preview = processor.getPreviewName();
    previewLabel.setText(preview.isEmpty() ? "NO SAMPLE LOADED"
                                           : preview + "  /  " + processor.getPreviewInfo(),
                        juce::dontSendNotification);
}

void MoPhattVSTAudioProcessorEditor::chooseBank()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select an E-MU Emulator X bank", juce::File{}, "*.exb");

    chooser->launchAsync(juce::FileBrowserComponent::openMode
                       | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file == juce::File{})
                return;

            juce::String message;
            if (processor.addBank(file, message))
                refreshBankList();
            else
                bankList.setText(message, juce::dontSendNotification);
        });
}

void MoPhattVSTAudioProcessorEditor::chooseEbl()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select an E-MU sample", juce::File{}, "*.ebl");

    chooser->launchAsync(juce::FileBrowserComponent::openMode
                       | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file == juce::File{})
                return;

            juce::String message;
            if (processor.loadEblPreview(file, message))
                refreshBankList();
            else
                previewLabel.setText(message, juce::dontSendNotification);
        });
}

void MoPhattVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101216));
    g.setColour(juce::Colour(0xffd6ff3f));
    g.fillRect(0, 0, getWidth(), 4);
    g.setColour(juce::Colour(0xff252a32));
    g.drawLine(24.0f, 92.0f, static_cast<float>(getWidth() - 24), 92.0f, 1.0f);
}

void MoPhattVSTAudioProcessorEditor::resized()
{
    title.setBounds(24, 20, 300, 38);
    subtitle.setBounds(26, 59, 350, 22);
    addBankButton.setBounds(getWidth() - 300, 34, 130, 38);
    loadEblButton.setBounds(getWidth() - 158, 34, 134, 38);
    previewLabel.setBounds(26, 102, getWidth() - 52, 24);
    bankList.setBounds(24, 138, getWidth() - 48, getHeight() - 162);
}
