#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <map>

namespace
{
    juce::File findEblSampleFile(const juce::File& bankFile,
                                 const juce::String& bankName,
                                 int sampleIndex)
    {
        const auto id = juce::String(sampleIndex);
        const auto paddedId = id.paddedLeft('0', 3);
        const auto prefixedName = bankName + "SL" + paddedId + ".ebl";
        const auto shortName = "SL" + paddedId + ".ebl";

        const auto parent = bankFile.getParentDirectory();
        const auto samplePool = parent.getChildFile("SamplePool");
        const auto bankFolder = parent.getChildFile(bankName);
        const juce::File directories[] = {
            samplePool,
            parent,
            bankFolder.getChildFile("SamplePool"),
            bankFolder
        };
        const juce::String names[] = {
            prefixedName,
            shortName,
            bankName + "SL" + id + ".ebl",
            "SL" + id + ".ebl",
            id + ".ebl"
        };

        for (const auto& directory : directories)
            for (const auto& name : names)
            {
                const auto candidate = directory.getChildFile(name);
                if (candidate.existsAsFile())
                    return candidate;
            }

        return {};
    }
}

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

bool MoPhattVSTAudioProcessor::loadPreset(int bankIndex, int presetIndex, juce::String& message)
{
    if (!juce::isPositiveAndBelow(bankIndex, static_cast<int>(banks.size())))
    {
        message = "Select a valid bank.";
        return false;
    }

    const auto& bank = banks[static_cast<size_t>(bankIndex)];
    if (!juce::isPositiveAndBelow(presetIndex, static_cast<int>(bank.presets.size())))
    {
        message = "Select a valid preset.";
        return false;
    }

    const auto& preset = bank.presets[static_cast<size_t>(presetIndex)];
    if (preset.zones.empty())
    {
        message = "No sample zones were found in this preset yet.";
        return false;
    }

    std::map<int, std::shared_ptr<EblSample>> loadedSamples;
    std::vector<std::unique_ptr<EblSamplerSound>> sounds;
    juce::String missingSamples;

    for (const auto& zone : preset.zones)
    {
        auto sample = loadedSamples.find(zone.sampleIndex);
        if (sample == loadedSamples.end())
        {
            const auto sampleFile = findEblSampleFile(bank.file, bank.bankName, zone.sampleIndex);

            if (!sampleFile.existsAsFile())
            {
                missingSamples += juce::String(zone.sampleIndex) + " ";
                continue;
            }

            EblSample decoded;
            juce::String error;
            if (!EblSample::load(sampleFile, decoded, error))
            {
                missingSamples += juce::String(zone.sampleIndex) + " ";
                continue;
            }

            decoded.rootNote = zone.rootNote;
            auto shared = std::make_shared<EblSample>(std::move(decoded));
            sample = loadedSamples.emplace(zone.sampleIndex, std::move(shared)).first;
        }

        sounds.push_back(std::make_unique<EblSamplerSound>(
            sample->second, zone.keyLow, zone.keyHigh,
            zone.velocityLow, zone.velocityHigh, zone.rootNote, zone.voiceParameters));
    }

    if (sounds.empty())
    {
        message = "Could not load any samples for this preset. Missing sample IDs: " + missingSamples;
        return false;
    }

    synth.allNotesOff(1, false);
    synth.clearSounds();
    for (auto& sound : sounds)
        synth.addSound(sound.release());

    previewFile = juce::File();
    previewName = preset.name;
    previewInfo = juce::String(static_cast<int>(sounds.size())) + " zones / MIDI ready";

    message = "Loaded " + preset.name + " with "
            + juce::String(static_cast<int>(sounds.size())) + " zones.";
    if (missingSamples.isNotEmpty())
        message += " Missing sample IDs: " + missingSamples.trim();
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
