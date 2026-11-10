#pragma once

#include <JuceHeader.h>
#include "EblSample.h"
#include <memory>
#include <utility>

class EblSamplerSound : public juce::SynthesiserSound
{
public:
    EblSamplerSound(std::shared_ptr<EblSample> dataIn,
                    int keyLowIn = 0, int keyHighIn = 127,
                    int velocityLowIn = 0, int velocityHighIn = 127,
                    int rootNoteIn = 60)
        : data(std::move(dataIn)),
          keyLow(keyLowIn), keyHigh(keyHighIn),
          velocityLow(velocityLowIn), velocityHigh(velocityHighIn),
          rootNote(rootNoteIn) {}

    bool appliesToNote(int midiNoteNumber) override
    {
        return midiNoteNumber >= keyLow && midiNoteNumber <= keyHigh;
    }

    bool appliesToChannel(int) override { return true; }

    std::shared_ptr<EblSample> data;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 0;
    int velocityHigh = 127;
    int rootNote = 60;
};
