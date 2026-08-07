#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "DSP/ProgramManager.h"
#include "DSP/ReverbEngine.h"
#include "DSP/StereoWidthStage.h"

/**
    REFLECT-84 - a merged plate / digital-rack reverb, the fourth Neon Foundry casting.

    Signal chain (fixed order, all in processBlock):

        in --+------------------------------ dry -----------------------------+
             |                                                                 |
             +- PreDelay - Input Diffusion -+                                   |
                                            |                                   |
                +- Early Reflections -------+---------------+                   |
                |                                            |                   |
                +- Late Tank (per-algorithm topology) -------+                   |
                      ^                                       |                   |
                      +- feedback: Damping LF/HF -> Modulation -> GRAIN ---+     |
                                                                            |     |
                       ER/late blend - Stereo Width - Mix -----------------+-----+
                                                        |
                                              Output Trim - out

    The grain stage lives INSIDE the tank's feedback path rather than on the output, so its
    truncation compounds once per recirculation and the decay envelope itself breaks into steps -
    which is exactly the picture the TANK LIVE scope draws. See Source/DSP/GrainSpec.h.
*/
class Reflect84AudioProcessor final : public juce::AudioProcessor
{
public:
    Reflect84AudioProcessor();
    ~Reflect84AudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return NF_PRODUCT_NAME; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    /** A conservative estimate, not a hard bound: the tanks are feedback networks whose actual
        decay varies continuously with the automatable Size/Decay parameters. Hosts use this for
        freeze/bounce trailing silence, so it is deliberately longer than the 8s maximum RT60. */
    double getTailLengthSeconds() const override { return 12.0; }

    //==============================================================================
    // Programs, never Presets - BRAND.md's terminology rule reaches the code, not just the label.
    int getNumPrograms() override { return programManager.getNumPrograms(); }
    int getCurrentProgram() override { return programManager.getCurrentProgram(); }
    void setCurrentProgram (int index) override { programManager.requestProgramChange (index); }
    const juce::String getProgramName (int index) override { return programManager.getProgramName (index); }

    /** Renaming in place would be an overwrite by another name, and there is deliberately no
        overwrite path - Save always creates a new Program. */
    void changeProgramName (int, const juce::String&) override {}

    ProgramManager& getProgramManager() noexcept { return programManager; }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    /** GUI-facing derived display state, polled by the editor's timers. Continuous scalars go
        through plain relaxed atomics rather than a FIFO - the house pattern across all three
        sibling castings, and sufficient because the GUI redraws far more often than these need
        to be exact.

        The IN/OUT values are already in dB: the conversion belongs on the producing side so both
        readouts and anything else reading them agree on what "the current level" means. */
    float getInputMeterDb()  const noexcept { return inputMeterDb.load  (std::memory_order_relaxed); }
    float getOutputMeterDb() const noexcept { return outputMeterDb.load (std::memory_order_relaxed); }

    /** 0-1 energy still circulating in the tank. Drives the TANK LIVE lamp - BRAND.md's rule of
        exactly one LED per plugin reporting the most important live discrete state. */
    float getTankEnergy() const noexcept { return tankEnergy.load (std::memory_order_relaxed); }

private:
    //==============================================================================
    ProgramManager programManager { apvts };

    //==============================================================================
    // Cached raw parameter pointers. Read once at the top of processBlock; never call
    // getRawParameterValue per block.
    std::atomic<float>* sizeParam       = nullptr;
    std::atomic<float>* decayParam      = nullptr;
    std::atomic<float>* preDelayParam   = nullptr;
    std::atomic<float>* densityParam    = nullptr;
    std::atomic<float>* dampHFParam     = nullptr;
    std::atomic<float>* dampLFParam     = nullptr;
    std::atomic<float>* modulationParam = nullptr;
    std::atomic<float>* grainParam      = nullptr;
    std::atomic<float>* widthParam      = nullptr;
    std::atomic<float>* mixParam        = nullptr;
    std::atomic<float>* trimParam       = nullptr;
    std::atomic<float>* algorithmParam  = nullptr;

    ReverbEngine reverbEngine;

    juce::AudioBuffer<float> dryBuffer;

    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> trimSmoothed;
    juce::SmoothedValue<float> widthSmoothed;

    std::atomic<float> inputMeterDb  { -100.0f };
    std::atomic<float> outputMeterDb { -100.0f };
    std::atomic<float> tankEnergy    { 0.0f };

    double displaySampleRate = 44100.0;

    void updateDisplayState (const juce::AudioBuffer<float>& dry,
                             const juce::AudioBuffer<float>& out,
                             int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reflect84AudioProcessor)
};
