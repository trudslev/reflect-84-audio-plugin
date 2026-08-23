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

    /** Call once per sample per LOOP POSITION, from inside the feedback loop. Positions must be
        visited in order 0..n-1 within a sample, because the hold counter advances on the last one.

        **A position is a delay line, not an audio channel, and conflating the two made Chamber
        diverge.** `FdnTank` called this as `process (i % 2, ...)` with `i` the LINE index, so an
        FDN-4's lines 0 and 2 shared one held slot and 1 and 3 shared the other. During a hold each
        pair returns the SAME value, collapsing four independent state variables onto two identical
        ones - and an orthogonal mixing matrix summing two copies of one signal adds them
        coherently, so the energy bound that makes the loop stable stops applying. The counter also
        advanced twice per sample, four times for Hall, running the hold at the wrong rate.

        Measured before the fix, Chamber at QUIET VIOLENCE's values with dampLF 120: grain 0.00 and
        0.05 stable, 0.15 / 0.35 / 0.60 all diverging past 1e35. Both stable ends are exactly the
        cases where no sharing can occur - grain 0 is a passthrough, and 0.05 has holdPeriod 1, so
        every position captures its own input on every sample and never reads a neighbour's. */
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
    std::array<float, 8> held {};   // one per loop position; FDN-8 is the widest consumer
    int channels = 2;
    int holdPeriod = 1;
    int counter = 0;
};
