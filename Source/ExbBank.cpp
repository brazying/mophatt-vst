#include "ExbBank.h"
#include <algorithm>
#include <cstring>

namespace
{
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

    juce::uint16 readBE16(const juce::uint8* p)
    {
        return static_cast<juce::uint16>((static_cast<juce::uint16>(p[0]) << 8)
                                       | static_cast<juce::uint16>(p[1]));
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

    juce::MemoryBlock data;
    if (!file.loadFileAsData(data))
    {
        info.error = "Could not read the bank file.";
        return info;
    }

    const auto* bytes = static_cast<const juce::uint8*>(data.getData());
    const size_t size = data.getSize();
    if (size < 20 || !hasTag(bytes, size, 0, "FORM") || !hasTag(bytes, size, 8, "E5B0"))
    {
        info.error = "Not an E-MU Emulator X EXB bank.";
        return info;
    }

    // TOC2 is usually the first top-level chunk, but some banks place INFO before it.
    size_t cursor = 12;
    size_t tocStart = 0;
    size_t tocSize = 0;
    while (cursor + 8 <= size)
    {
        const size_t chunkSize = static_cast<size_t>(readBE32(bytes + cursor + 4));
        if (chunkSize > size || cursor + 8 + chunkSize > size)
            break;

        if (hasTag(bytes, size, cursor, "TOC2"))
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
        const auto* entry = bytes + tocStart + offset;
        const int index = static_cast<int>(readBE16(entry + 12));
        const auto name = readUtf16Le(entry + 14, 64);

        if (std::memcmp(entry, "E5P1", 4) == 0)
        {
            if (name.isNotEmpty())
                info.presets.push_back({ name, index });
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
