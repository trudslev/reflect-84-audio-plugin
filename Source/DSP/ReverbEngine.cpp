#include "ReverbEngine.h"

ReverbEngine::ReverbEngine()
{
    // Index order is the DSP enum's, NOT the panel's corner order - see Parameters.h's Algorithm
    // enum for why those deliberately differ.
    tanks[0] = std::make_unique<PlateTank>();
    tanks[1] = std::make_unique<DigitalRoomTank>();
    tanks[2] = std::make_unique<ChamberTank>();
    tanks[3] = std::make_unique<HallTank>();
}

ReverbTank* ReverbEngine::tankFor (int index) noexcept
{
    return tanks[(size_t) juce::jlimit (0, (int) tanks.size() - 1, index)].get();
}

void ReverbEngine::prepare (const juce::dsp::ProcessSpec& spec, float initialPreDelayMs,
                            int initialAlgorithm)
{
    sampleRate = spec.sampleRate;

    for (auto& tank : tanks)
        tank->prepare (spec);

    preDelayLine.setMaximumDelayInSamples (
        (int) std::ceil (maxPreDelayMs * 0.001 * spec.sampleRate) + 4);
    preDelayLine.prepare (spec);

    previousTankOutput.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);

    // The tank the first block will actually be asked for — see the header. A stored copy of a
    // selection guarding a TRANSITION must start at what the configuration selects.
    currentAlgorithm = juce::jlimit (0, (int) tanks.size() - 1, initialAlgorithm);

    switchCrossfade.reset (spec.sampleRate, switchCrossfadeSeconds);
    switchCrossfade.setCurrentAndTargetValue (1.0f);

    // Pre-delay moves a read position, so it is smoothed per sample: stepping it per block turns
    // a Pre-Delay automation ramp into a series of clicks.
    preDelaySmoothed.reset (spec.sampleRate, 0.05);
    preDelaySmoothed.setCurrentAndTargetValue (initialPreDelayMs);

    reset();
}

void ReverbEngine::reset()
{
    for (auto& tank : tanks)
        tank->reset();

    preDelayLine.reset();
    previousTankOutput.clear();
    switchCrossfade.setCurrentAndTargetValue (1.0f);
}

float ReverbEngine::getEnergy() const noexcept
{
    return tanks[(size_t) currentAlgorithm]->getEnergy();
}

//==============================================================================
void ReverbEngine::process (juce::AudioBuffer<float>& buffer,
                            int algorithmIndex,
                            const TankParameters& params,
                            float preDelayMs)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    // --- Pre-delay -----------------------------------------------------------
    {
        preDelaySmoothed.setTargetValue (
            juce::jlimit (0.0f, maxPreDelayMs, preDelayMs) * 0.001f * (float) sampleRate);

        for (int n = 0; n < numSamples; ++n)
        {
            const float delaySamples = juce::jmax (0.0f, preDelaySmoothed.getNextValue());

            for (int ch = 0; ch < numChannels; ++ch)
            {
                preDelayLine.pushSample (ch, buffer.getSample (ch, n));
                buffer.setSample (ch, n, preDelayLine.popSample (ch, delaySamples, true));
            }
        }
    }

    const int requested = juce::jlimit (0, (int) tanks.size() - 1, algorithmIndex);

    if (requested != currentAlgorithm)
    {
        // Snapshot what the OUTGOING tank would have produced for this block's input, then swap.
        // makeCopyOf's second argument is avoidReallocating - this must not allocate here.
        previousTankOutput.makeCopyOf (buffer, true);
        tankFor (currentAlgorithm)->process (previousTankOutput, params);

        currentAlgorithm = requested;
        switchCrossfade.setCurrentAndTargetValue (0.0f);
        switchCrossfade.setTargetValue (1.0f);
    }

    tankFor (currentAlgorithm)->process (buffer, params);

    // Early-out: no per-sample blending work in the overwhelmingly common case.
    if (! switchCrossfade.isSmoothing() && switchCrossfade.getCurrentValue() >= 1.0f)
        return;

    for (int n = 0; n < numSamples; ++n)
    {
        const float blend = switchCrossfade.getNextValue();

        for (int ch = 0; ch < juce::jmin (numChannels, previousTankOutput.getNumChannels()); ++ch)
        {
            const float previous = previousTankOutput.getSample (ch, n);
            const float current = buffer.getSample (ch, n);
            buffer.setSample (ch, n, previous + (current - previous) * blend);
        }
    }
}
