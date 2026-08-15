#include "FdnTank.h"

using namespace ReverbPrimitives;

namespace
{
    float msToSamples (float ms, double sampleRate) noexcept
    {
        return ms * 0.001f * (float) sampleRate;
    }

    /** An early-reflection pattern from a list of times, with a per-tap gain law and alternating
        channels. `asymmetry` unbalances the two sides so the left and right patterns are never
        the same sequence - the single cheapest thing that stops a reverb sounding like one mono
        signal in two speakers. */
    std::vector<EarlyReflections::Tap> buildTaps (const std::vector<float>& timesMs,
                                                  float falloff,
                                                  float asymmetry)
    {
        std::vector<EarlyReflections::Tap> taps;
        taps.reserve (timesMs.size());

        for (size_t i = 0; i < timesMs.size(); ++i)
            taps.push_back ({ timesMs[i],
                              std::pow (falloff, (float) i) * ((i % 2 == 0) ? 1.0f : asymmetry),
                              (int) (i % 2) });

        return taps;
    }
}

//==============================================================================
FdnTank::FdnTank (Config config) : cfg (std::move (config))
{
    jassert (cfg.numLines == 4 || cfg.numLines == 8);
    jassert ((int) cfg.lineMs.size() == cfg.numLines);
}

void FdnTank::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    early.prepare (sampleRate, cfg.maxEarlyMs);

    inputDiffuser.resize (cfg.inputDiffusionMs.size());

    for (size_t i = 0; i < inputDiffuser.size(); ++i)
        inputDiffuser[i].prepare ((int) std::ceil (msToSamples (cfg.inputDiffusionMs[i], sampleRate)) + 8);

    for (int i = 0; i < cfg.numLines; ++i)
        lines[(size_t) i].prepare (
            (int) std::ceil (msToSamples (cfg.lineMs[(size_t) i] + cfg.modDepthMs * 2.0f, sampleRate)) + 8);

    lfo.prepare (sampleRate, cfg.seed);
    grain.prepare (2);

    reset();
}

void FdnTank::reset()
{
    early.reset();

    for (auto& ap : inputDiffuser)
        ap.reset();

    for (int i = 0; i < maxLines; ++i)
    {
        lines[(size_t) i].reset();
        damping[(size_t) i].reset();
        lowCut[(size_t) i].reset();
    }

    feedbackState.fill (0.0f);
    lfo.reset();
    grain.reset();
    energy = 0.0f;
}

//==============================================================================
void FdnTank::mix (std::array<float, maxLines>& v) const noexcept
{
    const int n = cfg.numLines;

    if (cfg.hadamardMixing)
    {
        // Fast Walsh-Hadamard butterflies, then normalise by 1/sqrt(N) to keep the transform
        // orthogonal - without that the network gains energy on every pass and blows up.
        for (int step = 1; step < n; step <<= 1)
        {
            for (int i = 0; i < n; i += step << 1)
            {
                for (int j = i; j < i + step; ++j)
                {
                    const float a = v[(size_t) j];
                    const float b = v[(size_t) (j + step)];
                    v[(size_t) j] = a + b;
                    v[(size_t) (j + step)] = a - b;
                }
            }
        }

        const float norm = 1.0f / std::sqrt ((float) n);

        for (int i = 0; i < n; ++i)
            v[(size_t) i] *= norm;
    }
    else
    {
        // Householder: y = x - (2/N) * sum(x). Orthogonal, and every line ends up coupled to
        // every other one in a single pass.
        float sum = 0.0f;

        for (int i = 0; i < n; ++i)
            sum += v[(size_t) i];

        const float scale = 2.0f / (float) n;

        for (int i = 0; i < n; ++i)
            v[(size_t) i] -= scale * sum;
    }
}

