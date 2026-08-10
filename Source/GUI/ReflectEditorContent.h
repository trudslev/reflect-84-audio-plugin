#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "AlgorithmSwitch.h"
#include "PanelBackground.h"
#include "PanelReadouts.h"
#include "ProgramHeader.h"
#include "ReflectLookAndFeel.h"
#include "TankScope.h"

#include <array>
#include <memory>

class Reflect84AudioProcessor;

/**
    The fixed 1200 x 615 reference canvas.

    Every child draws in untransformed canvas coordinates and never learns the window size -
    PluginEditor applies one uniform scale transform to this whole component. There is deliberately
    no resized(): all bounds are set once, here, from ReflectTheme::Layout.
*/
class ReflectEditorContent final : public juce::Component
{
public:
    explicit ReflectEditorContent (Reflect84AudioProcessor& processor);
    ~ReflectEditorContent() override;

    /** The disengaged state, GUI-SPEC.md section 10. Drawn OVER every child rather than in paint(),
        which is why this is paintOverChildren: the multiply has to sit above the whole panel
        including the texture overlay, and below nothing. */
    void paintOverChildren (juce::Graphics& g) override;

private:
    Reflect84AudioProcessor& processorRef;

    // Declared BEFORE the child components so it outlives them - a LookAndFeel destroyed while a
    // child still points at it is a use-after-free during teardown.
    ReflectLookAndFeel lookAndFeel;

    using APVTS = juce::AudioProcessorValueTreeState;

    // Declaration order is z-order for equally-positioned children added in sequence: the static
    // background first, then the controls, then the mouse-transparent live layers on top.
    PanelBackground panelBackground;

    std::array<std::unique_ptr<ReflectKnob>, ReflectTheme::Layout::knobs.size()> knobs;
    std::array<std::unique_ptr<APVTS::SliderAttachment>, ReflectTheme::Layout::knobs.size()> knobAttachments;

    AlgorithmSwitch algorithmSwitch;
    std::unique_ptr<APVTS::SliderAttachment> algorithmAttachment;

    TankScope tankScope;
    ProgramHeader programHeader;

    /** Paints nothing and claims no clicks of its own; it exists so the Program list has a parent
        area to be laid out in. Its bounds are what stop the list moving or overflowing the panel -
        see the constructor, and ../../CLAUDE.md's "The Program dropdown". */
    juce::Component menuHost;
    PanelReadouts panelReadouts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReflectEditorContent)
};
