#pragma once

#include "DigitalRoomTank.h"
#include "FdnTank.h"
#include "PlateTank.h"
#include "ReverbTank.h"

#include <memory>

/**
    Owns all four tanks, routes the wet path through the selected one, and crossfades on a switch.

    Switching algorithm swaps one whole network for another, which cannot be click-free by
    construction - the two have unrelated internal states. Gatecrasher's ReverbEngine solves it by
    running the outgoing tank on a COPY of this block's input, swapping, then blending from that
    snapshot into the new tank's output; the same approach is used here. The alternative, muting
    across the switch, is audible as a hole.

    Pre-delay lives here too, ahead of every tank, because it is part of the wet path's geometry
    rather than an output control.
*/
class ReverbEngine
{
public:
    ReverbEngine();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer,
                  int algorithmIndex,
                  const TankParameters& params,
                  float preDelayMs);

    /** 0-1 energy still circulating in the active tank, for the TANK LIVE lamp. */
    float getEnergy() const noexcept;

    static constexpr double switchCrossfadeSeconds = 0.06;
    static constexpr float maxPreDelayMs = 180.0f;

private:
    ReverbTank* tankFor (int index) noexcept;

    std::array<std::unique_ptr<ReverbTank>, 4> tanks;

    juce::dsp::DelayLine<float> preDelayLine { 1 };
    juce::AudioBuffer<float> previousTankOutput;
    juce::SmoothedValue<float> switchCrossfade;
    juce::SmoothedValue<float> preDelaySmoothed;

    double sampleRate = 44100.0;
    int currentAlgorithm = 0;
};
