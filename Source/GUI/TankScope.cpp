#include "TankScope.h"

#include "../PluginProcessor.h"
#include "../DSP/GrainSpec.h"

#include <cmath>

using namespace ReflectTheme;

namespace
{
    /** Seeded once so the hairlines are identical on every instance and every launch - they are
        meant to read as a fixed diffuse tail behind the envelope, not as animated noise. */
    constexpr juce::int64 noiseSeed = 0x5CE07A5CE07AULL;
}

juce::Rectangle<int> TankScope::canvasBounds()
{
    return { (int) Layout::centreColumnX,
             (int) Layout::scopeHeaderY,
             (int) Layout::centreColumnW,
             (int) (Layout::scopeBezelY + Layout::scopeBezelH - Layout::scopeHeaderY) };
}

TankScope::TankScope (Reflect84AudioProcessor& processor)
    : processorRef (processor)
{
    setInterceptsMouseClicks (false, false);
    setBounds (canvasBounds());

    juce::Random random { noiseSeed };

    for (auto& n : noise)
        n = 0.35f + random.nextFloat() * 0.65f;

    startTimerHz (Layout::scopeTimerHz);
}

TankScope::~TankScope()
{
    stopTimer();
}

void TankScope::timerCallback()
{
    phase = std::fmod (phase + Layout::scopePhaseStep, 1.0f);

    // The lamp follows the tank's own energy with a little smoothing, so it fades out with the
    // tail rather than flickering with individual blocks.
    const float target = processorRef.getTankEnergy();
    lampLevel += 0.35f * (target - lampLevel);

    repaint();
}

juce::Rectangle<float> TankScope::screenContentArea() const
{
    // The screen div carries a 1px border, so its content box is inset by one pixel all round.
    return { Layout::scopeScreenX - (float) getX() + 1.0f,
             Layout::scopeScreenY - (float) getY() + 1.0f,
             Layout::scopeScreenW - 2.0f,
             Layout::scopeScreenH - 2.0f };
}

juce::Point<float> TankScope::fromViewBox (float x, float y) const
{
    const auto area = screenContentArea();

    return { area.getX() + x * (area.getWidth()  / Layout::scopeViewBoxW),
             area.getY() + y * (area.getHeight() / Layout::scopeViewBoxH) };
}

//==============================================================================
void TankScope::paint (juce::Graphics& g)
{
    const float decayNorm = processorRef.apvts.getRawParameterValue (ParamIDs::decay)->load();

    paintHeader (g, ParamFormat::decaySeconds (decayNorm));
    paintScreen (g);
}

void TankScope::paintHeader (juce::Graphics& g, float decaySeconds)
{
    const juce::Rectangle<float> lamp { Layout::ledX - (float) getX(),
                                        Layout::ledY - (float) getY(),
                                        Layout::ledSize, Layout::ledSize };

    // Glow first, behind the bulb: `0 0 14px 3px <accent @45%>`. A plain two-stop radial
    // interpolates alpha linearly and reads as a hard-edged disc, so the falloff is shaped with
    // intermediate stops - TapeRot's FailLamp learned the same lesson.
    {
        const auto centre = lamp.getCentre();
        const float glow = Layout::ledGlowRadius;

        // Floored rather than allowed to reach zero: a rack unit's tank lamp is a powered
        // indicator that brightens with activity, not a bulb that goes out. A fully dark LED on
        // an idle panel reads as a fault.
        const float intensity = juce::jlimit (0.30f, 1.0f, lampLevel);

        juce::ColourGradient halo { Colour::accent.withAlpha (0.45f * intensity), centre.x, centre.y,
                                    Colour::accent.withAlpha (0.0f), centre.x + glow, centre.y, true };
        halo.addColour (0.25, Colour::accent.withAlpha (0.28f * intensity));
        halo.addColour (0.50, Colour::accent.withAlpha (0.14f * intensity));
        halo.addColour (0.75, Colour::accent.withAlpha (0.04f * intensity));

        g.setGradientFill (halo);
        g.fillEllipse (centre.x - glow, centre.y - glow, glow * 2.0f, glow * 2.0f);
    }

    // Bulb: radial-gradient(circle at 40% 32%, <accent lightened>, <accent> 70%, #2a3a24)
    {
        const auto lit = Colour::accent.interpolatedWith (juce::Colours::white, 0.15f);
        const float intensity = juce::jlimit (0.55f, 1.0f, lampLevel);
        const auto core = juce::Colour (0xFF2A3A24).interpolatedWith (lit, intensity);
        const auto edge = juce::Colour (0xFF2A3A24).interpolatedWith (Colour::accent, intensity);

        juce::ColourGradient bulb { core, lamp.getX() + lamp.getWidth() * 0.40f,
                                          lamp.getY() + lamp.getHeight() * 0.32f,
                                    juce::Colour (0xFF2A3A24),
                                    lamp.getRight(), lamp.getBottom(), true };
        bulb.addColour (0.70, edge);

        g.setGradientFill (bulb);
        g.fillEllipse (lamp);

        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawEllipse (lamp.reduced (0.5f).translated (0.0f, 0.5f), 1.0f);
    }

    // TANK LIVE
    Text::drawTracked (g, "TANK LIVE",
                       Font::mono (Layout::ledLabelSize),
                       Font::trackingPx (Layout::ledLabelTracking, Layout::ledLabelSize),
                       { Layout::ledLabelX - (float) getX(), 0.0f, 200.0f, Layout::scopeHeaderH },
                       juce::Justification::centredLeft, Colour::textPrimary);

    // DECAY TAIL . RT60 x.x s . 200 ms / DIV, right-aligned with 18px gaps.
    {
        const auto font = Font::mono (Layout::scopeHeaderTextSize);
        const float tracking = Font::trackingPx (Layout::scopeHeaderTracking, Layout::scopeHeaderTextSize);

        const std::array<juce::String, 3> parts {
            "DECAY TAIL",
            "RT60 " + juce::String (decaySeconds, 1) + " s",
            "200 ms / DIV"
        };

        float right = (float) getWidth();

        for (int i = (int) parts.size() - 1; i >= 0; --i)
        {
            const float width = Text::trackedWidth (parts[(size_t) i], font, tracking);

            Text::drawTracked (g, parts[(size_t) i], font, tracking,
                               { right - width, 0.0f, width, Layout::scopeHeaderH },
                               juce::Justification::left, Colour::textTertiary);

            right -= width + Layout::scopeHeaderGap;
        }
    }
}

