#pragma once

#include "GrainStage.h"
#include "ReverbPrimitives.h"
#include "ReverbTank.h"

/**
    DIGITAL ROOM - the 1984 rack unit.

    Sparse early reflections read at WHOLE SAMPLE positions, with no interpolation, feeding a
    Moorer/Schroeder network of four parallel damped combs into two series allpasses. Short,
    mutually-prime delays and low diffusion, so at low DENSITY the individual repeats stay audible
    rather than fusing.

    The coarseness is the point, not a shortcut. A period-correct digital room ran a tapped delay
    at a fixed clock and had no fractional interpolator to smooth between taps; that is what makes
    it read as a machine rather than a space. It is also the algorithm DIGITAL GRAIN belongs to -
    at full travel the tank is running at a fraction of the host sample rate with an 8-bit word,
    which on this topology sounds like the machine it is imitating rather than like damage.
*/
class DigitalRoomTank final : public ReverbTank
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer, const TankParameters& p) override;
    float getEnergy() const noexcept override { return energy; }

private:
    static constexpr int numCombs = 4;
    static constexpr int numAllpasses = 2;

    double sampleRate = 44100.0;

    ReverbPrimitives::EarlyReflections early;
    std::vector<ReverbPrimitives::EarlyReflections::Tap> taps;

    std::array<std::array<ReverbPrimitives::DampedComb, numCombs>, 2> combs;
    std::array<std::array<ReverbPrimitives::Allpass, numAllpasses>, 2> allpasses;
    std::array<ReverbPrimitives::OnePoleHP, 2> lowCut;

    ReverbPrimitives::LfoBank<numCombs> lfo;
    GrainStage grain;

    float energy = 0.0f;
};
