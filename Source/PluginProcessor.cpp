#include "PluginProcessor.h"
#include "PluginEditor.h"

MoPhattVSTAudioProcessor::MoPhattVSTAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void MoPhattVSTAudioProcessor::prepareToPlay(double, int) {}
void MoPhattVSTAudioProcessor::releaseResources() {}

bool MoPhattVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

void MoPhattVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
}

juce::AudioProcessorEditor* MoPhattVSTAudioProcessor::createEditor()
{
    return new MoPhattVSTAudioProcessorEditor(*this);
}

bool MoPhattVSTAudioProcessor::addBank(const juce::File& file, juce::String& message)
{
    auto bank = ExbBank::inspect(file);
    if (!bank.isValid())
    {
        message = bank.error;
        return false;
    }

    for (const auto& existing : banks)
    {
        if (existing.file.getFullPathName() == file.getFullPathName())
        {
            message = "This bank is already loaded.";
            return false;
        }
    }

    message = "Loaded " + bank.bankName + " — "
            + juce::String(static_cast<int>(bank.presets.size())) + " preset labels found.";
    banks.push_back(std::move(bank));
    return true;
}

void MoPhattVSTAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement state("MOPHATT_VST_STATE");
    for (const auto& bank : banks)
    {
        auto* node = state.createNewChildElement("BANK");
        node->setAttribute("path", bank.file.getFullPathName());
    }
    copyXmlToBinary(state, destData);
}

void MoPhattVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    banks.clear();
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr || !xml->hasTagName("MOPHATT_VST_STATE"))
        return;

    for (auto* node = xml->getFirstChildElement(); node != nullptr; node = node->getNextElement())
    {
        if (node->hasTagName("BANK"))
        {
            const juce::File file(node->getStringAttribute("path"));
            juce::String ignored;
            addBank(file, ignored);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoPhattVSTAudioProcessor();
}
