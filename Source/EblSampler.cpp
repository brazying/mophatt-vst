#include "EblSampler.h"
#include <cmath>

bool EblSamplerVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<EblSamplerSound*>(sound) != nullptr;
}

void EblSamplerVoice::updatePlaybackRatio()
{
    // Emulator X presets can use pitch bend; use the standard +/- 2 semitone range.
    const double bend = (static_cast<double>(pitchWheel) - 8192.0) / 8192.0;
    playbackRatio = basePlaybackRatio * std::pow(2.0, (bend * 2.0) / 12.0);
}

void EblSamplerVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
                                int currentPitchWheelPosition)
{
    auto* eblSound = dynamic_cast<EblSamplerSound*>(sound);
    if (eblSound == nullptr || eblSound->data == nullptr
        || velocity * 127.0f < eblSound->velocityLow
        || velocity * 127.0f > eblSound->velocityHigh)
    {
        clearCurrentNote();
        sample.reset();
        return;
    }

    sample = eblSound->data;
    position = 0.0;
    noteLevel = velocity;
    attackLevel = 0.0f;
    attackSamplesElapsed = 0;
    attackSamplesTotal = juce::jmax(1, static_cast<int>(getSampleRate() * 0.003));
    releasing = false;
    releaseSamplesRemaining = 0;
    releaseSamplesTotal = juce::jmax(1, static_cast<int>(getSampleRate() * 0.12));
    pitchWheel = juce::jlimit(0, 16383, currentPitchWheelPosition);

    const double semitones = static_cast<double>(midiNoteNumber - eblSound->rootNote);
    basePlaybackRatio = std::pow(2.0, semitones / 12.0)
                      * static_cast<double>(sample->sampleRate) / getSampleRate();
    updatePlaybackRatio();
}

void EblSamplerVoice::stopNote(float, bool allowTailOff)
{
    if (!allowTailOff)
    {
        clearCurrentNote();
        sample.reset();
        return;
    }

    if (!releasing)
    {
        releasing = true;
        releaseSamplesRemaining = releaseSamplesTotal;
    }
}

void EblSamplerVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchWheel = juce::jlimit(0, 16383, newPitchWheelValue);
    updatePlaybackRatio();
}

void EblSamplerVoice::controllerMoved(int, int) {}

void EblSamplerVoice::renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    if (sample == nullptr || sample->audio.getNumSamples() == 0)
        return;

    const auto& source = sample->audio;
    const int sourceFrames = source.getNumSamples();
    const int sourceChannels = source.getNumChannels();
    const int outputChannels = output.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        if (sample->hasLoop && !releasing && position >= sample->loopEnd)
        {
            const int loopLength = juce::jmax(1, sample->loopEnd - sample->loopStart);
            position = static_cast<double>(sample->loopStart)
                     + std::fmod(position - sample->loopStart, static_cast<double>(loopLength));
        }

        if (position >= sourceFrames)
        {
            clearCurrentNote();
            sample.reset();
            break;
        }

        const int index = static_cast<int>(position);
        const float fraction = static_cast<float>(position - index);

        // Four-point Hermite interpolation reduces aliasing and roughness compared with
        // the previous linear interpolator, especially when samples are transposed.
        const int i0 = juce::jlimit(0, sourceFrames - 1, index - 1);
        const int i1 = juce::jlimit(0, sourceFrames - 1, index);
        const int i2 = juce::jlimit(0, sourceFrames - 1, index + 1);
        const int i3 = juce::jlimit(0, sourceFrames - 1, index + 2);

        float envelope = 1.0f;
        if (!releasing && attackSamplesElapsed < attackSamplesTotal)
        {
            attackLevel = static_cast<float>(++attackSamplesElapsed)
                        / static_cast<float>(attackSamplesTotal);
            envelope = attackLevel;
        }
        else if (!releasing)
        {
            envelope = 1.0f;
        }
        else
        {
            envelope = static_cast<float>(releaseSamplesRemaining)
                     / static_cast<float>(releaseSamplesTotal);
            if (--releaseSamplesRemaining <= 0)
            {
                clearCurrentNote();
                sample.reset();
                break;
            }
        }

        const float gain = noteLevel * juce::jlimit(0.0f, 1.0f, envelope);
        for (int channel = 0; channel < outputChannels; ++channel)
        {
            const int sourceChannel = sourceChannels == 1 ? 0 : juce::jmin(channel, sourceChannels - 1);
            const float y0 = source.getSample(sourceChannel, i0);
            const float y1 = source.getSample(sourceChannel, i1);
            const float y2 = source.getSample(sourceChannel, i2);
            const float y3 = source.getSample(sourceChannel, i3);

            const float c0 = y1;
            const float c1 = 0.5f * (y2 - y0);
            const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            const float value = ((c3 * fraction + c2) * fraction + c1) * fraction + c0;
            output.addSample(channel, startSample + i, value * gain);
        }

        position += playbackRatio;
    }
}
