#pragma once

#include "GrainSpec.h"

#include <array>

/**
    DIGITAL GRAIN, in the audio domain.

    Two artefacts of a 1980s fixed-point reverb, both driven from GrainSpec so the TANK LIVE scope
    and the tank are reading the same number:

      - truncation to a short word length (~11.4 bits at the threshold down to ~8.6 at full)
      - sample-and-hold decimation of the tank's update rate (1x to 9x), with NO anti-alias
        filtering, so the fold-back is part of the sound rather than something to be cleaned up

    This belongs INSIDE the tank's feedback path, called once per recirculation. That is the whole
    point: the truncation compounds, each pass quantising what the previous pass already quantised,
    so the decay envelope itself breaks into steps instead of the output merely sounding gritty.
    Put it on the output and the plugin still makes a noise, but the scope stops telling the truth.
*/
class GrainStage
{
public:
    void prepare (int numChannels)
    {
        channels = juce::jlimit (1, (int) held.size(), numChannels);
        reset();
    }

    void reset() noexcept
    {
        held.fill (0.0f);
        counter = 0;
    }

    /** Call once per sample per channel, from inside the feedback loop. Channels must be visited
        in order 0..n-1 within a sample, because the hold counter advances on the last one. */
    float process (int channel, float x) noexcept
    {
        if (! spec.active)
            return x;

        auto& value = held[(size_t) channel];

        if (counter == 0)
            value = spec.quantize (x);

        if (channel == channels - 1)
            if (++counter >= holdPeriod)
                counter = 0;

        return value;
    }

    void setGrain (float normalisedGrain) noexcept
    {
        spec = GrainSpec::fromNormalized (normalisedGrain);
        holdPeriod = spec.holdSamples();

        if (! spec.active)
            counter = 0;
    }

    const GrainSpec& getSpec() const noexcept { return spec; }

private:
    GrainSpec spec {};
    std::array<float, 2> held {};
    int channels = 2;
    int holdPeriod = 1;
    int counter = 0;
};
