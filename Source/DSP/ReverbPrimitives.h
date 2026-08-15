#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cmath>
#include <vector>

/**
    The shared toolkit the four tank topologies are built from.

    Header-only and deliberately small: each piece does one thing, holds its own state, and knows
    nothing about parameters, the APVTS, or which algorithm is using it. The topologies differ by
    how they wire these together, not by flags inside them.
*/
namespace ReverbPrimitives
{

/** The house seeding convention, so two instances of anything random decorrelate rather than
    phase-locking. Same golden-ratio constant TapeRot uses. */
inline juce::int64 seedFor (int index) noexcept
{
    return (juce::int64) (0x9E3779B97F4A7C15ULL * (juce::uint64) (index + 1));
}

//==============================================================================
/** One-pole low-pass. Hand-rolled rather than juce::dsp::IIR because the cutoff moves with the
    DAMPING HF knob every block, and IIR::Coefficients::makeLowPass heap-allocates per call. */
struct OnePoleLP
{
    float state = 0.0f;
    float coeff = 1.0f;

    void setCutoff (float hz, double sampleRate) noexcept
    {
        coeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / (float) sampleRate);
        coeff = juce::jlimit (0.0f, 1.0f, coeff);
    }

    void reset() noexcept { state = 0.0f; }

    float process (float x) noexcept
    {
        state += coeff * (x - state);
        return state;
    }
};

/** One-pole high-pass, as input-minus-lowpass. */
struct OnePoleHP
{
    OnePoleLP lp;

    void setCutoff (float hz, double sampleRate) noexcept { lp.setCutoff (hz, sampleRate); }
    void reset() noexcept { lp.reset(); }
    float process (float x) noexcept { return x - lp.process (x); }
};

//==============================================================================
/** A fixed-length delay with a fractional read tap.

    Allocated once at its maximum length; SIZE and modulation move the read offset rather than
    resizing anything, so neither can allocate or click on the audio thread. Linear interpolation
    is enough here - the modulation depths involved are a couple of milliseconds at sub-Hz rates,
    well below where the interpolator's own high-frequency loss becomes audible in a reverb tail.
*/
class Delay
{
public:
    void prepare (int maxSamples)
    {
        buffer.assign ((size_t) juce::jmax (4, maxSamples + 4), 0.0f);
        writeIndex = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    void write (float x) noexcept
    {
        buffer[(size_t) writeIndex] = x;

        if (++writeIndex >= (int) buffer.size())
            writeIndex = 0;
    }

    float readAt (float delaySamples) const noexcept
    {
        const int size = (int) buffer.size();
        const float clamped = juce::jlimit (1.0f, (float) size - 2.0f, delaySamples);

        float pos = (float) writeIndex - clamped;

        while (pos < 0.0f)
            pos += (float) size;

        const int i0 = (int) pos;
        const int i1 = i0 + 1 >= size ? 0 : i0 + 1;
        const float frac = pos - (float) i0;

        return buffer[(size_t) i0] + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);
    }

    int capacity() const noexcept { return (int) buffer.size(); }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
};

//==============================================================================
/** Schroeder allpass with an optionally modulated delay length - the diffusion workhorse. */
class Allpass
{
public:
    void prepare (int maxSamples) { delay.prepare (maxSamples); }
    void reset() noexcept { delay.reset(); }

    float process (float x, float delaySamples, float feedback) noexcept
    {
        const float delayed = delay.readAt (delaySamples);
        const float out = -x * feedback + delayed;
        delay.write (x + out * feedback);
        return out;
    }

private:
    Delay delay;
};

//==============================================================================
/** Feedback comb with damping inside the loop, so high frequencies decay faster than lows - which
    is what damping physically is, rather than a tone control on the output. */
class DampedComb
{
public:
    void prepare (int maxSamples)
    {
        delay.prepare (maxSamples);
        lp.reset();
    }

    void reset() noexcept { delay.reset(); lp.reset(); }
    void setDamping (float hz, double sampleRate) noexcept { lp.setCutoff (hz, sampleRate); }

    float process (float x, float delaySamples, float feedback) noexcept
    {
        const float delayed = delay.readAt (delaySamples);
        delay.write (x + lp.process (delayed) * feedback);
        return delayed;
    }

private:
    Delay delay;
    OnePoleLP lp;
};

