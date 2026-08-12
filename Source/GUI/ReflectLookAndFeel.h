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

    /** Full 0-1 travel in pixels of vertical drag, and the value while Shift is held.

        **190 / 760 is the suite figure, not this casting's.** Six castings had six drag feels -
        JUCE's untouched 250 in two, 200, 180, and 190 in two - so the same hand got a different
        response from each. Nothing about any casting's identity argues for that, which made it the
        clearest accidental drift in the audit and the cheapest to fix. 190 is the plurality.

        The 4x ratio is this casting's contribution kept whole: Reflect-84 was the only one with a
        fine mode, and it is behaviour rather than appearance - someone who learns Shift on one
        casting expects it on the next - so it went to all six and 180/720 moved to 190/760 rather
        than the ratio bending to fit. GUI-SPEC section 7's 180 is superseded by that ruling. */
    static constexpr int coarseDragPixels = 190;
    static constexpr int fineDragPixels = 760;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReflectKnob)
};
