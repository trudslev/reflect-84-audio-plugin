#include "PanelBackground.h"

using namespace ReflectTheme;

PanelBackground::PanelBackground()
{
    setInterceptsMouseClicks (false, false);
    buildTexture();
}

//==============================================================================
void PanelBackground::buildTexture()
{
    const int w = (int) Layout::canvasWidth;
    const int h = (int) Layout::canvasHeight;

    texture = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g { texture };

    // repeating-linear-gradient(0deg, rgba(80,60,25,.035) 0 1px, transparent 1px 3px)
    g.setColour (Colour::scanline);

    for (int y = 0; y < h; y += (int) Layout::scanlinePitch)
        g.fillRect (0, y, w, 1);

    // radial-gradient(120% 90% at 20% 0%, rgba(255,255,255,.5), transparent 60%)
    // The stop at 60% is where the sheen has fully faded, so the gradient's own end point sits
    // there rather than at the ellipse's edge.
    const float cx = (float) w * 0.20f;
    const float cy = 0.0f;
    const float radius = (float) w * 1.20f * 0.60f;

    juce::ColourGradient sheen { juce::Colours::white.withAlpha (0.5f), cx, cy,
                                 juce::Colours::white.withAlpha (0.0f), cx + radius, cy, true };
    g.setGradientFill (sheen);
    g.fillRect (0, 0, w, h);
}

//==============================================================================
void PanelBackground::paint (juce::Graphics& g)
{
    paintFascia (g);
    paintHeader (g);
    paintDividers (g);
    paintSectionLabels (g);
    paintKnobLabels (g);
}

void PanelBackground::paintFascia (juce::Graphics& g)
{
    const juce::Rectangle<float> panel { 0.0f, 0.0f, Layout::canvasWidth, Layout::canvasHeight };

    auto fascia = Paint::verticalGradient (panel, Colour::fasciaTop, Colour::fasciaBottom);
    fascia.addColour (0.60, Colour::fasciaMid);
    g.setGradientFill (fascia);
    g.fillRoundedRectangle (panel, Layout::panelRadius);

    // The overlay texture is full-bleed but must not spill past the rounded corners.
    {
        juce::Graphics::ScopedSaveState save { g };
        juce::Path clip;
        clip.addRoundedRectangle (panel, Layout::panelRadius);
        g.reduceClipRegion (clip);
        g.drawImageAt (texture, 0, 0);
    }

    // inset 0 1px 0 rgba(255,255,255,.75) - the lit top edge of the moulding
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.drawLine (Layout::panelRadius, 0.5f, Layout::canvasWidth - Layout::panelRadius, 0.5f, 1.0f);

    // inset 0 -2px 6px rgba(90,70,40,.18) - the shaded bottom lip
    {
        const juce::Rectangle<float> lip { 0.0f, Layout::canvasHeight - 8.0f, Layout::canvasWidth, 8.0f };
        g.setGradientFill ({ Colour::panelInnerShade.withAlpha (0.0f), lip.getX(), lip.getY(),
                             Colour::panelInnerShade, lip.getX(), lip.getBottom(), false });
        g.fillRect (lip);
    }
}

void PanelBackground::paintHeader (juce::Graphics& g)
{
    const juce::Rectangle<float> bezel { Layout::headerX, Layout::headerY, Layout::headerW, Layout::headerH };
    Paint::drawBezelPlate (g, bezel, Layout::headerRadius);

    // --- Wordmark: etched/engraved metal legend. BRAND.md requires a nameplate metaphor distinct
    //     from TapeRot's Dymo label and Gatecrasher's stencil; here it is engraving, which is a
    //     dark line below the glyphs and a faint light line above, not a drop shadow.
    {
        const auto font = Font::wordmark (Layout::wordmarkSize);
        const float tracking = Font::trackingPx (0.10f, Layout::wordmarkSize);
        const juce::Rectangle<float> area { Layout::wordmarkX, Layout::wordmarkTop,
                                            600.0f, Layout::wordmarkLineHeight };

        Text::drawTracked (g, "REFLECT-84", font, tracking,
                           area.translated (0.0f, -1.0f), juce::Justification::centredLeft,
                           juce::Colours::white.withAlpha (0.18f));
        Text::drawTracked (g, "REFLECT-84", font, tracking,
                           area.translated (0.0f, 1.0f), juce::Justification::centredLeft,
                           juce::Colours::black.withAlpha (0.65f));
        Text::drawTracked (g, "REFLECT-84", font, tracking,
                           area, juce::Justification::centredLeft, Colour::wordmark);
    }

    // --- Taglines: real hardware-spec-sheet phrasing, per BRAND.md.
    {
        const auto font = Font::mono (Layout::taglineSize);
        const float tracking = Font::trackingPx (0.30f, Layout::taglineSize);

        const juce::Rectangle<float> line1 { Layout::wordmarkX, Layout::taglineTop,
                                             600.0f, Layout::taglineLineHeight };
        const auto line2 = line1.translated (0.0f, Layout::taglineLineHeight + Layout::taglineGap);

        Text::drawTracked (g, "REVERB PROCESSOR", font, tracking, line1,
                           juce::Justification::centredLeft, Colour::bezelLabel);
        Text::drawTracked (g, "MODEL RF-84 " + Text::middleDot() + " STEREO", font, tracking, line2,
                           juce::Justification::centredLeft, Colour::bezelLabel);
    }

    // --- PROGRAM caption
    Text::drawTracked (g, "PROGRAM", Font::mono (9.0f), Font::trackingPx (0.34f, 9.0f),
                       { Layout::programLabelX, Layout::programLabelY, 200.0f, Layout::programLabelH },
                       juce::Justification::centredLeft, Colour::bezelGold);

    // --- IN / OUT captions
    {
        const auto font = Font::mono (9.0f);
        const float tracking = Font::trackingPx (0.28f, 9.0f);

        Text::drawTracked (g, "IN", font, tracking,
                           { Layout::meterInX, Layout::meterLabelY, Layout::meterWellW, Layout::meterLabelH },
                           juce::Justification::centred, Colour::bezelLabel);
        Text::drawTracked (g, "OUT", font, tracking,
                           { Layout::meterOutX, Layout::meterLabelY, Layout::meterWellW, Layout::meterLabelH },
                           juce::Justification::centred, Colour::bezelLabel);
    }
}

