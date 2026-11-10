#include "ExbBank.h"
#include <algorithm>
#include <cstring>

namespace
{
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

    // In E5B0/TOC2 banks, the table contains repeated E5P1 records.
    // Their names are zero-terminated UTF-16LE strings at record offset + 14.
    // The size field in this table is not a conventional enclosing chunk size,
    // so scan for each record marker rather than jumping by that field.
    for (size_t cursor = 16; cursor + 78 <= data.getSize(); ++cursor)
    {
        if (!hasTag(data, cursor, "E5P1"))
            continue;

        const auto name = readUtf16Le(bytes + cursor + 14, 64);
        if (name.isEmpty())
            continue;

        const bool duplicate = std::any_of(info.presets.begin(), info.presets.end(),
            [&name](const ExbPreset& preset) { return preset.name == name; });

        if (!duplicate)
            info.presets.push_back({ name, static_cast<int>(info.presets.size()) + 1 });
    }

    if (info.presets.empty())
        info.error = "EXB container recognized, but no preset names were found.";

    return info;
}
