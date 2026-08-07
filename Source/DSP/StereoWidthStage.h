#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/**
    Mid/side width on the wet path, 0-200%.

    Stateless and unsmoothed by itself - the processor hands it an already-smoothed width - so it
    is just the M/S identity applied per sample. At 100% it is bit-transparent, which matters:
    a width control that colours the signal at its neutral setting is a bug, not a feature.
*/
struct StereoWidthStage
{
    /** Takes the smoother rather than a value so width automation ramps per sample. If the buffer
        is mono there is no side signal to scale, but the smoother is still advanced - leaving it
        stalled would make it jump the moment a stereo buffer arrived. */
    static void process (juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& width) noexcept
    {
        const int numSamples = buffer.getNumSamples();

        if (buffer.getNumChannels() < 2)
        {
            width.skip (numSamples);
            return;
        }

        auto* left = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            const float w = juce::jlimit (0.0f, 2.0f, width.getNextValue());

            const float mid = (left[n] + right[n]) * 0.5f;
            const float side = (left[n] - right[n]) * 0.5f * w;

            left[n] = mid + side;
            right[n] = mid - side;
        }
    }
};
