#pragma once

#include "GrainStage.h"
#include "ReverbPrimitives.h"
#include "ReverbTank.h"

/**
    PLATE - a Dattorro-style figure-of-eight.

    No early reflections at all: four series allpasses smear the input into a continuous wash and
    feed it straight into the tank. That is what makes a plate a plate - it is dense from the first
    millisecond and has no sense of a room, because a sheet of steel has no walls to reflect off.

    The tank is a single loop crossing between two halves, each half being a modulated allpass, a
    delay, in-loop damping, a second allpass and another delay. Output is taken from seven fixed
    taps spread across both halves, which is where the stereo image comes from - the two channels
    are two different views of one shared network, not two independent reverbs.
*/
class PlateTank final : public ReverbTank
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer, const TankParameters& p) override;
    float getEnergy() const noexcept override { return energy; }

private:
    double sampleRate = 44100.0;

    // Input diffusion: 4 series allpasses, no modulation.
    std::array<ReverbPrimitives::Allpass, 4> inputDiffuser;

    // The figure-of-eight. Index 0 is the "left" half, 1 the "right".
    std::array<ReverbPrimitives::Allpass, 2> modulatedAllpass;
    std::array<ReverbPrimitives::Allpass, 2> fixedAllpass;
    std::array<ReverbPrimitives::Delay, 2> delayA;
    std::array<ReverbPrimitives::Delay, 2> delayB;
    std::array<ReverbPrimitives::OnePoleLP, 2> damping;
    std::array<ReverbPrimitives::OnePoleHP, 2> lowCut;

    ReverbPrimitives::LfoBank<2> lfo;
    GrainStage grain;

    std::array<float, 2> tankState {};
    float energy = 0.0f;
};
