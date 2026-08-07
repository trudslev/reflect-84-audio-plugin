#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ReflectTheme.h"

#include <array>

class Reflect84AudioProcessor;

/**
    The live numeric layer: the value under each knob, and the IN/OUT peak readouts in the header.

    BRAND.md asks for real metering language over vague labels - actual dB, ms, Hz and % under
    every control rather than knob position alone - so this is not decoration; it is what makes the
    panel readable without wiggling anything.

    A full-canvas overlay so it can paint in absolute canvas coordinates, mouse-transparent, and
    repainted only when a displayed string actually changes. Polling at 20 Hz and repainting
    unconditionally would redraw a 1200x615 component twenty times a second to show the same
    numbers - CHORUS-60 does exactly that and its own source flags it as a divergence from
    Gatecrasher's change-detecting version.
*/
class PanelReadouts final : public juce::Component,
                            private juce::Timer
{
public:
    explicit PanelReadouts (Reflect84AudioProcessor& processor);
    ~PanelReadouts() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    /** Rebuilds the displayed strings; returns true if any of them moved. */
    bool refresh();

    Reflect84AudioProcessor& processorRef;

    std::array<juce::String, ReflectTheme::Layout::knobs.size()> readouts;
    juce::String inputText;
    juce::String outputText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelReadouts)
};
