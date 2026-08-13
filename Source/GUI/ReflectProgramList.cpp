#include "ReflectProgramList.h"

using namespace ReflectTheme;

namespace
{
    constexpr float pi = 3.14159265358979323846f;
}

ReflectProgramList::ReflectProgramList()
{
    setWantsKeyboardFocus (false);
}

//==============================================================================
void ReflectProgramList::setPrograms (std::vector<ProgramId> programs,
                                      const ProgramId& current,
                                      const std::function<juce::String (const ProgramId&)>& labelFor)
{
    rows.clear();
    scrollY = 0;
    hoveredRow = -1;

    bool factoryCaptionDone = false;
    bool userCaptionDone = false;

    for (const auto& id : programs)
    {
        // **Order is INIT, separator, FACTORY caption, items, USER caption, items** - section 9.
        // The separator sits under INIT alone, which is what puts INIT outside both banks visually
        // as well as in the model.
        if (id.bank == ProgramBank::factory && ! std::exchange (factoryCaptionDone, true))
        {
            rows.push_back ({ Row::Kind::separator, {}, {}, false });
            rows.push_back ({ Row::Kind::caption, {}, "FACTORY", false });
        }

        if (id.bank == ProgramBank::user && ! std::exchange (userCaptionDone, true))
            rows.push_back ({ Row::Kind::caption, {}, "USER", false });

        rows.push_back ({ Row::Kind::item, id, labelFor (id), id == current });
    }

    // **The USER group is never hidden.** An empty bank shows a non-selectable placeholder rather
    // than vanishing, so the list's shape does not change with the bank's contents.
    if (! userCaptionDone)
    {
        rows.push_back ({ Row::Kind::caption, {}, "USER", false });
        rows.push_back ({ Row::Kind::placeholder, {},
                          Text::emDash() + juce::String (" none saved ") + Text::emDash(), false });
    }

    repaint();
}

//==============================================================================
int ReflectProgramList::contentHeight() const noexcept
{
    int h = 0;

    for (const auto& r : rows)
        h += r.height();

    return h;
}

bool ReflectProgramList::scrollable() const noexcept
{
    // Measured against the FULL height, not the banded viewport: the bands only appear because the
    // content did not fit, so asking whether it fits inside them would be circular.
    return contentHeight() > getHeight();
}

int ReflectProgramList::viewportHeight() const noexcept
{
    return scrollable() ? getHeight() - 2 * chevronBandHeight : getHeight();
}

int ReflectProgramList::maxScroll() const noexcept
{
    return juce::jmax (0, contentHeight() - viewportHeight());
}

void ReflectProgramList::setScroll (int y)
{
    const int clamped = juce::jlimit (0, maxScroll(), y);

    if (clamped != scrollY)
    {
        scrollY = clamped;
        repaint();
    }
}

void ReflectProgramList::resized()
{
    setScroll (scrollY);   // a resize can shorten the travel
}

juce::Rectangle<int> ReflectProgramList::topBandArea() const
{
    return { 0, 0, getWidth(), chevronBandHeight };
}

juce::Rectangle<int> ReflectProgramList::bottomBandArea() const
{
    return { 0, getHeight() - chevronBandHeight, getWidth(), chevronBandHeight };
}

int ReflectProgramList::rowAt (juce::Point<int> p) const
{
    const int top = scrollable() ? chevronBandHeight : 0;

    if (scrollable() && (topBandArea().contains (p) || bottomBandArea().contains (p)))
        return -1;   // the bands are furniture, not rows

    int y = top - scrollY;

    for (size_t i = 0; i < rows.size(); ++i)
    {
        const int h = rows[i].height();

        if (p.y >= y && p.y < y + h)
            return (int) i;

        y += h;
    }

    return -1;
}

