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
    void mouseDrag (const juce::MouseEvent&) override {}
    void mouseUp (const juce::MouseEvent&) override {}
    void mouseMove (const juce::MouseEvent& e) override;

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