//==============================================================================
/** A bank of decorrelated LFOs for the tank's internal modulation.

    Rates are mutually inharmonic and each one random-walks slowly around its nominal value. Both
    matter on a long tail: fixed, related rates make the modulation audible as a periodic chorus
    rather than as the tank simply refusing to ring.
*/
template <int N>
class LfoBank
{
public:
    /** How often the bank re-targets and re-phases, in HERTZ rather than in samples.

        **Both figures here were CHOSEN, not derived, and they are named in their own units so that
        stays visible.** The defect this replaced was `0.02f` applied once per block — a time
        constant whose value in seconds was whatever buffer the host happened to hand over. Making it
        time-based is half the fix; making the figure legible as time is the other half, because an
        interval in samples would have been the identical defect one axis over.

        **1 kHz.** Above today's best case, so no buffer size gets worse: the current update rate IS
        the buffer rate, 750 Hz at 64 samples down to 23 Hz at 2048. It costs one update per 48
        samples at 48 kHz, where the host currently decides anything between 1-in-64 and 1-in-2048.
        Expressed in Hz so it holds across sample rates — a fixed sample interval would make the
        update rate sample-rate-dependent, which is this defect on the other axis.
    */
    static constexpr double lfoUpdateRateHz = 1000.0;

    /** The rate random-walk's time constant, in SECONDS.

        **500 ms, chosen for what the control does rather than to preserve any buffer's accident.**
        This is a slow drift in the modulation rate — a transport's speed wandering — so its musical
        range is hundreds of milliseconds to seconds. Inheriting the 64-sample buffer's behaviour
        would give ~67 ms, which reads as wobble in the modulation itself rather than as drift;
        inheriting the 512-sample one gives ~535 ms.

        500 ms sits close enough to that 512 figure that anything authored on a common default buffer
        keeps its character, and it is defensible on its own terms rather than because 512 happened
        to be the buffer somebody used. **If it sounds wrong once the tanks are running it is a
        number to tune, not one to reverse-engineer.**
    */
    static constexpr double rateWalkTimeConstantSeconds = 0.5;

    void prepare (double sr, int seedOffset = 0)
    {
        sampleRate = sr;
        random = juce::Random (seedFor (seedOffset));

        updateIntervalSamples = juce::jmax (1, (int) std::round (sr / lfoUpdateRateHz));
        samplesSinceUpdate = 0;

        // One-pole coefficient for the chosen time constant at the chosen update interval. Both
        // sides are now times, so neither moves when the host changes its buffer.
        const double updateSeconds = (double) updateIntervalSamples / sr;
        walkCoefficient = (float) (1.0 - std::exp (-updateSeconds / rateWalkTimeConstantSeconds));

        for (int i = 0; i < N; ++i)
        {
            // 0.31 .. 1.19 Hz, spread by an irrational step so no two lock together.
            nominalRate[(size_t) i] = 0.31f + 0.88f * (float) std::fmod (0.6180339887 * (i + 1), 1.0);
            rate[(size_t) i] = nominalRate[(size_t) i];
            phase[(size_t) i] = (float) std::fmod (0.7548776662 * (i + 1), 1.0);
        }
    }

    void reset() noexcept
    {
        // The counter resets with the phase. A render that began mid-interval would put the first
        // update at a different absolute sample than a render that began at one, which is exactly
        // the buffer dependence this class was fixed for — arriving through the back door.
        samplesSinceUpdate = 0;

        for (int i = 0; i < N; ++i)
            phase[(size_t) i] = (float) std::fmod (0.7548776662 * (i + 1), 1.0);
    }