void PanelBackground::paintDividers (juce::Graphics& g)
{
    Paint::drawVerticalDivider (g, Layout::divider1X, Layout::dividerTop, Layout::dividerBottom);
    Paint::drawVerticalDivider (g, Layout::divider2X, Layout::dividerTop, Layout::dividerBottom);
    Paint::drawHorizontalDivider (g, Layout::leftColumnX, Layout::leftDividerY, Layout::leftColumnW);
}

void PanelBackground::paintSectionLabels (juce::Graphics& g)
{
    Paint::drawSectionPill (g, "REVERB TANK", Layout::leftColumnCentre,   Layout::tankPillY);
    Paint::drawSectionPill (g, "CHARACTER",   Layout::centreColumnCentre, Layout::characterPillY);
    Paint::drawSectionPill (g, "OUTPUT",      Layout::rightColumnCentre,  Layout::outputPillY);

    // ALGORITHM caption, 16px below the rotary.
    Text::drawTracked (g, "ALGORITHM", Font::mono (Layout::algoCaptionSize),
                       Font::trackingPx (0.26f, Layout::algoCaptionSize),
                       { Layout::leftColumnX, Layout::algoCaptionY, Layout::leftColumnW, 12.0f },
                       juce::Justification::centred, Colour::textMuted);

    // DAMPING sits to the LEFT of its knob pair rather than above it.
    {
        const auto font = Font::mono (Layout::dampingLabelSize);
        const float tracking = Font::trackingPx (Layout::dampingLabelTracking, Layout::dampingLabelSize);
        const float width = Text::trackedWidth ("DAMPING", font, tracking);

        Text::drawTracked (g, "DAMPING", font, tracking,
                           { Layout::dampingLabelRight - width, Layout::dampingLabelY, width, 12.0f },
                           juce::Justification::centredLeft, Colour::textMuted);
    }

    // Version stamp, bottom-right of the OUTPUT column - BRAND.md's "came with a printed manual"
    // detail.
    {
        const auto font = Font::mono (Layout::versionSize);
        const float tracking = Font::trackingPx (Layout::versionTracking, Layout::versionSize);

        Text::drawTracked (g, "v" NF_VERSION_SHORT, font, tracking,
                           { Layout::versionRight - 100.0f, Layout::versionY, 100.0f, 13.0f },
                           juce::Justification::right, Colour::textFaint);
    }
}

void PanelBackground::paintKnobLabels (juce::Graphics& g)
{
    for (const auto& spec : Layout::knobs)
    {
        const auto& v = Layout::variantFor (spec.size);

        const auto labelFont = Font::mono (v.labelSize);
        const float labelTracking = Font::trackingPx (v.labelTracking, v.labelSize);

        const float labelTop = spec.centreY + v.radius + Layout::knobLabelGap;

        Text::drawTracked (g, spec.label, labelFont, labelTracking,
                           { spec.centreX - 100.0f, labelTop, 200.0f, Layout::knobLabelLineHeight },
                           juce::Justification::centred,
                           // The two CHARACTER knobs are the panel's primary controls and carry
                           // the darker text weight; everything else is secondary.
                           spec.size == Layout::KnobSize::large ? Colour::textPrimary
                                                                : Colour::textSecondary);
    }
}
