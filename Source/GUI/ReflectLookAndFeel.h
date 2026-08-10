#pragma once

#include "ReflectTheme.h"

/**
    Draws every continuous knob on the panel.

    design/README.md section 4 describes one knob component with four size variants, differing only
    in body diameter, tick spacing, pointer proportions and face gradient - so there is one paint
    path here, parameterised by ReflectTheme::Layout::KnobVariant, rather than four near-copies.

    The ALGORITHM rotary is deliberately NOT drawn here: it is a 4-position detented switch with a
    dark bezel-matching face and corner labels, not a member of the knob grammar. See
    AlgorithmSwitch.
*/
class ReflectLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ReflectLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    /** The Program menu drops out of the LCD, so it uses the panel's own monospace face rather
        than the host system's UI font - a menu of Program names in Helvetica hanging off a 1980s
        rack display breaks the illusion immediately. */
    juce::Font getPopupMenuFont() override;

    /** Paints one knob body at an explicit centre, independent of any Slider. Exposed so the
        LookAndFeel path and any direct caller share a single implementation, and so the geometry
        is unit-testable without a live Component. */
    static void paintKnob (juce::Graphics& g,
                           juce::Point<float> centre,
                           ReflectTheme::Layout::KnobSize size,
                           const ReflectTheme::Layout::KnobScale& scale,
                           float value01);
};

/**
    A continuous panel knob. Subclasses juce::Slider so it stays SliderAttachment-compatible while
    carrying its own size variant.

    Drag behaviour follows design/README.md section 7: vertical, 180px for the full 0-1 travel.
    Fine-drag and double-click-to-default are listed there as gaps in the prototype to be added in
    the real build, so both are here.
*/
class ReflectKnob final : public juce::Slider
{
public:
    ReflectKnob (ReflectTheme::Layout::KnobSize sizeVariant,
                 ReflectTheme::Layout::KnobScale scaleForKnob);

    const ReflectTheme::Layout::KnobScale& scale() const noexcept { return knobScale; }

    ReflectTheme::Layout::KnobSize size() const noexcept { return knobSize; }

    const ReflectTheme::Layout::KnobVariant& variant() const noexcept
    {
        return ReflectTheme::Layout::variantFor (knobSize);
    }

    /** Positions the knob's hit area from a centre point, sized to cover the tick ring too. */
    void setCentrePosition (juce::Point<float> centre);

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    ReflectTheme::Layout::KnobSize knobSize;
    ReflectTheme::Layout::KnobScale knobScale;
    bool fineDragActive = false;

    /** Full 0-1 travel in pixels of vertical drag, and the multiplier applied while shift is
        held. design/README.md section 7 specifies 180px; the x0.25 fine mode is this build's
        addition, per the same section's open items. */
    static constexpr int coarseDragPixels = 180;
    static constexpr int fineDragPixels = 720;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReflectKnob)
};