void FdnTank::process (juce::AudioBuffer<float>& buffer, const TankParameters& p)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (2, buffer.getNumChannels());

    if (numChannels == 0 || numSamples == 0)
        return;

    grain.setGrain (p.grain01);

    const int n = cfg.numLines;

    std::array<float, maxLines> lengthSamples {};
    std::array<float, maxLines> feedback {};
    std::array<float, maxLines> modOffset {};

    const float modScale = msToSamples (cfg.modDepthMs, sampleRate) * p.mod01;

    for (int i = 0; i < n; ++i)
    {
        const size_t s = (size_t) i;

        lengthSamples[s] = msToSamples (cfg.lineMs[s] * p.sizeScale, sampleRate);
        feedback[s] = feedbackForRT60 (lengthSamples[s], p.decaySeconds, sampleRate);
        modOffset[s] = modScale * lfo.value (i);

        damping[s].setCutoff (p.dampHFHz, sampleRate);
        lowCut[s].setCutoff (p.dampLFHz, sampleRate);
    }

    const float diffusion = cfg.diffusionBase + cfg.diffusionRange * p.density01;

    std::array<float*, 2> out {};

    for (int ch = 0; ch < numChannels; ++ch)
        out[(size_t) ch] = buffer.getWritePointer (ch);

    float peak = 0.0f;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        // Only the modulation offsets follow the bank; lengths, feedback and the filters are
        // parameter-driven and stay per block.
        if (lfo.tick())
            for (int i = 0; i < n; ++i)
                modOffset[(size_t) i] = modScale * lfo.value (i);

        float mono = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            mono += out[(size_t) ch][sampleIndex];

        mono *= numChannels > 1 ? 0.5f : 1.0f;

        float earlyL = 0.0f, earlyR = 0.0f;
        early.process (mono, cfg.taps, p.sizeScale, /* interpolated */ true, earlyL, earlyR);

        float diffused = mono;

        for (size_t i = 0; i < inputDiffuser.size(); ++i)
            diffused = inputDiffuser[i].process (
                diffused, msToSamples (cfg.inputDiffusionMs[i] * p.sizeScale, sampleRate), diffusion);

        // Read every line, then mix, then write back - the read has to happen before the mix or
        // the matrix would be operating on a half-updated state.
        std::array<float, maxLines> read {};

        for (int i = 0; i < n; ++i)
        {
            const size_t s = (size_t) i;
            float v = lines[s].readAt (lengthSamples[s] + modOffset[s]);

            v = damping[s].process (v);
            v = lowCut[s].process (v);

            read[s] = v * feedback[s];
        }

        mix (read);

        for (int i = 0; i < n; ++i)
        {
            const size_t s = (size_t) i;

            // Grain closes each line's loop, so the truncation compounds once per circulation
            // and the tail's envelope steps down rather than gliding.
            lines[s].write (grain.process (i % 2, diffused + read[s]));
        }

        // Alternate lines feed alternate outputs, which decorrelates the two channels without
        // needing a separate widening stage inside the tank.
        std::array<float, 2> late { 0.0f, 0.0f };

        for (int i = 0; i < n; ++i)
            late[(size_t) (i % 2)] += lines[(size_t) i].readAt (lengthSamples[(size_t) i]);

        const float lateScale = 1.0f / std::sqrt ((float) n);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float wet = (ch == 0 ? earlyL : earlyR) * cfg.earlyGain
                            + late[(size_t) (numChannels > 1 ? ch : 0)] * lateScale;

            out[(size_t) ch][sampleIndex] = wet;
            peak = juce::jmax (peak, std::abs (wet));
        }
    }

    energy = juce::jlimit (0.0f, 1.0f, peak * 2.0f);
}

//==============================================================================
ChamberTank::ChamberTank()
    : FdnTank ([]
      {
          Config c;
          c.numLines = 4;
          c.hadamardMixing = false;                          // Householder
          c.lineMs = { 23.1f, 29.7f, 37.3f, 44.9f };         // mutually prime-ish, medium space
          c.inputDiffusionMs = { 8.3f, 5.9f, 3.7f };
          c.diffusionBase = 0.42f;
          c.diffusionRange = 0.28f;
          c.modDepthMs = 0.9f;
          c.maxEarlyMs = 95.0f;
          c.earlyGain = 0.55f;
          c.seed = 2;

          // Asymmetric, closely-spaced, no gap: a chamber's reflections start almost immediately
          // and crowd together.
          c.taps = buildTaps ({ 15.1f, 18.7f, 22.3f, 27.1f, 31.9f, 37.7f,
                                44.3f, 51.1f, 58.9f, 67.3f, 77.9f, 89.3f },
                              0.88f, 0.82f);
          return c;
      }())
{
}

HallTank::HallTank()
    : FdnTank ([]
      {
          Config c;
          c.numLines = 8;
          c.hadamardMixing = true;                           // Hadamard
          c.lineMs = { 45.7f, 53.9f, 62.3f, 71.1f,
                       82.7f, 94.3f, 109.1f, 127.9f };       // long, mutually prime-ish
          c.inputDiffusionMs = { 13.7f, 9.7f, 6.7f, 4.3f };
          c.diffusionBase = 0.48f;
          c.diffusionRange = 0.30f;
          c.modDepthMs = 2.4f;                               // the deepest of the four
          c.maxEarlyMs = 170.0f;
          c.earlyGain = 0.40f;
          c.seed = 3;

          // Note the gap: a tight cluster of first reflections at 25-45 ms, then nothing until
          // 95 ms. That silence is what the ear reads as distance to the far wall.
          c.taps = buildTaps ({ 25.3f, 31.7f, 38.9f, 44.3f,
                                95.1f, 108.7f, 121.3f, 134.9f, 148.1f, 161.7f },
                              0.90f, 0.78f);
          return c;
      }())
{
}
