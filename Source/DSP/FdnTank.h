#pragma once

#include "GrainStage.h"
#include "ReverbPrimitives.h"
#include "ReverbTank.h"

#include <vector>

/**
    A feedback delay network: N delay lines whose outputs are mixed by an orthogonal matrix and fed
    back into their own inputs.

    This is the one piece two algorithms share, and they share it because a chamber and a hall are
    genuinely the same KIND of network - a set of coupled paths around an enclosed volume - at
    different scales. What differs is not a coefficient table but the network itself: CHAMBER is
    order 4 with Householder mixing, HALL is order 8 with Hadamard, and they carry different early
    reflection patterns, delay ranges, diffusion and modulation depth.

    An orthogonal mixing matrix is what makes an FDN worth the trouble over parallel combs: energy
    moves between lines without being created or destroyed, so the echo density keeps rising as the
    tail develops instead of staying at whatever the line count set it to. Combs cannot do that -
    they never talk to each other.

    Damping sits inside each line's feedback path, not on the output, so high frequencies decay
    faster than lows the way they do in a real room.
*/
class FdnTank : public ReverbTank
{
public:
    static constexpr int maxLines = 8;

    struct Config
    {
        int numLines = 4;
        bool hadamardMixing = false;            // false = Householder

        std::vector<float> lineMs;              // one per line, mutually prime-ish
        std::vector<float> inputDiffusionMs;    // series allpasses before the network

        float diffusionBase = 0.4f;             // allpass feedback at DENSITY 0
        float diffusionRange = 0.3f;            // added at DENSITY 1

        float modDepthMs = 1.2f;

        std::vector<ReverbPrimitives::EarlyReflections::Tap> taps;
        float earlyGain = 0.5f;
        float maxEarlyMs = 100.0f;

        int seed = 0;
    };

    explicit FdnTank (Config config);

    void prepare (const juce::dsp::ProcessSpec& spec) override;
    void reset() override;
    void process (juce::AudioBuffer<float>& buffer, const TankParameters& p) override;
    float getEnergy() const noexcept override { return energy; }

private:
    void mix (std::array<float, maxLines>& v) const noexcept;

    Config cfg;
    double sampleRate = 44100.0;

    ReverbPrimitives::EarlyReflections early;
    std::vector<ReverbPrimitives::Allpass> inputDiffuser;

    std::array<ReverbPrimitives::Delay, maxLines> lines;
    std::array<ReverbPrimitives::OnePoleLP, maxLines> damping;
    std::array<ReverbPrimitives::OnePoleHP, maxLines> lowCut;
    std::array<float, maxLines> feedbackState {};

    ReverbPrimitives::LfoBank<maxLines> lfo;
    GrainStage grain;

    float energy = 0.0f;
};

//==============================================================================
/** CHAMBER - order 4, Householder mixing, mutually-prime lines of 23-45 ms.

    An irregular medium space: the early pattern is asymmetric and diffused rather than sparse, so
    it builds more slowly than a plate but fuses sooner than a hall. Moderate diffusion and warm
    damping. */
class ChamberTank final : public FdnTank
{
public:
    ChamberTank();
};

/** HALL - order 8, Hadamard mixing, long lines of 45-130 ms.

    A wide sparse early pattern with a real gap before the late tail arrives, which is what makes a
    hall read as large: the ear hears the first reflections, then a pause, then the reverb. Strongest
    modulation of the four, because an eight-line network at an eight-second RT60 will ring
    metallically without it. */
class HallTank final : public FdnTank
{
public:
    HallTank();
};