    /** Advances the bank by ONE sample; returns true on the samples where its values changed.

        Call it once per sample and re-read `value (i)` when it returns true. The sine is only
        recomputed at an update, so the per-sample cost is an increment and a compare.

        ## FIXED 2026-08-15. What was here, and why one change closed all three

        Three defects, filed separately during the sweep because they had different defensibility
        and a ruling on the first must not dispose of the other two:

        **1 · The block's boundary phase was held across the whole block.** `advanceBlock` ran once
        per buffer and all three tanks read `value (i)` ONCE, outside their sample loop —
        PlateTank.cpp:112, FdnTank.cpp:147, DigitalRoomTank.cpp:118. Defensible as a CPU trade in
        itself, and plenty of shipping code makes it. What was not defensible was leaving it
        unstated, because **the modulation update rate WAS the buffer rate**: 750 Hz at 64 samples,
        23 Hz at 2048 — a stairstep on a modulator that coarsens as the buffer grows.

        **2 · The random walk stepped by a fixed `0.02f` per BLOCK.** A time constant whose value in
        seconds was whatever the buffer happened to be: ~67 ms at 64 samples, ~535 ms at 512.

        **3 · One `random.nextFloat()` per BLOCK**, so the walk's noise spectrum moved with the
        host's buffer setting.

        **A fixed update interval fixes 1, and 2 and 3 come free** — the step coefficient and the
        draw rate become time-based by construction. The alternative was accepting 1 and giving 2
        and 3 time-based coefficients, two changes with the stairstep left in; **the pre-stated
        discriminator closed that.** The sweep recorded that this casting's three block-size rows all
        begin at sample 1891 — one origin, not three — and required that index to VANISH rather than
        move. A phase held across a block is buffer-dependent whatever the coefficients do, so the
        two-change path could not have satisfied it.

        Before: sample-exact with MODULATION at 0, and 0.025 / 0.063 / 0.145 at 128 / 511 / 2048,
        every row first differing at sample 1891.

        **1 · The block's boundary phase is held across the block.** Defensible: an LFO updated once
        per buffer is a recognised CPU trade and plenty of shipping code makes it. What is not
        defensible is leaving it unstated, because the cost is larger than an invariance failure —
        **the modulation update rate IS the buffer rate**: 750 Hz at 64 samples, 23 Hz at 2048. That
        is a stairstep on a modulator that coarsens as the buffer grows, which a player hears before
        anybody measures it. If it stays, it wants a fixed update interval rather than the host's
        buffer.

        **2 · The random walk steps by a fixed 0.02 per BLOCK.** Not defensible at any buffer size:
        the coefficient is a time constant, so the rate's smoothing runs 32x faster at 2048 samples
        than at 64. Nobody chose this — it follows from 1.

        **3 · One `random.nextFloat()` per BLOCK.** Not defensible either: the number of draws per
        second is the buffer rate, so the walk's noise spectrum moves with the host's buffer setting.
        Also follows from 1, and also has no correct version at any size.

    */
    bool tick() noexcept
    {
        if (++samplesSinceUpdate < updateIntervalSamples)
            return false;

        samplesSinceUpdate = 0;

        for (int i = 0; i < N; ++i)
        {
            rate[(size_t) i] += walkCoefficient * (nominalRate[(size_t) i]
                                                   * (0.85f + 0.3f * random.nextFloat())
                                                   - rate[(size_t) i]);
            phase[(size_t) i] = std::fmod (phase[(size_t) i]
                                           + rate[(size_t) i] * (float) updateIntervalSamples
                                                              / (float) sampleRate,
                                           1.0f);
        }

        return true;
    }

    /** Sine value for line i at the current block phase, -1 .. 1. */
    float value (int i) const noexcept
    {
        return std::sin (juce::MathConstants<float>::twoPi * phase[(size_t) i]);
    }

private:
    double sampleRate = 44100.0;
    int updateIntervalSamples = 44;
    int samplesSinceUpdate = 0;
    float walkCoefficient = 0.002f;
    juce::Random random { 1 };
    std::array<float, (size_t) N> nominalRate {};
    std::array<float, (size_t) N> rate {};
    std::array<float, (size_t) N> phase {};
};

//==============================================================================
/** A tapped delay line producing the discrete early-reflection pattern that gives each algorithm
    its sense of a specific room before the late tail arrives. */
class EarlyReflections
{
public:
    struct Tap
    {
        float timeMs;
        float gain;
        int channel;    // 0 = left, 1 = right
    };

    void prepare (double sr, float maxTimeMs)
    {
        sampleRate = sr;
        delay.prepare ((int) std::ceil (maxTimeMs * 0.001 * sr) + 8);
    }

    void reset() noexcept { delay.reset(); }

    /** @param interpolated  false uses the nearest whole sample - Digital Room wants that
                             deliberately coarse behaviour, everything else does not. */
    void process (float input, const std::vector<Tap>& taps, float sizeScale,
                  bool interpolated, float& outL, float& outR) noexcept
    {
        delay.write (input);

        outL = 0.0f;
        outR = 0.0f;

        for (const auto& tap : taps)
        {
            float samples = tap.timeMs * 0.001f * (float) sampleRate * sizeScale;

            if (! interpolated)
                samples = std::floor (samples);

            const float value = delay.readAt (samples) * tap.gain;

            (tap.channel == 0 ? outL : outR) += value;
        }
    }

private:
    double sampleRate = 44100.0;
    Delay delay;
};

} // namespace ReverbPrimitives
