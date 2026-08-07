#include "PlateTank.h"

using namespace ReverbPrimitives;

namespace
{
    // Dattorro's plate, expressed in milliseconds rather than his 29761 Hz sample counts so the
    // network keeps its proportions at any sample rate.
    constexpr std::array<float, 4> inputDiffusionMs { 4.77f, 3.60f, 12.74f, 9.31f };

    constexpr std::array<float, 2> modulatedAllpassMs { 22.58f, 30.51f };
    constexpr std::array<float, 2> delayAMs          { 149.63f, 141.69f };
    constexpr std::array<float, 2> fixedAllpassMs    { 60.48f, 89.24f };
    constexpr std::array<float, 2> delayBMs          { 124.99f, 106.28f };

    // Seven output taps per channel, read from the four tank delays at points that are mutually
    // prime-ish so no two taps reinforce the same repeat.
    constexpr std::array<float, 7> tapFractions { 0.0893f, 0.9987f, 0.6427f, 0.6706f,
                                                 0.6683f, 0.0628f, 0.3581f };

    constexpr float modDepthMs = 1.6f;
    constexpr float maxSizeScale = 1.0f;

    float msToSamples (float ms, double sampleRate) noexcept
    {
        return ms * 0.001f * (float) sampleRate;
    }
}

//==============================================================================
void PlateTank::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    const auto capacity = [this] (float ms)
    {
        return (int) std::ceil (msToSamples (ms * maxSizeScale, sampleRate)) + 8;
    };

    for (size_t i = 0; i < inputDiffuser.size(); ++i)
        inputDiffuser[i].prepare (capacity (inputDiffusionMs[i]));

    for (size_t i = 0; i < 2; ++i)
    {
        modulatedAllpass[i].prepare (capacity (modulatedAllpassMs[i] + modDepthMs * 2.0f));
        fixedAllpass[i].prepare (capacity (fixedAllpassMs[i]));
        delayA[i].prepare (capacity (delayAMs[i]));
        delayB[i].prepare (capacity (delayBMs[i]));
    }

    lfo.prepare (sampleRate, 0);
    grain.prepare (2);

    reset();
}

void PlateTank::reset()
{
    for (auto& ap : inputDiffuser) ap.reset();

    for (size_t i = 0; i < 2; ++i)
    {
        modulatedAllpass[i].reset();
        fixedAllpass[i].reset();
        delayA[i].reset();
        delayB[i].reset();
        damping[i].reset();
        lowCut[i].reset();
    }

    lfo.reset();
    grain.reset();
    tankState.fill (0.0f);
    energy = 0.0f;
}

//==============================================================================
void PlateTank::process (juce::AudioBuffer<float>& buffer, const TankParameters& p)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());

    if (numChannels == 0 || numSamples == 0)
        return;

    for (size_t i = 0; i < 2; ++i)
    {
        damping[i].setCutoff (p.dampHFHz, sampleRate);
        lowCut[i].setCutoff (p.dampLFHz, sampleRate);
    }

    grain.setGrain (p.grain01);
    lfo.advanceBlock (numSamples);

    // Diffusion coefficients: DENSITY controls how hard the input is smeared before it reaches
    // the tank, which on a plate is most of what "density" means.
    const float inputDiffusion = 0.55f + 0.25f * p.density01;
    const float tankDiffusion  = 0.45f + 0.25f * p.density01;

    // One shared decay coefficient for the whole loop, scaled from the total loop length.
    const float loopSamples = msToSamples ((delayAMs[0] + delayBMs[0] + delayAMs[1] + delayBMs[1])
                                               * p.sizeScale, sampleRate);
    const float decay = feedbackForRT60 (loopSamples * 0.5f, p.decaySeconds, sampleRate);

    const float modSamples = msToSamples (modDepthMs, sampleRate) * p.mod01;

    std::array<float*, 2> out {};

    for (int ch = 0; ch < numChannels; ++ch)
        out[(size_t) ch] = buffer.getWritePointer (ch);

    std::array<float, 2> lfoValue { lfo.value (0), lfo.value (1) };

    float peak = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        // Mono sum into the diffusers - a plate is driven at one point, not two.
        float x = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            x += out[(size_t) ch][n];

        x *= numChannels > 1 ? 0.5f : 1.0f;

        for (size_t i = 0; i < inputDiffuser.size(); ++i)
            x = inputDiffuser[i].process (x,
                                          msToSamples (inputDiffusionMs[i] * p.sizeScale, sampleRate),
                                          i < 2 ? inputDiffusion : inputDiffusion * 0.85f);

        std::array<float, 2> tapSum { 0.0f, 0.0f };

        for (int half = 0; half < 2; ++half)
        {
            const size_t h = (size_t) half;
            const size_t other = (size_t) (1 - half);

            // The figure-of-eight: each half is fed by the input plus the OTHER half's output.
            float v = x + tankState[other] * decay;

            const float modulated = msToSamples (modulatedAllpassMs[h] * p.sizeScale, sampleRate)
                                  + modSamples * lfoValue[h];

            v = modulatedAllpass[h].process (v, modulated, tankDiffusion);

            delayA[h].write (v);
            v = delayA[h].readAt (msToSamples (delayAMs[h] * p.sizeScale, sampleRate));

            // Damping is INSIDE the loop, so highs decay faster than lows on every pass rather
            // than being filtered once on the way out.
            v = damping[h].process (v);
            v = lowCut[h].process (v);

            v = fixedAllpass[h].process (v, msToSamples (fixedAllpassMs[h] * p.sizeScale, sampleRate),
                                         tankDiffusion * 0.9f);

            delayB[h].write (v);
            v = delayB[h].readAt (msToSamples (delayBMs[h] * p.sizeScale, sampleRate));

            // Grain closes the loop: quantised here, so the next pass re-quantises what this one
            // already truncated and the tail steps down rather than gliding.
            v = grain.process (half, v);

            tankState[h] = v;

            // Seven taps across both of this half's delays.
            for (size_t t = 0; t < tapFractions.size(); ++t)
            {
                const auto& line = (t % 2 == 0) ? delayA[h] : delayB[h];
                const float lengthMs = (t % 2 == 0 ? delayAMs[h] : delayBMs[h]) * p.sizeScale;
                tapSum[h] += line.readAt (msToSamples (lengthMs * tapFractions[t], sampleRate));
            }
        }

        const float scale = 0.16f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float wet = tapSum[(size_t) (numChannels > 1 ? ch : 0)] * scale;
            out[(size_t) ch][n] = wet;
            peak = juce::jmax (peak, std::abs (wet));
        }
    }

    energy = juce::jlimit (0.0f, 1.0f, peak * 2.0f);
}