//==============================================================================
void ReflectProgramList::paint (juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();

    // **Glass, not bezel** - the display continuing downward. Three stops, the mid at 45%.
    juce::ColourGradient surface (Colour::listTop, full.getCentreX(), full.getY(),
                                  Colour::listBottom, full.getCentreX(), full.getBottom(), false);
    surface.addColour (0.45, Colour::listMid);
    g.setGradientFill (surface);
    g.fillRect (full);

    // The LCD's own phosphor treatment, continued: 1px scanlines on a 3px pitch, plus a soft bloom
    // from the top edge where the list meets the glass.
    g.setColour (juce::Colour::fromRGBA (198, 222, 255, 7));   // .028

    for (int y = 0; y < getHeight(); y += 3)
        g.fillRect (0, y, getWidth(), 1);

    juce::ColourGradient bloom (juce::Colour::fromRGBA (150, 190, 240, 13), full.getCentreX(), full.getY(),
                                juce::Colours::transparentBlack, full.getCentreX(), full.getY() + full.getHeight() * 0.6f,
                                false);
    g.setGradientFill (bloom);
    g.fillRect (full);

    // **No top border and no top radius** - the join to the LCD is invisible by design. Sides only.
    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 179));   // .70
    g.fillRect (0.0f, 0.0f, 1.0f, full.getHeight());
    g.fillRect (full.getRight() - 1.0f, 0.0f, 1.0f, full.getHeight());

    // The recess continuing from the LCD: a shade down each side and across the top.
    for (int i = 0; i < 7; ++i)
    {
        const float a = 0.55f * (1.0f - (float) i / 7.0f);
        g.setColour (juce::Colours::black.withAlpha (a * 0.35f));
        g.fillRect ((float) i, 0.0f, 1.0f, full.getHeight());
        g.fillRect (full.getRight() - 1.0f - (float) i, 0.0f, 1.0f, full.getHeight());
    }

    for (int i = 0; i < 6; ++i)
    {
        const float a = 0.70f * (1.0f - (float) i / 6.0f);
        g.setColour (juce::Colours::black.withAlpha (a * 0.4f));
        g.fillRect (0.0f, (float) i, full.getWidth(), 1.0f);
    }

    // --- rows ---------------------------------------------------------------
    const bool scrolls = scrollable();
    const int top = scrolls ? chevronBandHeight : 0;

    {
        // Clipped to the viewport so a row cannot draw into a band. With the bands present the list
        // is INSET by them rather than fading under them - section 9 is explicit that they are
        // fixed furniture at the ends, not an overlay on moving content.
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion ({ 0, top, getWidth(), viewportHeight() });

        int y = top - scrollY;

        for (size_t i = 0; i < rows.size(); ++i)
        {
            const int h = rows[i].height();
            const juce::Rectangle<int> area { 0, y, getWidth(), h };

            if (area.getBottom() > top && area.getY() < top + viewportHeight())
                paintRow (g, rows[i], area, (int) i == hoveredRow);

            y += h;
        }
    }

    if (scrolls)
        paintBands (g);
}

void ReflectProgramList::paintRow (juce::Graphics& g, const Row& row,
                                   juce::Rectangle<int> area, bool hovered) const
{
    const auto r = area.toFloat();

    if (row.kind == Row::Kind::separator)
    {
        // A 1px rule 4px down the 9px band, so the gap reads evenly above and below.
        g.setColour (Colour::listSeparator);
        g.fillRect (r.getX() + (float) textInsetLeft, r.getY() + 4.0f,
                    r.getWidth() - (float) (textInsetLeft + textInsetRight), 1.0f);
        return;
    }

    if (row.kind == Row::Kind::caption)
    {
        Text::drawTracked (g, row.text, Font::mono (10.0f), 2.6f,
                           r.withTrimmedLeft ((float) textInsetLeft)
                            .withTrimmedRight ((float) textInsetRight),
                           juce::Justification::centredLeft, Colour::listCaption);
        return;
    }

    if (row.kind == Row::Kind::placeholder)
    {
        Text::drawTracked (g, row.text, Font::mono (12.0f), 0.96f,
                           r.withTrimmedLeft ((float) textInsetLeft)
                            .withTrimmedRight ((float) textInsetRight),
                           juce::Justification::centredLeft, Colour::listPlaceholder);
        return;
    }

    // --- an item ------------------------------------------------------------
    // **The current row does not respond to hover** - it is already lifted, and a second lift on
    // top would read as a different state again.
    if (row.isCurrent)
    {
        g.setColour (Colour::listCurrentField);
        g.fillRect (r);

        // The 3px lit bar, flush at x = 0, full row height. Its glow is drawn as two widening
        // washes rather than a blur, which is enough at this size and costs nothing.
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (Colour::listMarker.withAlpha (0.65f * 0.18f * (float) i / 3.0f));
            g.fillRect (r.withWidth ((float) markerWidth + (float) i * 2.0f));
        }

        g.setColour (Colour::listMarker);
        g.fillRect (r.withWidth ((float) markerWidth));
    }
    else if (hovered)
    {
        g.setColour (Colour::listHoverField);
        g.fillRect (r);
    }

    Text::drawTracked (g, row.text, Font::mono (13.0f), 1.3f,
                       r.withTrimmedLeft ((float) textInsetLeft)
                        .withTrimmedRight ((float) textInsetRight),
                       juce::Justification::centredLeft,
                       row.isCurrent ? Colour::listCurrentText : Colour::listItem);
}

