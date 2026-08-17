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

    /** The knob's STATIC layers — tick ring, contact shadow, body, inner cap, inner shading.
        Everything that does not move with the value, so it can be rendered once and blitted. */
    static void paintKnobStatic (juce::Graphics& g,
                                 juce::Point<float> centre,
                                 ReflectTheme::Layout::KnobSize size,
                                 const ReflectTheme::Layout::KnobScale& scale);

    /** The one layer that does move. */
    static void paintKnobPointer (juce::Graphics& g,
                                  juce::Point<float> centre,
                                  ReflectTheme::Layout::KnobSize size,
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

    /** **Draws the cached static layer and the live pointer, rather than going through
        `drawRotarySlider`.**

        Call 5 asks for the code-drawn knobs to be cached. `setBufferedToImage` is the obvious
        reading and it is the wrong one: JUCE refreshes that buffer on every `repaint()`, and a
        Slider repaints on every value change, so the whole knob would re-render on every drag frame
        and the cache would save nothing at all. It would also be invisible — the panel would look
        identical and profile identically, which is the failure mode worth naming.

        So the split is by what actually changes. The tick ring, shadow, body, inner cap and inner
        shading depend only on the size class and the scale, so they are rendered once into an image
        at the current device scale and blitted. The pointer is three lines of arithmetic and is
        drawn live.

        **The cache is keyed on the device scale, not on the value**, which is what makes it a cache
        rather than a buffer: it is rebuilt when the editor is resized and at no other time. A resize
        therefore costs one uncached paint per knob and every frame after it is a blit. */
    void paint (juce::Graphics& g) override;

    /** Test seam for the cache; see `staticLayerBuilds`. */
    int staticLayerBuildCount() const noexcept { return staticLayerBuilds; }

    const ReflectTheme::Layout::KnobScale& scale() const noexcept { return knobScale; }

    ReflectTheme::Layout::KnobSize size() const noexcept { return knobSize; }

    juce::Image staticLayer;

    /** **How many times the static layer has been rendered**, so the cache can be shown to be one.

        Without it, a cache that rebuilds on every frame is indistinguishable from one that never
        does: the panel looks identical either way and the only difference is a cost nothing
        headless measures. This makes the distinguishing property assertable — many values at one
        scale rebuild once, a scale change rebuilds again. */
    int staticLayerBuilds = 0;

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
