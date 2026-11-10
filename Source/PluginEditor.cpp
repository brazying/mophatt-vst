#include "PluginEditor.h"

MoPhattVSTAudioProcessorEditor::MoPhattVSTAudioProcessorEditor(MoPhattVSTAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(620, 270);

    title.setFont(juce::Font(27.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(title);

    subtitle.setFont(juce::Font(11.0f));
    subtitle.setColour(juce::Label::textColourId, juce::Colour(0xff9298a3));
    addAndMakeVisible(subtitle);

    addBankButton.onClick = [this] { chooseBank(); };
    addBankButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffd6ff3f));
    addBankButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff111111));
    addBankButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd6ff3f));
    addAndMakeVisible(addBankButton);

    auto styleCombo = [](juce::ComboBox& combo)
    {
        combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff17191e));
        combo.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff30343c));
        combo.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffd6ff3f));
    };

    styleCombo(bankSelector);
    styleCombo(presetSelector);
    bankSelector.setTextWhenNothingSelected("Choose a bank");
    presetSelector.setTextWhenNothingSelected("Choose a preset");
    bankSelector.onChange = [this] { refreshPresetList(); };
    presetSelector.onChange = [this] { loadSelectedPreset(); };
    addAndMakeVisible(bankSelector);
    addAndMakeVisible(presetSelector);

    previousPresetButton.onClick = [this] { stepPreset(-1); };
    nextPresetButton.onClick = [this] { stepPreset(1); };
    for (auto* button : { &previousPresetButton, &nextPresetButton })
    {
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff303640));
        button->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible(*button);
    }

    bankLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    bankLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9298a3));
    addAndMakeVisible(bankLabel);

    presetLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    presetLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9298a3));
    addAndMakeVisible(presetLabel);

    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffd6ff3f));
    addAndMakeVisible(statusLabel);

    refreshBankList();
}

void MoPhattVSTAudioProcessorEditor::refreshBankList()
{
    bankSelector.clear();
    for (int i = 0; i < static_cast<int>(processor.getBanks().size()); ++i)
    {
        const auto& bank = processor.getBanks()[static_cast<size_t>(i)];
        bankSelector.addItem(bank.bankName, i + 1);
    }

    if (processor.getBanks().empty())
    {
        presetSelector.clear();
        presetItems.clear();
        statusLabel.setText("LOAD AN EXB BANK TO GET STARTED", juce::dontSendNotification);
        return;
    }

    bankSelector.setSelectedId(bankSelector.getNumItems(), juce::dontSendNotification);
    refreshPresetList();
}

void MoPhattVSTAudioProcessorEditor::refreshPresetList()
{
    presetSelector.clear();
    presetItems.clear();

    const int bankIndex = bankSelector.getSelectedId() - 1;
    if (!juce::isPositiveAndBelow(bankIndex, static_cast<int>(processor.getBanks().size())))
    {
        statusLabel.setText("SELECT A BANK", juce::dontSendNotification);
        return;
    }

    const auto& bank = processor.getBanks()[static_cast<size_t>(bankIndex)];
    for (int i = 0; i < static_cast<int>(bank.presets.size()); ++i)
    {
        presetSelector.addItem(bank.presets[static_cast<size_t>(i)].name, i + 1);
        presetItems.emplace_back(bankIndex, i);
    }

    if (presetItems.empty())
    {
        statusLabel.setText("NO PRESETS FOUND IN THIS BANK", juce::dontSendNotification);
        return;
    }

    presetSelector.setSelectedId(1, juce::sendNotification);
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
            {
                refreshBankList();
                statusLabel.setText(message, juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText(message, juce::dontSendNotification);
            }
        });
}

void MoPhattVSTAudioProcessorEditor::loadSelectedPreset()
{
    const int index = presetSelector.getSelectedId() - 1;
    if (!juce::isPositiveAndBelow(index, static_cast<int>(presetItems.size())))
        return;

    const auto selection = presetItems[static_cast<size_t>(index)];
    juce::String message;
    processor.loadPreset(selection.first, selection.second, message);
    statusLabel.setText(message, juce::dontSendNotification);
}

void MoPhattVSTAudioProcessorEditor::stepPreset(int direction)
{
    const int count = presetSelector.getNumItems();
    if (count == 0)
        return;

    const int current = presetSelector.getSelectedId();
    const int next = juce::jlimit(1, count, current + direction);
    if (next != current)
        presetSelector.setSelectedId(next, juce::sendNotification);
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
    addBankButton.setBounds(getWidth() - 158, 30, 134, 36);

    bankLabel.setBounds(26, 111, 80, 16);
    bankSelector.setBounds(24, 130, getWidth() - 48, 34);

    presetLabel.setBounds(26, 178, 80, 16);
    previousPresetButton.setBounds(24, 198, 38, 38);
    presetSelector.setBounds(70, 198, getWidth() - 140, 38);
    nextPresetButton.setBounds(getWidth() - 62, 198, 38, 38);

    statusLabel.setBounds(26, 242, getWidth() - 52, 20);
}
