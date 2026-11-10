#include "ExbBank.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace
{
    struct Chunk
    {
        juce::String tag;
        size_t dataStart = 0;
        size_t dataEnd = 0;
    };

    bool hasTag(const juce::uint8* bytes, size_t size, size_t offset, const char* tag)
    {
        return offset + 4 <= size && std::memcmp(bytes + offset, tag, 4) == 0;
    }

    juce::uint32 readBE32(const juce::uint8* p)
    {
        return (static_cast<juce::uint32>(p[0]) << 24)
             | (static_cast<juce::uint32>(p[1]) << 16)
             | (static_cast<juce::uint32>(p[2]) << 8)
             | static_cast<juce::uint32>(p[3]);
    }

    float readBEFloat(const juce::uint8* p)
    {
        const juce::uint32 bits = readBE32(p);
        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    juce::uint16 readBE16(const juce::uint8* p)
    {
        return static_cast<juce::uint16>((static_cast<juce::uint16>(p[0]) << 8)
                                       | static_cast<juce::uint16>(p[1]));
    }

    float readBEFloat(const juce::uint8* p)
    {
        const juce::uint32 raw = readBE32(p);
        float value = 0.0f;
        std::memcpy(&value, &raw, sizeof(value));
        return std::isfinite(value) ? value : 0.0f;
    }

    std::vector<Chunk> readChunks(const juce::uint8* data, size_t start, size_t end)
    {
        std::vector<Chunk> chunks;
        size_t cursor = start;

        while (cursor + 8 <= end)
        {
            const size_t declaredSize = static_cast<size_t>(readBE32(data + cursor + 4));
            const size_t payloadStart = cursor + 8;
            const size_t availableSize = end - payloadStart;
            const size_t actualSize = juce::jmin(declaredSize, availableSize);

            chunks.push_back({ juce::String::fromUTF8(reinterpret_cast<const char*>(data + cursor), 4),
                               payloadStart, payloadStart + actualSize });

            if (declaredSize > availableSize)
                break;

            cursor = payloadStart + declaredSize;
        }
        return chunks;
    }

    bool findList(const juce::uint8* data, size_t start, size_t end,
                  const char* listType, size_t& listStart, size_t& listEnd)
    {
        for (const auto& chunk : readChunks(data, start, end))
        {
            if (chunk.tag == "LIST" && chunk.dataStart + 4 <= chunk.dataEnd
                && std::memcmp(data + chunk.dataStart, listType, 4) == 0)
            {
                listStart = chunk.dataStart + 4;
                listEnd = chunk.dataEnd;
                return true;
            }
        }
        return false;
    }

    void readWindows(const juce::uint8* data, size_t start, size_t end,
                     int& keyLow, int& keyHigh, int& velocityLow, int& velocityHigh,
                     bool zoneWindows)
    {
        int windowIndex = 0;
        for (const auto& chunk : readChunks(data, start, end))
        {
            if (chunk.tag != "ETW " || chunk.dataEnd - chunk.dataStart < 8)
                continue;

            const int low = data[chunk.dataStart + 4];
            const int high = data[chunk.dataStart + 7];

            if (windowIndex == 0)
            {
                keyLow = low;
                keyHigh = high;
            }
            else if (windowIndex == 1)
            {
                velocityLow = low;
                velocityHigh = high;
            }

            ++windowIndex;
            if (windowIndex >= (zoneWindows ? 2 : 3))
                break;
        }
    }

    int mapRangeValue(int parentLow, int parentHigh, int childValue)
    {
        return parentLow + static_cast<int>(std::lround(
            static_cast<double>(juce::jlimit(0, 127, childValue)) * (parentHigh - parentLow) / 127.0));
    }

    void copyChunkPayload(const juce::uint8* data, const Chunk& chunk,
                          std::vector<juce::uint8>& destination)
    {
        destination.assign(data + chunk.dataStart, data + chunk.dataEnd);
    }

    void parseVoiceParameters(const juce::uint8* data, size_t start, size_t end,
                              ExbVoiceParameters& parameters)
    {
        int envelopeIndex = 0;

        for (const auto& chunk : readChunks(data, start, end))
        {
            const size_t chunkSize = chunk.dataEnd - chunk.dataStart;

            if (chunk.tag == "E5Fl" && chunkSize >= 13)
            {
                parameters.filter.type = static_cast<int>(data[chunk.dataStart + 4]);
                parameters.filter.frequency = readBEFloat(data + chunk.dataStart + 5);
                parameters.filter.resonance = readBEFloat(data + chunk.dataStart + 9);
                parameters.filter.valid = true;
            }
            else if (chunk.tag == "E5Am" && chunkSize >= 8)
            {
                parameters.amplitude = readBEFloat(data + chunk.dataStart + 4);
                parameters.hasAmplitude = true;
            }
            else if (chunk.tag == "LIST" && chunkSize >= 4)
            {
                const auto* listType = data + chunk.dataStart;
                const size_t listStart = chunk.dataStart + 4;

                if (std::memcmp(listType, "EvL ", 4) == 0)
                {
                    for (const auto& envelopeChunk : readChunks(data, listStart, chunk.dataEnd))
                    {
                        if (envelopeChunk.tag != "E5Ev"
                            || envelopeIndex >= static_cast<int>(parameters.envelopes.size())
                            || envelopeChunk.dataEnd - envelopeChunk.dataStart < 64)
                            continue;

                        auto& envelope = parameters.envelopes[static_cast<size_t>(envelopeIndex++)];
                        for (int stage = 0; stage < 6; ++stage)
                        {
                            const size_t offset = envelopeChunk.dataStart + 10
                                                + static_cast<size_t>(stage) * 9;
                            envelope.stages[static_cast<size_t>(stage)].rate = readBEFloat(data + offset);
                            envelope.stages[static_cast<size_t>(stage)].level = readBEFloat(data + offset + 4);
                        }
                        envelope.valid = true;
                    }
                }
                else if (std::memcmp(listType, "LFOL", 4) == 0)
                {
                    for (const auto& lfoChunk : readChunks(data, listStart, chunk.dataEnd))
                        if (lfoChunk.tag == "E5LF")
                            parameters.lfoData.emplace_back(data + lfoChunk.dataStart,
                                                            data + lfoChunk.dataEnd);
                }
                else if (std::memcmp(listType, "FuGL", 4) == 0)
                {
                    for (const auto& functionChunk : readChunks(data, listStart, chunk.dataEnd))
                        if (functionChunk.tag == "EFGn")
                            parameters.functionGeneratorData.emplace_back(data + functionChunk.dataStart,
                                                                          data + functionChunk.dataEnd);
                }
                else if (std::memcmp(listType, "CrdL", 4) == 0)
                {
                    for (const auto& cordChunk : readChunks(data, listStart, chunk.dataEnd))
                    {
                        if (cordChunk.tag != "E5Cd"
                            || cordChunk.dataEnd - cordChunk.dataStart < 10)
                            continue;

                        ExbModulationCord cord;
                        cord.source = static_cast<int>(data[cordChunk.dataStart + 4]);
                        cord.destination = static_cast<int>(data[cordChunk.dataStart + 5]);
                        cord.amount = readBEFloat(data + cordChunk.dataStart + 6);
                        if (cord.source != 0 || cord.destination != 0 || cord.amount != 0.0f)
                            parameters.modulationCords.push_back(cord);
                    }
                }
            }
        }
    }

    ExbVoiceParameters parseVoiceParameters(const juce::uint8* data, size_t start, size_t end)
    {
        ExbVoiceParameters parameters;
        for (const auto& chunk : readChunks(data, start, end))
        {
            const auto length = chunk.dataEnd - chunk.dataStart;
            if (chunk.tag == "E5Am" && length >= 8)
            {
                parameters.ampGainDb = juce::jlimit(-96.0f, 10.0f, readBEFloat(data + chunk.dataStart + 4));
            }
            else if (chunk.tag == "E5Fl" && length >= 13)
            {
                parameters.filterType = static_cast<int>(data[chunk.dataStart + 4]);
                parameters.filterFrequency = juce::jlimit(0.0f, 1.0f, readBEFloat(data + chunk.dataStart + 5));
                parameters.filterResonance = juce::jlimit(0.0f, 1.0f, readBEFloat(data + chunk.dataStart + 9));
            }
            else if (chunk.tag == "LIST" && chunk.dataStart + 4 <= chunk.dataEnd
                     && std::memcmp(data + chunk.dataStart, "EvL ", 4) == 0)
            {
                int envelopeIndex = 0;
                for (const auto& envelope : readChunks(data, chunk.dataStart + 4, chunk.dataEnd))
                {
                    if (envelope.tag != "E5Ev" || envelope.dataEnd - envelope.dataStart < 64
                        || envelopeIndex >= static_cast<int>(parameters.envelopes.size()))
                        continue;
                    auto& target = parameters.envelopes[static_cast<size_t>(envelopeIndex++)];
                    for (int stage = 0; stage < 6; ++stage)
                    {
                        const size_t offset = envelope.dataStart + 10 + static_cast<size_t>(stage) * 9;
                        target.rates[static_cast<size_t>(stage)] = juce::jlimit(0.0f, 1.0f, readBEFloat(data + offset));
                        target.levels[static_cast<size_t>(stage)] = juce::jlimit(0.0f, 100.0f, readBEFloat(data + offset + 4));
                    }
                    target.valid = true;
                }
            }
            else if (chunk.tag == "LIST" && chunk.dataStart + 4 <= chunk.dataEnd
                     && std::memcmp(data + chunk.dataStart, "CrdL", 4) == 0)
            {
                for (const auto& cord : readChunks(data, chunk.dataStart + 4, chunk.dataEnd))
                {
                    if (cord.tag != "E5Cd" || cord.dataEnd - cord.dataStart < 10)
                        continue;
                    ExbModCord item;
                    item.source = data[cord.dataStart + 4];
                    item.destination = data[cord.dataStart + 5];
                    item.amount = readBEFloat(data + cord.dataStart + 6);
                    if (item.source != 0 || item.destination != 0 || item.amount != 0.0f)
                        parameters.modulationCords.push_back(item);
                }
            }
        }
        return parameters;
    }

    void parseZoneList(const juce::uint8* data, size_t start, size_t end,
                       int voiceKeyLow, int voiceKeyHigh,
                       int voiceVelocityLow, int voiceVelocityHigh,
                       const ExbVoiceParameters& parameters,
                       std::vector<ExbZone>& zones)
    {
        for (const auto& chunk : readChunks(data, start, end))
        {
            if (chunk.tag == "Zhdr" && chunk.dataEnd - chunk.dataStart >= 12)
            {
                ExbZone zone;
                zone.sampleIndex = static_cast<int>(readBE16(data + chunk.dataStart + 4));
                zone.rootNote = juce::jlimit(0, 127, static_cast<int>(data[chunk.dataStart + 10]));
                zone.parameters = parameters;
                size_t zoneWindowsStart = 0;
                size_t zoneWindowsEnd = 0;
                if (findList(data, chunk.dataEnd, end, "TWL ", zoneWindowsStart, zoneWindowsEnd))
                {
                    int zoneKeyLow = 0, zoneKeyHigh = 127;
                    int zoneVelocityLow = 0, zoneVelocityHigh = 127;
                    readWindows(data, zoneWindowsStart, zoneWindowsEnd,
                                zoneKeyLow, zoneKeyHigh, zoneVelocityLow, zoneVelocityHigh, true);
                    zone.keyLow = mapRangeValue(voiceKeyLow, voiceKeyHigh, zoneKeyLow);
                    zone.keyHigh = mapRangeValue(voiceKeyLow, voiceKeyHigh, zoneKeyHigh);
                    zone.velocityLow = mapRangeValue(voiceVelocityLow, voiceVelocityHigh, zoneVelocityLow);
                    zone.velocityHigh = mapRangeValue(voiceVelocityLow, voiceVelocityHigh, zoneVelocityHigh);
                }
                else
                {
                    zone.keyLow = voiceKeyLow;
                    zone.keyHigh = voiceKeyHigh;
                    zone.velocityLow = voiceVelocityLow;
                    zone.velocityHigh = voiceVelocityHigh;
                }
                if (zone.sampleIndex > 0 && zone.keyLow <= zone.keyHigh
                    && zone.velocityLow <= zone.velocityHigh)
                    zones.push_back(std::move(zone));
            }
        }
    }

    void parseVoice(const juce::uint8* data, size_t start, size_t end,
                    std::vector<ExbZone>& zones)
    {
        int keyLow = 0, keyHigh = 127;
        int velocityLow = 0, velocityHigh = 127;
        size_t voiceWindowsStart = 0;
        size_t voiceWindowsEnd = 0;
        if (findList(data, start, end, "TWL ", voiceWindowsStart, voiceWindowsEnd))
            readWindows(data, voiceWindowsStart, voiceWindowsEnd,
                        keyLow, keyHigh, velocityLow, velocityHigh, false);

        const auto parameters = parseVoiceParameters(data, start, end);
        size_t zoneListStart = 0;
        size_t zoneListEnd = 0;
        if (findList(data, start, end, "E5ZL", zoneListStart, zoneListEnd))
            parseZoneList(data, zoneListStart, zoneListEnd,
                          keyLow, keyHigh, velocityLow, velocityHigh, parameters, zones);
    }
}

juce::String ExbBank::readUtf16Le(const juce::uint8* bytes, size_t byteCount)
{
    juce::String result;
    for (size_t i = 0; i + 1 < byteCount; i += 2)
    {
        const juce::uint16 codeUnit = static_cast<juce::uint16>(bytes[i])
                                    | (static_cast<juce::uint16>(bytes[i + 1]) << 8);
        if (codeUnit == 0)
            break;
        result += juce::String::charToString(static_cast<juce::juce_wchar>(codeUnit));
    }
    return result.trim();
}

ExbBankInfo ExbBank::inspect(const juce::File& file)
{
    ExbBankInfo info;
    info.file = file;
    info.bankName = file.getFileNameWithoutExtension();

    if (!file.existsAsFile())
    {
        info.error = "File does not exist.";
        return info;
    }

    juce::MemoryBlock memory;
    if (!file.loadFileAsData(memory))
    {
        info.error = "Could not read the bank file.";
        return info;
    }

    const auto* data = static_cast<const juce::uint8*>(memory.getData());
    const size_t size = memory.getSize();
    if (size < 20 || !hasTag(data, size, 0, "FORM") || !hasTag(data, size, 8, "E5B0"))
    {
        info.error = "Not an E-MU Emulator X EXB bank.";
        return info;
    }

    size_t cursor = 12;
    size_t tocStart = 0;
    size_t tocSize = 0;
    while (cursor + 8 <= size)
    {
        const size_t chunkSize = static_cast<size_t>(readBE32(data + cursor + 4));
        if (chunkSize > size || cursor + 8 + chunkSize > size)
            break;

        if (hasTag(data, size, cursor, "TOC2"))
        {
            tocStart = cursor + 8;
            tocSize = chunkSize;
            break;
        }
        cursor += 8 + chunkSize;
    }

    if (tocStart == 0 || tocSize < 78 || tocSize % 78 != 0)
    {
        info.error = "EXB bank has an invalid TOC2 table.";
        return info;
    }

    for (size_t offset = 0; offset + 78 <= tocSize; offset += 78)
    {
        const auto* entry = data + tocStart + offset;
        const int index = static_cast<int>(readBE16(entry + 12));
        const auto name = readUtf16Le(entry + 14, 64);

        if (std::memcmp(entry, "E5P1", 4) == 0)
        {
            ExbPreset preset;
            preset.name = name;
            preset.number = index;

            const size_t chunkOffset = static_cast<size_t>(readBE32(entry + 8));
            const size_t chunkSize = static_cast<size_t>(readBE32(entry + 4));
            if (chunkOffset + 10 <= size && chunkSize >= 2
                && chunkOffset + 8 + chunkSize <= size)
                parsePreset(data, chunkOffset + 10, chunkOffset + 8 + chunkSize, preset);

            if (preset.name.isNotEmpty())
                info.presets.push_back(std::move(preset));
        }
        else if (std::memcmp(entry, "E5SL", 4) == 0 || std::memcmp(entry, "EXSL", 4) == 0)
        {
            ++info.sampleCount;
        }
    }

    if (info.presets.empty())
        info.error = "EXB bank is valid, but no preset entries were found in TOC2.";

    return info;
}
