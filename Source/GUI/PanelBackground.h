#pragma once

#include "ReflectTheme.h"

/**
    Everything static on the panel: the fascia material and its overlay texture, the header bezel,
    the wordmark and taglines, section pills, engraved dividers, every knob's name label, the
    DAMPING caption, the ALGORITHM caption, and the version stamp.

    No interactive elements and no live values - it is painted once per repaint and never polls
    anything, so it can sit at the bottom of the z-order and be ignored by the mouse.
*/
class PanelBackground final : public juce::Component
{
public:
    PanelBackground();

    void paint (juce::Graphics& g) override;

    /** The texture overlay, drawn by the EDITOR after every child rather than by this component.

        Section 1 calls it "full bleed", and it means it: the scanlines and the sheen sit over the
        header bezel and the controls, not just the bare fascia. Drawn inside paint() it was covered
        by the very next thing painted - the header - which is where the effect is most visible,
        because white lifts a #22304c bezel far harder than an already-near-white fascia. */
    void paintTextureOverlay (juce::Graphics& g) const;

private:
    void paintFascia (juce::Graphics& g);
    void paintHeader (juce::Graphics& g);
    void paintDividers (juce::Graphics& g);
    void paintSectionLabels (juce::Graphics& g);
    void paintKnobLabels (juce::Graphics& g);

    /** The scanline + sheen overlay, rasterised once at construction.

        design/README.md's Canvas section specifies 1px horizontal scanlines every 3px plus a
        top-left radial sheen, full-bleed. At 1200x615 that is 205 lines, and redrawing them as
        fill calls on every repaint is pure waste - TapeRot's SectionPanel::generateSpeckleImage
        establishes the pre-render-once pattern for exactly this. */
    juce::Image texture;

    void buildTexture();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelBackground)
};
