#include "PluginProcessor.h"
#include "PluginEditor.h"

MoPhattVSTAudioProcessor::MoPhattVSTAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    for (int i = 0; i < 32; ++i)
        synth.addVoice(new EblSamplerVoice());
}

void MoPhattVSTAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
}

void MoPhattVSTAudioProcessor::releaseResources() {}

bool MoPhattVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
}

void MoPhattVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
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
            + juce::String(static_cast<int>(bank.presets.size())) + " presets, "
            + juce::String(bank.sampleCount) + " sample links.";
    banks.push_back(std::move(bank));
    return true;
}

bool MoPhattVSTAudioProcessor::loadEblPreview(const juce::File& file, juce::String& message)
{
    EblSample parsed;
    if (!EblSample::load(file, parsed, message))
        return false;

    auto sample = std::make_shared<EblSample>(std::move(parsed));
    synth.allNotesOff(1, false);
    synth.clearSounds();
    synth.addSound(new EblSamplerSound(sample));

    previewFile = file;
    previewName = sample->name;
    previewInfo = juce::String(sample->sampleRate) + " Hz  /  "
                + juce::String(sample->audio.getNumChannels()) + " ch  /  "
                + juce::String(sample->audio.getNumSamples()) + " frames";
    message = "Loaded sample: " + previewName;
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

    if (previewFile.existsAsFile())
    {
        auto* node = state.createNewChildElement("PREVIEW");
        node->setAttribute("path", previewFile.getFullPathName());
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
        else if (node->hasTagName("PREVIEW"))
        {
            const juce::File file(node->getStringAttribute("path"));
            juce::String ignored;
            loadEblPreview(file, ignored);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoPhattVSTAudioProcessor();
}
