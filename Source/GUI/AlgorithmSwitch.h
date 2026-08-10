#pragma once

#include "../Parameters.h"
#include "ReflectTheme.h"

/**
    The ALGORITHM rotary: a 4-position detented switch, not a continuous knob.

    Subclasses juce::Slider purely so it stays SliderAttachment-compatible with the Algorithm
    choice parameter while painting as a dark bezel-matching rotary with four corner labels -
    the same trick TapeRot's NoiseCharacterSwitch uses.

    Deliberately not drawn through ReflectLookAndFeel: design/README.md section 2 makes this the
    one "system" control among brass knobs, with its own face, its own dense tick ring starting at
    224.45 degrees, and 90-degree detents rather than a 270-degree travel arc.

    The corner labels are NOT in clockwise order. PLATE is top-left (index 0), DIGITAL ROOM
    top-right (1), CHAMBER bottom-right (2), HALL bottom-left (3). The design doc calls this out
    explicitly as a trap; ReflectTheme::Layout::algorithmCorners is the single table joining panel
    position to DSP enum value, and neither is ever derived from the other.
*/
class AlgorithmSwitch final : public juce::Slider
{
public:
    AlgorithmSwitch();

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;

    /** **Rotation order is not enum order.** Turning the knob sweeps HALL - PLATE - DIGITAL ROOM -
        CHAMBER, the corners read clockwise from bottom-left; the DSP enum is Plate 0, Digital Room
        1, Chamber 2, Hall 3. Same cycle, different starting point, so the two are joined by these
        and neither is derived from the other - the rule algorithmCorners already follows for panel
        POSITION.

        Overriding the two proportion hooks rather than reordering anything keeps the Slider's value
        equal to the parameter index, so the SliderAttachment needs no change, while drag and
        pointer both run in rotation order and clamp at HALL and CHAMBER instead of wrapping. */
    double valueToProportionOfLength (double value) override
    {
        return rotationOf ((int) value) / 3.0;
    }

    double proportionOfLengthToValue (double proportion) override
    {
        return enumAtRotation (juce::roundToInt (juce::jlimit (0.0, 1.0, proportion) * 3.0));
    }

    static int rotationOf (int algorithmIndex) noexcept { return (algorithmIndex + 1) % 4; }
    static int enumAtRotation (int rotation) noexcept   { return (rotation + 3) % 4; }

    /** Bounds covering the rotary, its tick ring and all four corner labels. */
    static juce::Rectangle<int> canvasBounds();

private:
    struct LabelHit
    {
        juce::Rectangle<float> area;
        int algorithmIndex;
    };

    std::array<LabelHit, 4> labelHits {};

    int selectedIndex() const;
    juce::Point<float> knobCentre() const;

    void layOutLabels();
    int labelIndexAt (juce::Point<float> position) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AlgorithmSwitch)
};
