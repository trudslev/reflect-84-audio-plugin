#include "AlgorithmSwitch.h"

using namespace ReflectTheme;

namespace
{
    constexpr float labelLineHeight = 12.5f;    // 10px * 1.25 line-height
    constexpr float labelInset = 3.0f;          // `right/left: calc(100% - 3px)`
    constexpr float labelVerticalInset = 4.0f;  // `bottom/top: calc(100% - 4px)`
    constexpr float labelHitPadding = 4.0f;
}

juce::Rectangle<int> AlgorithmSwitch::canvasBounds()
{
    // Wide enough for "DIGITAL"/"CHAMBER" on both sides and tall enough for the label rows above
    // and below, so the whole control - including its clickable legends - is one component.
    return juce::Rectangle<float> (Layout::leftColumnX,
                                   Layout::algoCentreY - Layout::algoRadius - 34.0f,
                                   Layout::leftColumnW,
                                   (Layout::algoRadius + 34.0f) * 2.0f).getSmallestIntegerContainer();
}

AlgorithmSwitch::AlgorithmSwitch()
    : juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox)
{
    setSliderSnapsToMousePosition (false);
    setBounds (canvasBounds());
    layOutLabels();
}

juce::Point<float> AlgorithmSwitch::knobCentre() const
{
    return { Layout::algoCentreX - (float) getX(), Layout::algoCentreY - (float) getY() };
}

int AlgorithmSwitch::selectedIndex() const
{
    return juce::jlimit (0, numAlgorithms - 1, juce::roundToInt (getValue()));
}

void AlgorithmSwitch::layOutLabels()
{
    const auto centre = knobCentre();
    const float r = Layout::algoRadius;

    const auto font = Font::mono (Layout::algoLabelSize);
    const float tracking = Font::trackingPx (0.18f, Layout::algoLabelSize);

    for (size_t i = 0; i < Layout::algorithmCorners.size(); ++i)
    {
        const auto& corner = Layout::algorithmCorners[i];

        const bool twoLines = corner.secondLine != nullptr;
        const float height = twoLines ? labelLineHeight * 2.0f : labelLineHeight;

        float width = Text::trackedWidth (corner.label, font, tracking);

        if (twoLines)
            width = juce::jmax (width, Text::trackedWidth (corner.secondLine, font, tracking));

        const bool onLeft = corner.corner == Layout::Corner::topLeft
                         || corner.corner == Layout::Corner::bottomLeft;
        const bool onTop  = corner.corner == Layout::Corner::topLeft
                         || corner.corner == Layout::Corner::topRight;

        // The design anchors each legend to the knob box's corner, overlapping it by a few px:
        // `right: calc(100% - 3px); bottom: calc(100% - 4px)` and its three mirrors.
        const float right = onLeft ? centre.x - r + labelInset : centre.x + r - labelInset + width;
        const float top   = onTop  ? centre.y - r + labelVerticalInset - height
                                   : centre.y + r - labelVerticalInset;

        labelHits[i] = { { right - width, top, width, height }, corner.index };
    }
}

int AlgorithmSwitch::labelIndexAt (juce::Point<float> position) const
{
    for (const auto& hit : labelHits)
        if (hit.area.expanded (labelHitPadding).contains (position))
            return hit.algorithmIndex;

    return -1;
}

bool AlgorithmSwitch::hitTest (int x, int y)
{
    const juce::Point<float> p { (float) x, (float) y };

    if (p.getDistanceFrom (knobCentre()) <= Layout::algoRadius)
        return true;

    return labelIndexAt (p) >= 0;
}

void AlgorithmSwitch::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position;

    if (const int index = labelIndexAt (p); index >= 0)
    {
        setValue ((double) index, juce::sendNotificationSync);
        return;
    }

    if (p.getDistanceFrom (knobCentre()) <= Layout::algoRadius)
    {
        // Click-to-nearest-detent rather than the prototype's blind advance: with four detents at
        // the diagonals, the pointer lands where the user aimed instead of one step clockwise.
        const auto delta = p - knobCentre();
        const float degrees = juce::radiansToDegrees (std::atan2 (delta.x, -delta.y));
        const float normalised = degrees < -180.0f + 45.0f ? degrees + 360.0f : degrees;

        const int nearest = juce::jlimit (0, numAlgorithms - 1,
                                          juce::roundToInt ((normalised + 45.0f) / 90.0f));

        setValue ((double) nearest, juce::sendNotificationSync);
    }
}

void AlgorithmSwitch::mouseMove (const juce::MouseEvent& e)
{
    // The component spans a region wider than the control itself, so the cursor has to be set per
    // move rather than once in the constructor.
    const bool overControl = e.position.getDistanceFrom (knobCentre()) <= Layout::algoRadius
                          || labelIndexAt (e.position) >= 0;

    setMouseCursor (overControl ? juce::MouseCursor::PointingHandCursor
                                : juce::MouseCursor::NormalCursor);
}

