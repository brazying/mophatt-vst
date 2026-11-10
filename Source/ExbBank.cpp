#include "ExbBank.h"

namespace
{
    juce::uint32 readBigEndian32(const juce::uint8* p)
    {
        return (static_cast<juce::uint32>(p[0]) << 24)
             | (static_cast<juce::uint32>(p[1]) << 16)
             | (static_cast<juce::uint32>(p[2]) << 8)
             |  static_cast<juce::uint32>(p[3]);
    }

    bool hasTag(const juce::MemoryBlock& data, size_t offset, const char* tag)
    {
        return offset + 4 <= data.getSize()
            && std::memcmp(static_cast<const juce::uint8*>(data.getData()) + offset, tag, 4) == 0;
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

        if (codeUnit >= 0x20 && codeUnit != 0x7f)
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

    if (data.getSize() < 20 || !hasTag(data, 0, "FORM") || !hasTag(data, 12, "TOC2"))
    {
        info.error = "Not a supported Emulator X EXB container (expected FORM / E5B0 / TOC2).";
        return info;
    }

    const auto* bytes = static_cast<const juce::uint8*>(data.getData());
    if (std::memcmp(bytes + 8, "E5B0", 4) != 0)
    {
        info.error = "Unsupported E-MU bank generation. This build expects E5B0 / EXB.";
        return info;
    }

    // EXB preset records use E5P1 chunks. The preset label is stored as
    // a zero-terminated UTF-16LE string beginning 14 bytes after the tag.
    // Chunk sizes are big-endian. We validate bounds before reading names.
    size_t cursor = 16;
    while (cursor + 8 <= data.getSize())
    {
        if (!hasTag(data, cursor, "E5P1"))
        {
            ++cursor;
            continue;
        }

        const auto chunkSize = static_cast<size_t>(readBigEndian32(bytes + cursor + 4));
        if (chunkSize < 8 || cursor + 8 + chunkSize > data.getSize())
        {
            ++cursor;
            continue;
        }

        const size_t nameOffset = cursor + 14;
        const size_t nameBytes = juce::jmin<size_t>(64, data.getSize() - nameOffset);
        auto name = readUtf16Le(bytes + nameOffset, nameBytes);

        if (name.isNotEmpty())
        {
            const bool duplicate = std::any_of(info.presets.begin(), info.presets.end(),
                [&name](const ExbPreset& preset) { return preset.name == name; });

            if (!duplicate)
                info.presets.push_back({ name, static_cast<int>(info.presets.size()) + 1 });
        }

        cursor += 8 + chunkSize;
    }

    if (info.presets.empty())
        info.error = "EXB container recognized, but no preset names were found by the initial parser.";

    return info;
}
