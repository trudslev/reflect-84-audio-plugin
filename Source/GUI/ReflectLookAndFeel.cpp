#include "ReflectLookAndFeel.h"

using namespace ReflectTheme;

namespace
{
    /** design/README.md section 4's per-variant body gradients. Each is a CSS
        radial-gradient(circle at 50% N%, ...) with three stops; Paint::radialFace handles CSS's
        farthest-corner sizing rule. */
    juce::ColourGradient faceGradientFor (juce::Rectangle<float> box, Layout::KnobSize size)
    {
        switch (size)
        {
            case Layout::KnobSize::large:
                return Paint::radialFace (box, 0.5f, 0.22f,
                                          juce::Colour (0xFFFDF6E0),
                                          juce::Colour (0xFFDDCB98), 0.52f,
                                          juce::Colour (0xFFB09A61), 0.76f,
                                          juce::Colour (0xFF7D6A3B));

            case Layout::KnobSize::medium:
                return Paint::radialFace (box, 0.5f, 0.24f,
                                          juce::Colour (0xFFFBF3DA),
                                          juce::Colour (0xFFD6C391), 0.55f,
                                          juce::Colour (0xFFA58F58), 0.80f,
                                          juce::Colour (0xFF7A6738));

            case Layout::KnobSize::small:
                return Paint::radialFace (box, 0.5f, 0.24f,
                                          juce::Colour (0xFFF9F1D8),
                                          juce::Colour (0xFFD4C18E), 0.58f,
                                          juce::Colour (0xFF9F8A55), 0.82f,
                                          juce::Colour (0xFF77653C));

        }

        // Unreachable: the enum has exactly three members since GUI-SPEC.md section 2 retired the
        // tiny variant. Kept so the function has a return on every path.
        return Paint::radialFace (box, 0.5f, 0.24f,
                                  juce::Colour (0xFFF9F1D8),
                                  juce::Colour (0xFFD4C18E), 0.58f,
                                  juce::Colour (0xFF9F8A55), 0.82f,
                                  juce::Colour (0xFF77653C));
    }

    float tickAlphaFor (Layout::KnobSize size)
    {
        // Tick rings are quoted at .7 / .6 / .55 alpha, largest to smallest - bigger knobs carry
        // slightly firmer ticks. The retired tiny variant shared small's .55.
        switch (size)
        {
            case Layout::KnobSize::large:  return 0.70f;
            case Layout::KnobSize::medium: return 0.60f;
            case Layout::KnobSize::small:  break;
        }

        return 0.55f;
    }
}

//==============================================================================
ReflectLookAndFeel::ReflectLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Colour::fasciaMid);
    setColour (juce::Label::textColourId,                 Colour::textPrimary);

    // The knob drag popup reads TooltipWindow::textColourId, not a Slider or Label colour ID.
    setColour (juce::BubbleComponent::backgroundColourId, Colour::bezelBottom);
    setColour (juce::BubbleComponent::outlineColourId,    Colour::bezelGold);
    setColour (juce::TooltipWindow::textColourId,         Colour::phosphor);
    setColour (juce::TooltipWindow::backgroundColourId,   Colour::bezelBottom);
    setColour (juce::TooltipWindow::outlineColourId,      Colour::bezelGold);

    // PopupMenu: the Program menu drops out of the LCD, so it stays in the bezel's material.
    setColour (juce::PopupMenu::backgroundColourId,          Colour::bezelBottom);
    setColour (juce::PopupMenu::textColourId,                Colour::phosphor);
    setColour (juce::PopupMenu::headerTextColourId,          Colour::bezelGold);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colour::bezelTop);
    setColour (juce::PopupMenu::highlightedTextColourId,     Colour::wordmark);
}

juce::Font ReflectLookAndFeel::getPopupMenuFont()
{
    return Font::mono (13.0f);
}