void TankScope::paintScreen (juce::Graphics& g)
{
    // --- Bezel ---------------------------------------------------------------
    const juce::Rectangle<float> bezel { Layout::scopeBezelX - (float) getX(),
                                         Layout::scopeBezelY - (float) getY(),
                                         Layout::scopeBezelW, Layout::scopeBezelH };

    g.setGradientFill (Paint::verticalGradient (bezel, Colour::scopeBezelTop, Colour::scopeBezelBottom));
    g.fillRoundedRectangle (bezel, Layout::scopeBezelRadius);

    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawLine (bezel.getX() + 2.0f, bezel.getBottom() + 0.5f,
                bezel.getRight() - 2.0f, bezel.getBottom() + 0.5f, 1.0f);

    // --- Screen --------------------------------------------------------------
    const juce::Rectangle<float> screen { Layout::scopeScreenX - (float) getX(),
                                          Layout::scopeScreenY - (float) getY(),
                                          Layout::scopeScreenW, Layout::scopeScreenH };

    g.setGradientFill (Paint::verticalGradient (screen, Colour::screenTop, Colour::screenBottom));
    g.fillRoundedRectangle (screen, Layout::scopeScreenRadius);

    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (screen, Layout::scopeScreenRadius, 1.0f);

    juce::Graphics::ScopedSaveState save { g };
    {
        juce::Path clip;
        clip.addRoundedRectangle (screen.reduced (1.0f), Layout::scopeScreenRadius);
        g.reduceClipRegion (clip);
    }

    const auto area = screenContentArea();

    // --- Grid, INSIDE THE PLOT REGION ONLY -----------------------------------
    // GUI-SPEC.md section 11: the grid stops at x = 520 and does not run under the level gutter's
    // legends. Drawn from the plot rectangle rather than the screen, so it cannot creep.
    {
        const auto plotTL = fromViewBox (Layout::scopePlotX, Layout::scopePlotY);
        const auto plotBR = fromViewBox (Layout::scopePlotX + Layout::scopePlotW,
                                         Layout::scopePlotY + Layout::scopePlotH);
        const juce::Rectangle<float> plot { plotTL, plotBR };

        g.setColour (Colour::scopeGrid);

        for (float x = plot.getX(); x < plot.getRight(); x += Layout::scopeGridPitchX)
            g.fillRect (x, plot.getY(), 1.0f, plot.getHeight());

        for (float y = plot.getY(); y < plot.getBottom(); y += Layout::scopeGridPitchY)
            g.fillRect (plot.getX(), y, plot.getWidth(), 1.0f);

        // Reservation hairlines - they make the reserved areas read as instrument furniture rather
        // than as empty space.
        g.setColour (Colour::scopeGrid.withMultipliedAlpha (2.0f));
        g.fillRect (area.getX(), plot.getY(), area.getWidth(), 1.0f);
        g.fillRect (plot.getRight(), plot.getY(), 1.0f, plot.getHeight());
    }

    // --- Parameter snapshot --------------------------------------------------
    const auto valueOf = [this] (const char* id)
    {
        return processorRef.apvts.getRawParameterValue (id)->load();
    };

    const float decayNorm = valueOf (ParamIDs::decay);
    const float preNorm = valueOf (ParamIDs::preDelay);
    const float densityNorm = valueOf (ParamIDs::density);
    const float modNorm = valueOf (ParamIDs::modulation);

    const auto grain = GrainSpec::fromNormalized (valueOf (ParamIDs::grain));

    const float decaySeconds = ParamFormat::decaySeconds (decayNorm);
    const float preMs = (float) juce::roundToInt (ParamFormat::preDelayMs (preNorm));

    // Everything below is in the PLOT region, not the screen. -60 dB is the baseline at y = 156,
    // 0 dB is y = 26, so full scale is 130 - and the time axis spans the plot's 520 rather than the
    // screen's 600, or a long tail would run into the level gutter it is annotated by.
    constexpr float W = Layout::scopePlotW;
    constexpr float baseline = Layout::scopeMinusSixtyDbY;
    constexpr float fullScale = Layout::scopeMinusSixtyDbY - Layout::scopeZeroDbY;

    const float preX = (preMs / 1000.0f) * (W / Layout::scopeTimeSpanSeconds);
    const float tau = decaySeconds / Layout::scopeTauDivisor;
    const float sweep = Layout::scopeSweepOrigin + phase * (W + Layout::scopeSweepOvershoot);

    // --- Noise tail, behind the trace ----------------------------------------
    // BRAND.md asks for the underlying process rendered subtly behind the primary trace so the
    // display reads as a real diagnostic instrument, and design/README.md section 6 specifies it
    // as 240 fixed hairlines. It is drawn well below the quoted .16 alpha because the prototype
    // cannot arbitrate the weight: its own tail is an SVG path of zero-area M/L segments with a
    // `fill` and no `stroke`, so it renders as nothing at all, and the approved screenshot shows
    // no hatch. At the literal alpha and the specified 2.5-unit spacing the hairlines merge into
    // a wash that competes with the envelope; this keeps the texture and loses the competition.
    {
        g.setColour (Colour::scopeNoiseTail.withMultipliedAlpha (0.45f));

        const float visibleRight = juce::jmin (sweep, W);

        for (size_t i = 0; i < noise.size(); ++i)
        {
            const float x = ((float) i / (float) noise.size()) * W;

            if (x < preX || x > visibleRight)
                continue;

            const float t = ((x - preX) / W) * Layout::scopeTimeSpanSeconds;
            const float env = std::exp (-t / (tau * Layout::scopeTauScale))
                            * noise[i] * (0.5f + densityNorm * 0.5f);

            const float y = baseline - juce::jmin (1.0f, env) * fullScale;

            const auto from = fromViewBox (x, baseline);
            const auto to   = fromViewBox (x, y);

            // Sub-pixel hairlines at the dead end of the tail would otherwise anti-alias into a
            // solid 1px band running the full width of the screen.
            if (from.y - to.y < 1.0f)
                continue;

            g.drawLine (from.x, from.y, to.x, to.y, 1.0f);
        }
    }

    // --- Envelope trace ------------------------------------------------------
    std::vector<juce::Point<float>> points;
    points.reserve (512);
    points.push_back ({ 0.0f, baseline });
    points.push_back ({ preX, baseline });

    for (float x = preX; x <= W; x += grain.stepPx)
    {
        const float t = ((x - preX) / W) * Layout::scopeTimeSpanSeconds;

        float env = std::exp (-t / (tau * Layout::scopeTauScale));
        env *= 1.0f + modNorm * 0.10f * std::sin (x * 0.075f + phase * 12.0f);
        env *= 0.55f + densityNorm * 0.45f;
        env = grain.quantizeEnvelope (env);
        env = juce::jlimit (0.0f, 1.0f, env);

        const float y = baseline - env * fullScale;

        // The extra point at the PREVIOUS height before each new x is what turns the curve into a
        // stair rather than a sloped polyline - a sample-and-hold trace, exactly what the tank's
        // own quantiser is doing to the tail.
        if (grain.active && points.size() > 2)
            points.push_back ({ x, points.back().y });

        points.push_back ({ x, y });
    }

    // The refresh sweep clips the trace, so it paints in and re-triggers like a real scope.
    std::vector<juce::Point<float>> visible;
    visible.reserve (points.size());

    for (const auto& p : points)
        if (p.x <= sweep)
            visible.push_back (fromViewBox (p.x, p.y));

    if (visible.size() < 2)
        return;

    juce::Path trace;
    trace.startNewSubPath (visible.front());

    for (size_t i = 1; i < visible.size(); ++i)
        trace.lineTo (visible[i]);

    // Fill under the trace: accent at 16%.
    {
        juce::Path fill = trace;
        fill.lineTo (visible.back().x, fromViewBox (0.0f, baseline).y);
        fill.lineTo (fromViewBox (0.0f, baseline));
        fill.closeSubPath();

        g.setColour (Colour::accent.withAlpha (Layout::scopeTraceFillAlpha));
        g.fillPath (fill);
    }

    // Phosphor: the design stacks a wide, semi-transparent pass through a Gaussian bloom under a
    // crisp core. JUCE has no cheap per-path blur, so the bloom is approximated by widening
    // strokes at falling alpha - the same trick TapeRot's Scope uses, one pass deeper.
    const juce::PathStrokeType::JointStyle joint = juce::PathStrokeType::mitered;

    const std::array<std::pair<float, float>, 3> bloom { {
        { Layout::scopeBloomWidth * 3.1f, 0.10f },
        { Layout::scopeBloomWidth * 1.9f, 0.16f },
        { Layout::scopeBloomWidth,        Layout::scopeBloomAlpha * 0.40f },
    } };

    for (const auto& [width, alpha] : bloom)
    {
        g.setColour (Colour::accent.withAlpha (alpha));
        g.strokePath (trace, { width, joint, juce::PathStrokeType::butt });
    }

    g.setColour (Colour::accent);
    g.strokePath (trace, { Layout::scopeTraceWidth, joint, juce::PathStrokeType::butt });

    // --- Legends, in their RESERVED areas -------------------------------------
    // GUI-SPEC.md section 11: none of these are drawn in the plot region. DCY ENV and the grain
    // state live in the 20px title strip; the two level labels live in the 80px gutter, each
    // vertically centred on the level it annotates and tied to it by a leader tick.
    //
    // They used to sit in the corners of the screen with the trace free to run underneath. That
    // only looked safe because the reference render happened to show a short decay - at 200ms per
    // division a long tail reaches the right edge and settles near the baseline, exactly where the
    // -60 dB legend sat.
    {
        const auto font = Font::mono (Layout::scopeLegendSize);
        const float tracking = Font::trackingPx (Layout::scopeLegendTracking, Layout::scopeLegendSize);
        const float lineHeight = 12.0f;

        const auto stripLeft  = fromViewBox (9.0f, Layout::scopeTitleStripH * 0.5f);
        const auto stripRight = fromViewBox (Layout::scopeViewBoxW - 9.0f, Layout::scopeTitleStripH * 0.5f);

        Text::drawTracked (g, "DCY ENV", font, tracking,
                           { stripLeft.x, stripLeft.y - lineHeight * 0.5f, 200.0f, lineHeight },
                           juce::Justification::left, Colour::scopeLegend);

        Text::drawTracked (g, grain.describe(), font, tracking,
                           { stripRight.x - 300.0f, stripRight.y - lineHeight * 0.5f, 300.0f, lineHeight },
                           juce::Justification::right, Colour::scopeLegend);

        // Leader ticks: a short horizontal line from the plot's edge out to the label, so each
        // level label is visibly attached to the level it marks rather than floating near it.
        const auto leader = [&g, this] (float viewBoxY, juce::Colour colour)
        {
            const auto from = fromViewBox (Layout::scopeLeaderTickX0, viewBoxY);
            const auto to   = fromViewBox (Layout::scopeLeaderTickX1, viewBoxY);
            g.setColour (colour);
            g.drawLine (from.x, from.y, to.x, to.y, Layout::scopeLeaderTickWidth);
        };

        leader (Layout::scopeZeroDbY,       Colour::scopeLegend);
        leader (Layout::scopeMinusSixtyDbY, Colour::scopeLegendDim);

        const auto zeroAt  = fromViewBox (Layout::scopeGutterLabelX, Layout::scopeZeroDbY);
        const auto minusAt = fromViewBox (Layout::scopeGutterLabelX, Layout::scopeMinusSixtyDbY);

        Text::drawTracked (g, "0 dB", font, tracking,
                           { zeroAt.x, zeroAt.y - lineHeight * 0.5f, 120.0f, lineHeight },
                           juce::Justification::left, Colour::scopeLegend);

        Text::drawTracked (g, Text::minusSign() + juce::String ("60 dB"), font, tracking,
                           { minusAt.x, minusAt.y - lineHeight * 0.5f, 120.0f, lineHeight },
                           juce::Justification::left, Colour::scopeLegendDim);
    }
}
