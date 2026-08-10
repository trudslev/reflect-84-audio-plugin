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
    //
    // **It is an ELLIPSE, not a circle** - 120% of the width across but only 90% of the HEIGHT
    // down, and on a 1340 x 645 panel those are wildly different distances. The stop at 60% is
    // where the sheen has fully faded, so both radii are scaled by it:
    //
    //     rx = 1340 * 1.20 * 0.60 = 965      ry = 645 * 0.90 * 0.60 = 348
    //
    // Drawn as a circle it was 965 in both directions - the highlight reached nearly three times
    // too far down the panel, so instead of a distinct sheen across the top the whole fascia came
    // out uniformly lighter and read as flat. Measured against the artwork it ran +14 to +17 too
    // bright through the middle rows while the top edge matched, which is the signature of a
    // highlight that has not fallen off rather than one that is missing.
    //
    // JUCE's radial ColourGradient is circular, so the ellipse comes from drawing that circle under
    // a y-scale about the gradient's own centre.
    const float cx = (float) w * 0.20f;
    const float cy = 0.0f;
    const float rx = (float) w * 1.20f * 0.60f;
    const float ry = (float) h * 0.90f * 0.60f;

    {
        juce::Graphics::ScopedSaveState save { g };
        g.addTransform (juce::AffineTransform::scale (1.0f, ry / rx, cx, cy));

        juce::ColourGradient sheen { juce::Colours::white.withAlpha (0.5f), cx, cy,
                                     juce::Colours::white.withAlpha (0.0f), cx + rx, cy, true };
        g.setGradientFill (sheen);

        // Filled in the PRE-transform space, so the rect has to be tall enough that it still covers
        // the image after the y-scale shrinks it.
        g.fillRect (juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h * rx / ry));
    }
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
    // Three vertical dividers now, for four columns.
    Paint::drawVerticalDivider (g, Layout::divider1X, Layout::dividerTop, Layout::dividerBottom);
    Paint::drawVerticalDivider (g, Layout::divider2X, Layout::dividerTop, Layout::dividerBottom);
    Paint::drawVerticalDivider (g, Layout::divider3X, Layout::dividerTop, Layout::dividerBottom);

    // The horizontal rule lives in COLUMN 1 now, separating ALGORITHM from the damping pair that
    // moved in beside it - not in the tank column where it used to sit.
    Paint::drawHorizontalDivider (g, Layout::col1X, Layout::leftDividerY, Layout::col1W);
}

void PanelBackground::paintSectionLabels (juce::Graphics& g)
{
    Paint::drawSectionPill (g, "REVERB TANK", Layout::col2Centre, Layout::tankPillY);
    Paint::drawSectionPill (g, "DAMPING",     Layout::col1Centre, Layout::dampingPillY);
    Paint::drawSectionPill (g, "CHARACTER",   Layout::col3Centre, Layout::characterPillY);
    Paint::drawSectionPill (g, "OUTPUT",      Layout::col4Centre, Layout::outputPillY);

    // ALGORITHM caption, 16px below the rotary.
    Text::drawTracked (g, "ALGORITHM", Font::mono (Layout::algoCaptionSize),
                       Font::trackingPx (0.26f, Layout::algoCaptionSize),
                       { Layout::col1X, Layout::algoCaptionY, Layout::col1W, 12.0f },
                       juce::Justification::centred, Colour::textMuted);

    // DAMPING is drawn as a section pill above, alongside the other three. In v1.0 it was a 9px
    // label set to the LEFT of its knob pair, because the pair lived inside the tank column and had
    // no room above them; giving damping its own column in v1.1 removed that constraint, so it now
    // reads as the section it always was.

    // Version stamp, bottom-right of the OUTPUT column - BRAND.md's "came with a printed manual"
    // detail.
    {
        const auto font = Font::mono (Layout::versionSize);
        const float tracking = Font::trackingPx (Layout::versionTracking, Layout::versionSize);

        Text::drawTracked (g, "v" NF_VERSION_SHORT, font, tracking,
                           { Layout::versionRight - 100.0f, Layout::versionY, 100.0f, 13.0f },
                           juce::Justification::right, Colour::textTertiary);
    }
}

void PanelBackground::paintKnobLabels (juce::Graphics& g)
{
    for (const auto& spec : Layout::knobs)
    {
        const auto& v = Layout::variantFor (spec.size);

        // --- Printed scale: one numeral per tick, plus the unit ------------------------------
        //
        // These replace the standing readouts, which makes them functional text rather than
        // decoration - 10px at 7.62:1, per GUI-SPEC.md section 7 and BRAND.md's Legibility floor.
        //
        // Each numeral is centred on the point at the numeral radius and its own tick angle, then
        // centred on its own box. Drawing them here rather than in the knob component keeps them
        // out of the repaint that follows the pointer: a printed scale never changes.
        {
            const auto scaleFont = Font::mono (Layout::scaleNumeralSize);

            for (int i = 0; i < spec.scale.count; ++i)
            {
                const auto& mark = spec.scale.marks[i];
                const float angle = Layout::knobArcStartDegrees
                                  + mark.f * (Layout::knobArcEndDegrees - Layout::knobArcStartDegrees);

                const auto at = Geometry::pointOnCircle ({ spec.centreX, spec.centreY },
                                                         v.numeralRadius, angle);

                g.setFont (scaleFont);
                g.setColour (Colour::scaleNumeral);
                g.drawText (Text::withRealMinus (mark.printed),
                            juce::Rectangle<float> (at.x - 30.0f, at.y - 8.0f, 60.0f, 16.0f),
                            juce::Justification::centred, false);
            }

            // The unit prints ONCE, in the 90-degree gap at the bottom of the arc between the two
            // end numerals - never appended to the control name, and never repeated on every
            // numeral. SIZE and DIGITAL GRAIN carry none by design.
            if (spec.scale.unit != nullptr)
            {
                g.setFont (Font::mono (Layout::scaleUnitSize));
                g.setColour (Colour::scaleNumeral);
                g.drawText (spec.scale.unit,
                            juce::Rectangle<float> (spec.centreX - 30.0f,
                                                    spec.centreY + v.unitDrop - 7.0f, 60.0f, 14.0f),
                            juce::Justification::centred, false);
            }
        }

        const auto labelFont = Font::mono (v.labelSize);
        const float labelTracking = Font::trackingPx (v.labelTracking, v.labelSize);

        // Below the NUMERALS, not below the body. The printed scale now sits outside the body at
        // the numeral radius, so a label placed at radius + gap lands on top of the bottom numerals
        // - which is exactly what it did. Section 5 puts the label "centred below the cell", and
        // numeralRadius + gap is that same row expressed from the pivot rather than from a cell
        // height, so the two cannot drift apart.
        const float labelTop = spec.centreY + v.numeralRadius + Layout::knobLabelGap;

        Text::drawTracked (g, spec.label, labelFont, labelTracking,
                           { spec.centreX - 100.0f, labelTop, 200.0f, Layout::knobLabelLineHeight },
                           juce::Justification::centred,
                           // The two CHARACTER knobs are the panel's primary controls and carry
                           // the darker text weight; everything else is secondary.
                           spec.size == Layout::KnobSize::large ? Colour::textPrimary
                                                                : Colour::textSecondary);
    }
}
