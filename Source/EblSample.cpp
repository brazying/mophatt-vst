#include "EblSample.h"
#include <cstring>
#include <cmath>

namespace
{
    bool tagAt(const juce::uint8* data, size_t size, size_t offset, const char* tag)
    {
        return offset + 4 <= size && std::memcmp(data + offset, tag, 4) == 0;
    }

    juce::uint32 readBE32(const juce::uint8* p)
    {
        return (static_cast<juce::uint32>(p[0]) << 24)
             | (static_cast<juce::uint32>(p[1]) << 16)
             | (static_cast<juce::uint32>(p[2]) << 8)
             | static_cast<juce::uint32>(p[3]);
    }

    juce::uint32 readLE32(const juce::uint8* p)
    {
        return static_cast<juce::uint32>(p[0])
             | (static_cast<juce::uint32>(p[1]) << 8)
             | (static_cast<juce::uint32>(p[2]) << 16)
             | (static_cast<juce::uint32>(p[3]) << 24);
    }

    juce::uint16 readLE16(const juce::uint8* p)
    {
        return static_cast<juce::uint16>(p[0])
             | (static_cast<juce::uint16>(p[1]) << 8);
    }

    juce::String readUtf16(const juce::uint8* p, size_t length)
    {
        juce::String value;
        for (size_t i = 0; i + 1 < length; i += 2)
        {
            const auto c = readLE16(p + i);
            if (c == 0)
                break;
            value += juce::String::charToString(static_cast<juce::juce_wchar>(c));
        }
        return value.trim();
    }
}