//==============================================================================
void AlgorithmSwitch::paint (juce::Graphics& g)
{
    const auto centre = knobCentre();
    const float r = Layout::algoRadius;
    const juce::Rectangle<float> body { centre.x - r, centre.y - r, r * 2.0f, r * 2.0f };

    const int selected = selectedIndex();

    // --- Corner labels -------------------------------------------------------
    {
        const auto font = Font::mono (Layout::algoLabelSize);
        const float tracking = Font::trackingPx (0.18f, Layout::algoLabelSize);

        for (size_t i = 0; i < Layout::algorithmCorners.size(); ++i)
        {
            const auto& corner = Layout::algorithmCorners[i];
            const auto& hit = labelHits[i];

            const bool isSelected = corner.index == selected;
            const auto colour = isSelected ? Colour::labelSelected : Colour::textFaint;

            const bool onLeft = corner.corner == Layout::Corner::topLeft
                             || corner.corner == Layout::Corner::bottomLeft;
            const auto justify = onLeft ? juce::Justification::right : juce::Justification::left;

            auto line = hit.area.withHeight (labelLineHeight);
            Text::drawTracked (g, corner.label, font, tracking, line, justify, colour);

            if (corner.secondLine != nullptr)
                Text::drawTracked (g, corner.secondLine, font, tracking,
                                   line.translated (0.0f, labelLineHeight), justify, colour);
        }
    }

    // --- Detent ticks, `inset: -15px`, starting at 224.45 degrees -------------
    // Four of them, one per detent, NOT a dense ring. The design doc's prose calls these "dense
    // conic ticks", but its own CSS repeats the 1.1-degree tick every 90 degrees
    // (`repeating-conic-gradient(from 224.45deg, <tick> 0deg 1.1deg, transparent 1.1deg 90deg)`)
    // and design/screenshots/01-panel.png shows exactly four dashes at the diagonals. The artwork
    // wins.
    {
        const float outer = r - Layout::algoTickInset;
        const float inner = outer - outer * 0.14f;

        g.setColour (Colour::tick.withAlpha (0.85f));

        for (int i = 0; i < numAlgorithms; ++i)
        {
            const float angle = Layout::algoTickStartDegrees + (float) i * (360.0f / (float) numAlgorithms);
            g.drawLine ({ Geometry::pointOnCircle (centre, inner, angle),
                          Geometry::pointOnCircle (centre, outer, angle) }, 1.6f);
        }
    }

    // --- Body: dark, matching the header bezel -------------------------------
    {
        juce::Path facePath;
        facePath.addEllipse (body);
        juce::DropShadow shadow { Colour::knobShadow, 12, { 0, 5 } };
        shadow.drawForPath (g, facePath);
    }

    g.setGradientFill (Paint::radialFace (body, 0.5f, 0.26f,
                                          Colour::algoFace0,
                                          Colour::algoFace1, 0.52f,
                                          Colour::algoFace2, 0.80f,
                                          Colour::algoFace3));
    g.fillEllipse (body);

    // Inner disc, `inset: 12px`, with a soft top sheen. Kept very low-contrast: it is a sheen on
    // a moulded surface, not a separate concentric part, and any visible edge on it reads as a
    // seam the reference artwork does not have.
    {
        const auto disc = body.reduced (12.0f);
        g.setGradientFill (Paint::radialFace (disc, 0.5f, 0.30f,
                                              juce::Colours::white.withAlpha (0.07f),
                                              juce::Colours::white.withAlpha (0.03f), 0.55f,
                                              juce::Colours::black.withAlpha (0.06f), 0.85f,
                                              juce::Colours::black.withAlpha (0.10f)));
        g.fillEllipse (disc);
    }

    // inset 0 1px 1px rgba(255,255,255,.16) - the lit upper lip
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawEllipse (body.reduced (0.5f).translated (0.0f, 0.5f), 1.0f);

    // --- Pointer: brass, from 9px inside the top edge --------------------------
    {
        // Detents at the four diagonals: -45, 45, 135, 225 degrees.
        const float angle = -45.0f + (float) selected * 90.0f;
        const float outer = r - Layout::algoPointerTopInset;
        const float length = Layout::algoPointerLengthFraction * r * 2.0f;

        const auto from = Geometry::pointOnCircle (centre, outer, angle);
        const auto to   = Geometry::pointOnCircle (centre, outer - length, angle);

        g.setGradientFill ({ Colour::algoPointerTop, from.x, from.y,
                             Colour::algoPointerBottom, to.x, to.y, false });

        juce::Path pointer;
        pointer.addLineSegment ({ from, to }, Layout::algoPointerWidth);
        g.fillPath (pointer);
    }
}
