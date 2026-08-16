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

    /*  **`initialPreDelayMs` is an argument, because `reset (rate, seconds)` does not set a value.**

        It is `setCurrentAndTargetValue (this->target)` internally — it sets the ramp LENGTH and
        snaps to whatever target the smoother last held, which is zero on a constructed object. So
        `preDelaySmoothed` glided up from zero across an instance's first render and never again:
        the first-run-only defect this casting's own premise check has to warm past, measured at
        0.392414443 first at sample 351.

        **`switchCrossfade` two lines above it was guarded all along**, with a literal 1.0. Nobody
        misunderstood the API — the correct form was written adjacent to the incorrect one — which is
        why a rule saying "know what reset does" would not have caught this and a grep did. */
    /*  **`initialAlgorithm` is an argument for the same reason `initialPreDelayMs` is, and this
        one was CORRECT BY COINCIDENCE rather than by design.**

        `currentAlgorithm` is constructed to 0 and compared per block against what the parameter
        asks for; a difference starts a ~60 ms crossfade out of the outgoing tank. Nothing reconciled
        it at prepare, and nothing needed to — because this casting's default Program selects
        algorithm 0 as well. **The line that made it correct was not in this class at all: it was a
        value in the Program bank.** That is the answer that defines correct-by-coincidence, and any
        edit to that Program's algorithm arms it.

        Measured while still armed, cold against warmed with pre-delay held at zero so the smoother
        finding could not be mistaken for it: algorithm 0 sample-exact, and the other three at
        **0.178 / 0.293 / 0.132**, all first at sample 351 — the wet arrival. Gatecrasher's live case
        measured 0.041, so this was latently four to seven times larger. */
    void prepare (const juce::dsp::ProcessSpec& spec, float initialPreDelayMs, int initialAlgorithm);
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