void ReflectProgramList::paintBands (juce::Graphics& g) const
{
    const auto top = topBandArea();
    const auto bottom = bottomBandArea();

    // Opaque, and each takes the surface's own end - so a band reads as the glass ending rather
    // than as a panel laid over it.
    g.setColour (Colour::listBandTop);
    g.fillRect (top);
    g.setColour (Colour::listBandBottom);
    g.fillRect (bottom);

    g.setColour (Colour::listBandRule);
    g.fillRect (top.getX(), top.getBottom() - 1, top.getWidth(), 1);
    g.fillRect (bottom.getX(), bottom.getY(), bottom.getWidth(), 1);

    paintChevron (g, top,    true,  scrollY > 0);
    paintChevron (g, bottom, false, scrollY < maxScroll());
}

void ReflectProgramList::paintChevron (juce::Graphics& g, juce::Rectangle<int> band,
                                       bool pointingUp, bool enabled) const
{
    // **The LCD caret's own construction**, which is where the list's ink comes from: a 9 x 9 box
    // with two 1.6px borders, rotated 45 degrees. Left+top for up, right+bottom for down - the up
    // chevron is literally the caret rotated.
    constexpr float box = 9.0f, stroke = 1.6f;

    juce::Path p;

    if (pointingUp)
    {
        p.startNewSubPath (0.0f, box);
        p.lineTo (0.0f, 0.0f);
        p.lineTo (box, 0.0f);
    }
    else
    {
        p.startNewSubPath (box, 0.0f);
        p.lineTo (box, box);
        p.lineTo (0.0f, box);
    }

    // Offset 4px toward the band's OUTER edge so the glyph sits optically centred rather than
    // geometrically - the rotated box's ink is not centred in its own bounds.
    const float cx = (float) band.getCentreX();
    const float cy = (float) band.getCentreY() + (pointingUp ? -4.0f : 4.0f);

    p.applyTransform (juce::AffineTransform::rotation (pi * 0.25f, box * 0.5f, box * 0.5f)
                          .translated (cx - box * 0.5f, cy - box * 0.5f));

    if (enabled)
    {
        juce::DropShadow (Colour::listChevron.withAlpha (0.55f), 4, {}).drawForPath (g, p);
        g.setColour (Colour::listChevron);
    }
    else
    {
        // **Not removed and not hidden** - it steps back to the Program buttons' unlit ink with no
        // glow. Nothing on this panel is ever drawn inert.
        g.setColour (Colour::listChevronDim);
    }

    g.strokePath (p, juce::PathStrokeType (stroke));
}

//==============================================================================
void ReflectProgramList::mouseDown (const juce::MouseEvent& e)
{
    if (scrollable())
    {
        // One click is four item rows, so a row is never left half under a band.
        if (topBandArea().contains (e.getPosition()))    { setScroll (scrollY - scrollStep); return; }
        if (bottomBandArea().contains (e.getPosition())) { setScroll (scrollY + scrollStep); return; }
    }

    const int row = rowAt (e.getPosition());

    if (row >= 0 && rows[(size_t) row].selectable())
    {
        if (onProgramChosen)
            onProgramChosen (rows[(size_t) row].id);

        return;
    }

    // A click on a caption, a separator or empty glass closes rather than doing nothing: the list
    // is modal in feel, and a dead click inside it reads as the panel having stopped responding.
    if (onDismissRequested)
        onDismissRequested();
}

void ReflectProgramList::mouseMove (const juce::MouseEvent& e)
{
    const int row = rowAt (e.getPosition());
    const int wanted = (row >= 0 && rows[(size_t) row].selectable() && ! rows[(size_t) row].isCurrent)
                           ? row : -1;

    if (wanted != hoveredRow)
    {
        hoveredRow = wanted;
        repaint();
    }
}

void ReflectProgramList::mouseExit (const juce::MouseEvent&)
{
    if (hoveredRow != -1)
    {
        hoveredRow = -1;
        repaint();
    }
}

void ReflectProgramList::mouseWheelMove (const juce::MouseEvent&,
                                         const juce::MouseWheelDetails& wheel)
{
    if (! scrollable())
        return;

    // The platform delta, not the chevron's step: a wheel is a continuous gesture and snapping it
    // to four rows would feel geared.
    setScroll (scrollY - juce::roundToInt (wheel.deltaY * (float) itemHeight * 3.0f));
}
