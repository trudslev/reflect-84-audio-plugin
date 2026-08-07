#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../Source/Parameters.h"

/**
    A bare AudioProcessor that exists only to own a real APVTS built from REFLECT-84's actual
    parameter layout. Anything that manipulates parameters - ProgramManager above all - can then be
    tested against the same parameter set the plugin ships, without dragging in the DSP or the GUI.
*/
class TestHostProcessor final : public juce::AudioProcessor
{
public:
    TestHostProcessor()
        : AudioProcessor (BusesProperties()
                              .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "PARAMETERS", createReflect84ParameterLayout())
    {
    }

    juce::AudioProcessorValueTreeState apvts;

    const juce::String getName() const override { return "Reflect84TestHost"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};

/** A scratch directory that deletes itself, so no test ever writes into the user's real
    Programs folder. */
struct ScopedTestDirectory
{
    explicit ScopedTestDirectory (const juce::String& name)
        : directory (juce::File::getSpecialLocation (juce::File::tempDirectory)
                         .getChildFile ("Reflect84Tests")
                         .getChildFile (name))
    {
        directory.deleteRecursively();
        directory.createDirectory();
    }

    ~ScopedTestDirectory() { directory.deleteRecursively(); }

    juce::File directory;
};
