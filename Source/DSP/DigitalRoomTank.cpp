#include "DigitalRoomTank.h"

using namespace ReverbPrimitives;

namespace
{
    // Comb lengths in ms, mutually prime-ish and short - a small hard-walled box, not a hall.
    constexpr std::array<float, 4> combMs { 29.7f, 37.1f, 41.3f, 47.9f };
    constexpr std::array<float, 2> allpassMs { 5.3f, 1.7f };

    constexpr float modDepthMs = 0.45f;   // deliberately shallow: Digital Room should stay a bit
                                          // metallic, that is its character rather than a fault
    constexpr float maxEarlyMs = 70.0f;

    float msToSamples (float ms, double sampleRate) noexcept
    {
        return ms * 0.001f * (float) sampleRate;
    }
}

//==============================================================================
void DigitalRoomTank::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    early.prepare (sampleRate, maxEarlyMs);

    // Sixteen sparse taps, 20-70 ms, alternating channels with slightly unequal gains so the
    // pattern is asymmetric - a real room's two ears never hear the same reflection sequence.
    taps.clear();

    constexpr std::array<float, 16> timesMs {
        20.3f, 23.1f, 26.9f, 29.7f, 33.1f, 36.7f, 39.3f, 42.9f,
        46.1f, 49.7f, 52.3f, 55.9f, 58.7f, 62.3f, 65.9f, 69.1f
    };

    for (size_t i = 0; i < timesMs.size(); ++i)
    {
        const float decayGain = std::pow (0.86f, (float) i);
        const float asymmetry = (i % 2 == 0) ? 1.0f : 0.87f;
        taps.push_back ({ timesMs[i], decayGain * asymmetry * 0.5f, (int) (i % 2) });
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int c = 0; c < numCombs; ++c)
            combs[(size_t) ch][(size_t) c].prepare (
                (int) std::ceil (msToSamples (combMs[(size_t) c] + modDepthMs * 2.0f, sampleRate)) + 8);

        for (int a = 0; a < numAllpasses; ++a)
            allpasses[(size_t) ch][(size_t) a].prepare (
                (int) std::ceil (msToSamples (allpassMs[(size_t) a], sampleRate)) + 8);
    }

    lfo.prepare (sampleRate, 1);
    grain.prepare (2);

    reset();
}

void DigitalRoomTank::reset()
{
    early.reset();

    for (int ch = 0; ch < 2; ++ch)
    {
        for (auto& comb : combs[(size_t) ch]) comb.reset();
        for (auto& ap : allpasses[(size_t) ch]) ap.reset();
        lowCut[(size_t) ch].reset();
    }

    lfo.reset();
    grain.reset();
    energy = 0.0f;
}

//==============================================================================
void DigitalRoomTank::process (juce::AudioBuffer<float>& buffer, const TankParameters& p)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());

    if (numChannels == 0 || numSamples == 0)
        return;

    grain.setGrain (p.grain01);

    for (int ch = 0; ch < 2; ++ch)
    {
        lowCut[(size_t) ch].setCutoff (p.dampLFHz, sampleRate);

        for (auto& comb : combs[(size_t) ch])
            comb.setDamping (p.dampHFHz, sampleRate);
    }

    // Low diffusion by design - this is where the audible discrete repeats come from.
    const float allpassFeedback = 0.30f + 0.25f * p.density01;

    // DENSITY also thins the early pattern: below halfway only every other tap contributes, so
    // the reflections read as countable events rather than as a wash.
    const size_t tapStride = p.density01 < 0.5f ? 2 : 1;

    std::vector<EarlyReflections::Tap> activeTaps;
    activeTaps.reserve (taps.size());

    for (size_t i = 0; i < taps.size(); i += tapStride)
        activeTaps.push_back (taps[i]);

    std::array<float, numCombs> combFeedback {};
    std::array<float, numCombs> combSamples {};
    std::array<float, numCombs> modOffset {};

    const float modScale = msToSamples (modDepthMs, sampleRate) * p.mod01;

    for (int c = 0; c < numCombs; ++c)
    {
        combSamples[(size_t) c] = msToSamples (combMs[(size_t) c] * p.sizeScale, sampleRate);
        combFeedback[(size_t) c] = feedbackForRT60 (combSamples[(size_t) c], p.decaySeconds, sampleRate);
        modOffset[(size_t) c] = modScale * lfo.value (c);
    }

    std::array<float*, 2> out {};

    for (int ch = 0; ch < numChannels; ++ch)
        out[(size_t) ch] = buffer.getWritePointer (ch);

    float peak = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        if (lfo.tick())
            for (int c = 0; c < numCombs; ++c)
                modOffset[(size_t) c] = modScale * lfo.value (c);

        float mono = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            mono += out[(size_t) ch][n];

        mono *= numChannels > 1 ? 0.5f : 1.0f;

        float earlyL = 0.0f, earlyR = 0.0f;
        early.process (mono, activeTaps, p.sizeScale, /* interpolated */ false, earlyL, earlyR);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const size_t c = (size_t) ch;

            float combSum = 0.0f;

            for (int i = 0; i < numCombs; ++i)
            {
                // Each channel offsets its comb lengths slightly, which is the only stereo
                // decorrelation this topology has - the combs are otherwise independent per side.
                const float channelSkew = ch == 0 ? 1.0f : 1.021f;

                combSum += combs[c][(size_t) i].process (
                    mono,
                    combSamples[(size_t) i] * channelSkew + modOffset[(size_t) i],
                    combFeedback[(size_t) i]);
            }

            combSum /= (float) numCombs;

            for (int a = 0; a < numAllpasses; ++a)
                combSum = allpasses[c][(size_t) a].process (
                    combSum, msToSamples (allpassMs[(size_t) a] * p.sizeScale, sampleRate), allpassFeedback);

            combSum = lowCut[c].process (combSum);
            combSum = grain.process (ch, combSum);

            const float wet = (ch == 0 ? earlyL : earlyR) * 0.6f + combSum;
            out[c][n] = wet;
            peak = juce::jmax (peak, std::abs (wet));
        }
    }

    energy = juce::jlimit (0.0f, 1.0f, peak * 2.0f);
}
