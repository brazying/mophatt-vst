#include "EblSampler.h"
#include <cmath>

bool EblSamplerVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<EblSamplerSound*>(sound) != nullptr;
}

void EblSamplerVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
                                int)
{
    auto* eblSound = dynamic_cast<EblSamplerSound*>(sound);
    if (eblSound == nullptr || eblSound->data == nullptr)
        return;

    sample = eblSound->data;
    position = 0.0;
    noteLevel = velocity;
    releasing = false;
    releaseSamplesRemaining = 0;
    releaseSamplesTotal = juce::jmax(1, static_cast<int>(getSampleRate() * 0.12));

    const double semitones = static_cast<double>(midiNoteNumber - sample->rootNote);
    playbackRatio = std::pow(2.0, semitones / 12.0)
                  * static_cast<double>(sample->sampleRate) / getSampleRate();
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

void EblSamplerVoice::pitchWheelMoved(int) {}
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
            position = static_cast<double>(sample->loopStart)
                     + std::fmod(position - sample->loopStart,
                                 static_cast<double>(juce::jmax(1, sample->loopEnd - sample->loopStart)));

        if (position >= sourceFrames)
        {
            clearCurrentNote();
            sample.reset();
            break;
        }

        const int indexA = juce::jlimit(0, sourceFrames - 1, static_cast<int>(position));
        const int indexB = juce::jmin(indexA + 1, sourceFrames - 1);
        const float fraction = static_cast<float>(position - indexA);

        float envelope = 1.0f;
        if (releasing)
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
            const float a = source.getSample(sourceChannel, indexA);
            const float b = source.getSample(sourceChannel, indexB);
            const float value = a + (b - a) * fraction;
            output.addSample(channel, startSample + i, value * gain);
        }

        position += playbackRatio;
    }
}