void ReflectLookAndFeel::paintKnob (juce::Graphics& g,
                                    juce::Point<float> centre,
                                    Layout::KnobSize size,
                                    const Layout::KnobScale& scale,
                                    float value01)
{
    const auto& v = Layout::variantFor (size);
    const float r = v.radius;
    const juce::Rectangle<float> body { centre.x - r, centre.y - r, r * 2.0f, r * 2.0f };

    // 1. Tick ring, behind the body.
    Paint::drawTickRing (g, centre, r, v, scale, Colour::tick.withAlpha (tickAlphaFor (size)));

    // 2. Contact shadow. The design quotes `0 Npx Mpx rgba(45,33,12,.4-.45)` - a soft, slightly
    //    dropped shadow that reads as the cap sitting proud of the fascia.
    {
        juce::Path facePath;
        facePath.addEllipse (body);
        juce::DropShadow shadow { Colour::knobShadow, juce::roundToInt (r * 0.28f), { 0, juce::roundToInt (r * 0.13f) } };
        shadow.drawForPath (g, facePath);
    }

    // 3. Sculpted body: top-lit dome.
    g.setGradientFill (faceGradientFor (body, size));
    g.fillEllipse (body);

    // 4. Inner cap (large variant only) - `inset: 15px` with its own top sheen.
    if (v.innerCapInset > 0.0f)
    {
        const auto cap = body.reduced (v.innerCapInset);
        g.setGradientFill (Paint::radialFace (cap, 0.5f, 0.28f,
                                              juce::Colours::white.withAlpha (0.50f),
                                              juce::Colours::white.withAlpha (0.26f), 0.5f,
                                              Colour::knobInnerShade.withAlpha (0.14f), 0.8f,
                                              Colour::knobInnerShade.withAlpha (0.18f)));
        g.fillEllipse (cap);
    }

    // 5. Bottom-inner shading: `inset 0 -Npx Mpx rgba(90,70,30,.35)`.
    {
        juce::ColourGradient inner { Colour::knobInnerShade.withAlpha (0.0f), centre.x, centre.y - r,
                                     Colour::knobInnerShade, centre.x, centre.y + r, false };
        inner.addColour (0.62, Colour::knobInnerShade.withAlpha (0.0f));
        g.setGradientFill (inner);
        g.fillEllipse (body);
    }

    // 6. Pointer: a dark line from near the top edge inward, rotating with the value.
    {
        const float angle = Geometry::knobAngleForValue (value01);
        const float length = v.pointerLengthFraction * (r * 2.0f);
        const float outer = r - v.pointerTopInset;

        const auto from = Geometry::pointOnCircle (centre, outer, angle);
        const auto to   = Geometry::pointOnCircle (centre, outer - length, angle);

        g.setColour (v.innerCapInset > 0.0f ? Colour::pointerDarkLarge : Colour::pointerDark);
        g.drawLine ({ from, to }, v.pointerWidth);
    }
}

void ReflectLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float, float,
                                           juce::Slider& slider)
{
    // rotaryStartAngle/rotaryEndAngle are deliberately ignored: the design's travel arc is fixed
    // at -135..+135 degrees clockwise from 12 o'clock, which is not JUCE's convention, and
    // Geometry::knobAngleForValue is the single place that mapping lives.
    const auto* knob = dynamic_cast<const ReflectKnob*> (&slider);
    const auto size = knob != nullptr ? knob->size() : Layout::KnobSize::medium;
    const auto scale = knob != nullptr ? knob->scale() : Layout::KnobScale { nullptr, 0, nullptr };

    const juce::Rectangle<float> bounds { (float) x, (float) y, (float) width, (float) height };
    paintKnob (g, bounds.getCentre(), size, scale, sliderPosProportional);
}

//==============================================================================
ReflectKnob::ReflectKnob (Layout::KnobSize sizeVariant, Layout::KnobScale scaleForKnob)
    : juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox),
      knobSize (sizeVariant),
      knobScale (scaleForKnob)
{
    setRotaryParameters (juce::degreesToRadians (Layout::knobArcStartDegrees),
                         juce::degreesToRadians (Layout::knobArcEndDegrees),
                         true);
    setVelocityBasedMode (false);
    setMouseDragSensitivity (coarseDragPixels);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ReflectKnob::setCentrePosition (juce::Point<float> centre)
{
    // The hit area covers the tick ring as well as the body, so the whole visual control is
    // grabbable and the ring is inside this component's clip region rather than a neighbour's.
    // Sized to the NUMERAL radius, not the body or the tick ring: the printed scale sits outside
    // both, and a component sized to the ticks silently clips its own numerals away.
    const float half = variant().numeralRadius + variant().labelSize;
    setBounds (juce::Rectangle<float> (centre.x - half, centre.y - half, half * 2.0f, half * 2.0f)
                   .getSmallestIntegerContainer());
}

void ReflectKnob::mouseDown (const juce::MouseEvent& e)
{
    // Sensitivity has to be settled BEFORE Slider::mouseDown records its drag anchor: JUCE
    // measures the drag from that anchor and scales by the current sensitivity, so changing it
    // part-way through a drag rescales the distance already travelled and the value jumps.
    fineDragActive = e.mods.isShiftDown();
    setMouseDragSensitivity (fineDragActive ? fineDragPixels : coarseDragPixels);

    juce::Slider::mouseDown (e);
}

void ReflectKnob::mouseDrag (const juce::MouseEvent& e)
{
    juce::Slider::mouseDrag (e);
}
