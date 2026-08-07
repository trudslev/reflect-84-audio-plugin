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
    void prepare (double sr, int seedOffset = 0)
    {
        sampleRate = sr;
        random = juce::Random (seedFor (seedOffset));

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
        for (int i = 0; i < N; ++i)
            phase[(size_t) i] = (float) std::fmod (0.7548776662 * (i + 1), 1.0);
    }

    /** Advances once per block and re-targets the random walk; the LFOs themselves are read
        per sample. */
    void advanceBlock (int numSamples) noexcept
    {
        for (int i = 0; i < N; ++i)
        {
            rate[(size_t) i] += 0.02f * (nominalRate[(size_t) i]
                                         * (0.85f + 0.3f * random.nextFloat()) - rate[(size_t) i]);
            phase[(size_t) i] = std::fmod (phase[(size_t) i]
                                           + rate[(size_t) i] * (float) numSamples / (float) sampleRate,
                                           1.0f);
        }
    }

    /** Sine value for line i at the current block phase, -1 .. 1. */
    float value (int i) const noexcept
    {
        return std::sin (juce::MathConstants<float>::twoPi * phase[(size_t) i]);
    }

private:
    double sampleRate = 44100.0;
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