bool EblSample::load(const juce::File& file, EblSample& result, juce::String& error)
{
    if (!file.existsAsFile())
    {
        error = "EBL file does not exist.";
        return false;
    }

    juce::MemoryBlock bytes;
    if (!file.loadFileAsData(bytes) || bytes.getSize() < 108 + 184)
    {
        error = "Could not read EBL file or file is too small.";
        return false;
    }

    const auto* data = static_cast<const juce::uint8*>(bytes.getData());
    const auto size = bytes.getSize();

    if (!tagAt(data, size, 0, "FORM") || !tagAt(data, size, 8, "E5B0"))
    {
        error = "Not an E-MU Emulator X EBL container.";
        return false;
    }

    // Find TOC2 instead of assuming it is always at byte 12.
    size_t cursor = 12;
    size_t tocOffset = 0;
    size_t tocSize = 0;
    while (cursor + 8 <= size)
    {
        const auto chunkSize = static_cast<size_t>(readBE32(data + cursor + 4));
        if (chunkSize > size || cursor + 8 + chunkSize > size)
            break;
        if (tagAt(data, size, cursor, "TOC2"))
        {
            tocOffset = cursor + 8;
            tocSize = chunkSize;
            break;
        }
        cursor += 8 + chunkSize;
    }

    if (tocOffset == 0 || tocSize < 78)
    {
        error = "EBL table of contents is missing or incomplete.";
        return false;
    }

    size_t sampleChunk = 0;
    for (size_t entry = 0; entry + 78 <= tocSize; entry += 78)
    {
        const auto* toc = data + tocOffset + entry;
        if (std::memcmp(toc, "E5S1", 4) == 0)
        {
            sampleChunk = readBE32(toc + 8);
            break;
        }
    }

    if (sampleChunk == 0 || sampleChunk + 10 > size
        || !tagAt(data, size, sampleChunk, "E5S1"))
    {
        error = "EBL sample chunk was not found.";
        return false;
    }

    const size_t payload = sampleChunk + 10;
    const auto chunkSize = static_cast<size_t>(readBE32(data + sampleChunk + 4));
    if (chunkSize < 2 || payload > size || chunkSize - 2 > size - payload)
    {
        error = "EBL sample chunk has invalid bounds.";
        return false;
    }

    const size_t payloadEnd = payload + chunkSize - 2;
    if (payload + 184 > payloadEnd)
    {
        error = "EBL sample header is incomplete.";
        return false;
    }

    const auto version = readLE16(data + payload + 2);
    if (version != 1 && version != 2)
    {
        error = "Unsupported EBL sample header version.";
        return false;
    }

    const size_t audioOffset = version == 1 ? 184 : 188;
    if (payload + audioOffset > payloadEnd)
    {
        error = "EBL audio data offset is invalid.";
        return false;
    }

    EblSample parsed;
    parsed.name = readUtf16(data + payload + 4, 64);
    parsed.sampleRate = static_cast<int>(readLE32(data + payload + 104));
    if (parsed.sampleRate < 4000 || parsed.sampleRate > 384000)
    {
        error = "EBL sample rate is outside the supported range.";
        return false;
    }

    const int bytesPerSample = data[payload + 115];
    if (bytesPerSample != 2 && bytesPerSample != 4)
    {
        error = "EBL sample uses an unsupported sample width.";
        return false;
    }
    const auto leftStart = static_cast<size_t>(readLE32(data + payload + 72));
    const auto leftEnd = static_cast<size_t>(readLE32(data + payload + 80));
    const auto rightStart = static_cast<size_t>(readLE32(data + payload + 76));
    const auto rightEnd = static_cast<size_t>(readLE32(data + payload + 84));

    if (leftEnd <= leftStart || leftEnd - leftStart > payloadEnd - payload
        || payload + leftEnd > payloadEnd || leftStart < audioOffset)
    {
        error = "EBL left-channel data range is invalid.";
        return false;
    }

    const size_t leftLength = leftEnd - leftStart;
    const size_t rightLength = rightEnd > rightStart ? rightEnd - rightStart : 0;
    const bool stereo = rightLength > 0 && rightStart != leftStart && rightLength == leftLength;

    if (stereo && (rightStart < audioOffset || payload + rightStart > payloadEnd
        || rightLength > payloadEnd - payload - rightStart))
    {
        error = "EBL right-channel data range is invalid.";
        return false;
    }

    if (leftLength % static_cast<size_t>(bytesPerSample) != 0)
    {
        error = "EBL sample data is not aligned to its sample format.";
        return false;
    }

    const int frames = static_cast<int>(leftLength / static_cast<size_t>(bytesPerSample));
    if (frames <= 0 || frames > 100000000)
    {
        error = "EBL sample has an invalid frame count.";
        return false;
    }

    parsed.audio.setSize(stereo ? 2 : 1, frames);
    for (int frame = 0; frame < frames; ++frame)
    {
        const auto readValue = [data, payload, bytesPerSample](size_t channelStart, int index)
        {
            const auto* sample = data + payload + channelStart
                               + static_cast<size_t>(index) * static_cast<size_t>(bytesPerSample);
            if (bytesPerSample == 4)
            {
                const juce::uint32 raw = readLE32(sample);
                float value = 0.0f;
                std::memcpy(&value, &raw, sizeof(value));
                return juce::jlimit(-1.0f, 1.0f, value);
            }

            const auto raw = static_cast<juce::int16>(readLE16(sample));
            return static_cast<float>(raw) / 32768.0f;
        };

        parsed.audio.setSample(0, frame, readValue(leftStart, frame));
        if (stereo)
            parsed.audio.setSample(1, frame, readValue(rightStart, frame));
    }

    parsed.hasLoop = readLE16(data + payload + 110) != 0;
    if (parsed.hasLoop)
    {
        const auto loopStartBytes = readLE32(data + payload + 88);
        const auto loopEndBytes = readLE32(data + payload + 96);
        if (loopStartBytes >= leftStart && loopEndBytes > loopStartBytes)
        {
            parsed.loopStart = static_cast<int>((loopStartBytes - leftStart) / bytesPerSample);
            parsed.loopEnd = static_cast<int>((loopEndBytes - leftStart) / bytesPerSample);
            parsed.hasLoop = parsed.loopStart >= 0 && parsed.loopEnd > parsed.loopStart
                          && parsed.loopEnd < frames;
        }
        else
        {
            parsed.hasLoop = false;
        }
    }

    if (parsed.name.isEmpty())
        parsed.name = file.getFileNameWithoutExtension();

    result = std::move(parsed);
    return true;
}
