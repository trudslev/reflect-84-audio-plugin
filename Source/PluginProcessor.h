#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <nf/UserEditGate.h>

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

    /** Handing the host a bypass parameter is what makes its bypass button drive ours, and what
        lets the editor paint the disengaged state (GUI-SPEC.md section 10). Returning it also makes
        US responsible for the bypassed audio path - JUCE does not insert one on our behalf. */
    juce::AudioParameterBool* getBypassParameter() const override { return bypassParam; }
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
    //==============================================================================
    /** **The host adapter - the ONLY place a Program is addressed by position.**

        **The list is the Factory bank and nothing else** - not INIT, not User Programs.
        juce_AudioProcessor.h documents getNumPrograms as "The value returned must be valid as soon
        as this object is created, and must not change over its lifetime", and a count including
        User Programs changed the moment one was saved.

        Before anyone makes it dynamic again: JUCE's VST3 wrapper builds the automatable Program
        parameter ONCE in its constructor from this value, so a Program saved afterwards was
        unreachable from the host. That was the API keeping its documented promise, not a bug.

        Excluding INIT too means host index n IS Factory Program n+1.

        **Accepted divergence.** getCurrentProgram answers 0 while a User Program is loaded, so a
        host's menu shows a Factory name while the panel shows the user's Program. Sound and panel
        are both correct; only the host's own menu is wrong. */
    int getNumPrograms() override { return programManager.getNumPrograms(); }
    int getCurrentProgram() override { return programManager.getCurrentFactoryPosition(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override { return programManager.getProgramName (index); }

    /** Renaming in place would be an overwrite by another name, and there is deliberately no
        overwrite path - Save always creates a new Program. */
    /** Deliberately a no-op: with Factory-only exposure nothing on the host's list can be renamed.
        Implementing it would be a back door into the Factory bank, which is what the permanent
        slugs exist to prevent. */
    void changeProgramName (int, const juce::String&) override {}

    ProgramManager& getProgramManager() noexcept { return programManager; }

    /** **Guards a host replaying a stale program index over a just-restored session.** Armed by
        setStateInformation, consumed by the next setCurrentProgram, disarmed by the first
        USER-originated edit. Automation must not disarm it: a host may write automation on load
        before replaying.

        Public because the editor hands it to `nf::connectUserEdit` for every control — which is the
        point of it living in core. This plugin once carried the guard with **zero** call sites for
        its disarm, so after restoring a session the guard stayed armed indefinitely and reverting an
        edit from the host did nothing at all. Wiring the disarm and the LCD hand-off separately is
        what made that possible; they are one call now. See nf/UserEditGate.h. */
    nf::UserEditGate userEdits;

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
    // Cached in the constructor like the rest, but typed rather than atomic<float>*: JUCE's
    // getBypassParameter() contract wants the parameter object itself.
    juce::AudioParameterBool* bypassParam = nullptr;

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
