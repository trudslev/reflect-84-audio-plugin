#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <cmath>

/** Everything a tank needs for a block, in physical units. No tank ever reads the APVTS - the
    processor converts once and passes plain values down. */
struct TankParameters
{
    float sizeScale = 1.0f;         // 0.20 .. 1.00, multiplies every delay length
    float decaySeconds = 2.0f;      // RT60, 0.4 .. 8.0
    float density01 = 0.7f;
    float dampHFHz = 8000.0f;
    float dampLFHz = 120.0f;
    float mod01 = 0.3f;
    float grain01 = 0.0f;
};

/**
    One reverb algorithm.

    Four implementations, and they are genuinely different networks rather than one network with
    four coefficient tables - a plate is not a hall with longer delays. Each owns its own early
    reflections, its own late topology, and its own GrainStage inside its own feedback path.

    The polymorphic shape (interface + one class per algorithm + an engine that owns all four and
    crossfades between them) is Gatecrasher's ReverbTank/ReverbEngine pattern, reused directly.
*/
class ReverbTank
{
public:
    virtual ~ReverbTank() = default;

    virtual void prepare (const juce::dsp::ProcessSpec& spec) = 0;
    virtual void reset() = 0;

    /** Processes in place. The input is already pre-delayed; the tank produces the full wet
        signal, early reflections and tail together. */
    virtual void process (juce::AudioBuffer<float>& buffer, const TankParameters& p) = 0;

    /** 0-1 estimate of the energy still circulating, for the TANK LIVE lamp. Reported by the tank
        rather than measured at the output so the lamp keeps reporting through the tail after the
        input has stopped - and, at Mix 0, when the output is silent by definition. */
    virtual float getEnergy() const noexcept = 0;

protected:
    /** Feedback gain for one delay line of `lengthSamples` that should decay by 60 dB in
        `rt60Seconds`. The standard result: g = 10^(-3 * L / (RT60 * fs)). Capped just below unity
        because an FDN at exactly 1.0 never decays and one above it diverges. */
    static float feedbackForRT60 (float lengthSamples, float rt60Seconds, double sampleRate) noexcept
    {
        const float exponent = -3.0f * lengthSamples / (juce::jmax (0.05f, rt60Seconds) * (float) sampleRate);
        return juce::jlimit (0.0f, 0.9995f, std::pow (10.0f, exponent));
    }
};
