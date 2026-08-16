#include "ProgramHeader.h"

#include "../PluginProcessor.h"

using namespace ReflectTheme;

namespace
{
    constexpr int pollHz = 20;

    /** 1 second period, 50% duty - the caret blinks on wall-clock time rather than a frame count
        so it stays steady whatever the poll rate is. */
    bool caretIsOn()
    {
        return (juce::Time::getMillisecondCounter() % 1000) < 500;
    }
}

//==============================================================================
juce::Rectangle<int> ProgramHeader::canvasBounds()
{
    const float right = Layout::deleteButtonX + Layout::deleteButtonW;

    return juce::Rectangle<float> (Layout::programWellX,
                                   Layout::programWellY,
                                   right - Layout::programWellX,
                                   Layout::programWellH).getSmallestIntegerContainer();
}

ProgramHeader::ProgramHeader (Reflect84AudioProcessor& processor)
    : processorRef (processor)
{
    setBounds (canvasBounds());
    setWantsKeyboardFocus (true);
    refreshFromProcessor();
    startTimerHz (pollHz);
}

ProgramHeader::~ProgramHeader()
{
    stopTimer();
}

//==============================================================================
juce::Rectangle<float> ProgramHeader::displayArea() const
{
    return { 0.0f, 0.0f, Layout::programWellW, Layout::programWellH };
}

juce::Rectangle<float> ProgramHeader::saveArea() const
{
    return { Layout::saveButtonX - Layout::programWellX, 0.0f,
             Layout::saveButtonW, Layout::headerButtonH };
}

juce::Rectangle<float> ProgramHeader::deleteArea() const
{
    return { Layout::deleteButtonX - Layout::programWellX, 0.0f,
             Layout::deleteButtonW, Layout::headerButtonH };
}

ProgramHeader::Region ProgramHeader::regionAt (juce::Point<float> position) const
{
    if (displayArea().contains (position)) return Region::display;
    if (saveArea().contains (position))    return Region::save;
    if (deleteArea().contains (position))  return Region::deleteOrCancel;

    return Region::none;
}

bool ProgramHeader::isRegionEnabled (Region region) const
{
    if (namingMode)
    {
        // STORE and CANCEL are both live; the display is a text field, not a menu trigger.
        return region == Region::save || region == Region::deleteOrCancel;
    }

    switch (region)
    {
        case Region::display:        return true;

        // **SAVE is gated on modification, and GUI-SPEC.md section 9 disagrees - the code is
        // right.** The spec says "SAVE is never disabled" while also saying the flow follows
        // TapeRot, which gates it; the two cannot both hold. Gating was a decision taken on this
        // side that never made it back to the designer, so the spec is describing an earlier
        // state rather than a newer intent. Raised with them; do not "fix" this to match §9.
        case Region::save:           return displayedIsModified;   // nothing moved, nothing to save

        // Only a User Program can be deleted. INIT and an unresolved id are not stored things.
        case Region::deleteOrCancel: return displayedId.bank == ProgramBank::user;
        case Region::none:           break;
    }

    return false;
}

//==============================================================================
bool ProgramHeader::refreshFromProcessor()
{
    // Never write the displayed mirrors while naming: leaving the mode then reverts the display
    // for free, with nothing to undo.
    if (namingMode)
        return false;

    auto& manager = processorRef.getProgramManager();

    const auto id = manager.getCurrentProgramId();
    const bool modified = manager.isModifiedFromLoadedProgram();

    if (id == displayedId && modified == displayedIsModified)
        return false;

    displayedId = id;
    displayedIsModified = modified;
    return true;
}

void ProgramHeader::showParameter (const juce::RangedAudioParameter& param)
{
    if (namingMode)
        return;     // the glass belongs to the name field until it commits or cancels

    // **Straight through nf::describeParameter**, which is straight through the parameter's own
    // name and JUCE's own text conversion - so the LCD and the host's automation lane cannot
    // disagree. Section 9 wants the printed control names in full (PRE-DELAY, STEREO WIDTH,
    // DAMPING HF), which is what the parameter names already are.
    //
    // **Nothing is re-cased**, and this casting is why the flag that used to do it was named
    // ValueCase::all rather than something that sounds harmless. Reflect-84's parameters bake their
    // unit into the text (ParamFormat's dampHFText, decayText, trimText), so upper-casing produced
    // "4.8 KHZ", "4.6 S" and "+2.5 DB". A capital S is a different unit from a lowercase one, and
    // KHZ is not a unit at all.
    //
    // The flag is gone from core as of 2026-08-13 and the NAME is no longer upper-cased either:
    // case belongs at the source, so a panel label is authored in caps in Parameters.h. That
    // re-authoring is still outstanding here — see the root ../CLAUDE.md.
    const auto text = nf::describeParameter (param, ReflectTheme::Layout::readoutFormat());
    const auto now = juce::Time::getMillisecondCounter();

    // Fires on every value change through a drag, not only on grab. Wired to onDragStart alone,
    // this LCD showed the value the knob held at the instant it was grabbed and never updated
    // while turning - live but frozen, which reads as a stuck display rather than a missing call.
    if (text != readout.textAt (now))
        repaint();

    readout.show (text);
    readoutWasShowing = true;
}

void ProgramHeader::releaseParameter()
{
    readout.release (juce::Time::getMillisecondCounter());
}

void ProgramHeader::timerCallback()
{
    bool needsRepaint = refreshFromProcessor();

    if (const bool showing = readout.isShowing (juce::Time::getMillisecondCounter());
        showing != readoutWasShowing)
    {
        readoutWasShowing = showing;
        needsRepaint = true;
    }

    if (namingMode)
    {
        if (const bool on = caretIsOn(); on != caretVisible)
        {
            caretVisible = on;
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

//==============================================================================
void ProgramHeader::mouseDown (const juce::MouseEvent& e)
{
    const auto region = regionAt (e.position);
    pressedRegion = isRegionEnabled (region) ? region : Region::none;
    repaint();
}

void ProgramHeader::mouseUp (const juce::MouseEvent& e)
{
    const auto released = regionAt (e.position);
    const auto pressed = pressedRegion;

    pressedRegion = Region::none;
    repaint();

    // Dragging off a button cancels it, the way a real button works.
    if (released != pressed || pressed == Region::none)
        return;

    if (! isRegionEnabled (pressed))
        return;

    if (namingMode)
    {
        if (pressed == Region::save)
            commitNaming();
        else if (pressed == Region::deleteOrCancel)
            cancelNaming();

        return;
    }

    switch (pressed)
    {
        case Region::display:
            showProgramMenu();
            break;

        case Region::save:
            enterNamingMode();
            break;

        case Region::deleteOrCancel:
            if (displayedId.bank == ProgramBank::user)
                processorRef.getProgramManager().deleteUserProgram (displayedId);
            break;

        case Region::none:
            break;
    }
}

void ProgramHeader::mouseMove (const juce::MouseEvent& e)
{
    const auto region = regionAt (e.position);

    setMouseCursor (isRegionEnabled (region) ? juce::MouseCursor::PointingHandCursor
                                             : juce::MouseCursor::NormalCursor);

    if (region != hoveredRegion)
    {
        hoveredRegion = region;
        repaint();
    }
}

void ProgramHeader::mouseExit (const juce::MouseEvent&)
{
    if (hoveredRegion != Region::none)
    {
        hoveredRegion = Region::none;
        repaint();
    }
}

//==============================================================================
void ProgramHeader::showProgramMenu()
{
    if (programList == nullptr)
        return;

    auto& manager = processorRef.getProgramManager();

    // **The list is a Component now**, so opening it is setting its rows and showing it - no
    // anchor rectangle, no parent area, no 1px strip. All of that existed to make PopupMenu land
    // where this panel wanted it; see ReflectProgramList.h.
    programList->setPrograms (manager.listPrograms(), manager.getCurrentProgramId(),
                              [&manager] (const ProgramId& id) { return manager.displayLabelFor (id); });

    const juce::Component::SafePointer<ProgramHeader> safeThis { this };

    programList->onProgramChosen = [safeThis] (const ProgramId& id)
    {
        if (safeThis == nullptr)
            return;

        safeThis->closeProgramMenu();
        safeThis->processorRef.getProgramManager().requestProgramChange (id);
    };

    programList->onDismissRequested = [safeThis]
    {
        if (safeThis != nullptr)
            safeThis->closeProgramMenu();
    };

    programList->setVisible (true);
    programList->toFront (false);

    menuOpen = true;
    repaint();
}

void ProgramHeader::closeProgramMenu()
{
    if (programList != nullptr)
        programList->setVisible (false);

    // Cleared here rather than on selection, so dismissing cannot leave the chevron inverted -
    // the same reason the PopupMenu version cleared it in its callback rather than its selection
    // branch.
    menuOpen = false;
    repaint();
}

//==============================================================================
void ProgramHeader::enterNamingMode()
{
    // Cancel the takeover rather than letting paint order hide it: hidden, it returns the moment
    // naming ends if the revert has not yet fired.
    readout.suppress();
    readoutWasShowing = false;

    namingMode = true;
    typedName.clear();
    caretVisible = true;
    grabKeyboardFocus();
    repaint();
}

void ProgramHeader::commitNaming()
{
    const auto name = typedName;

    namingMode = false;
    typedName.clear();
    giveAwayKeyboardFocus();

    processorRef.getProgramManager().saveNewUserProgram (name);

    refreshFromProcessor();
    repaint();
}

void ProgramHeader::cancelNaming()
{
    // Must NOT touch the APVTS: the user's tweaked-but-unsaved values survive a cancel. The
    // displayed mirrors were never written while naming, so simply leaving the mode restores
    // whatever was on screen before SAVE was pressed.
    namingMode = false;
    typedName.clear();
    giveAwayKeyboardFocus();
    repaint();
}

bool ProgramHeader::keyPressed (const juce::KeyPress& key)
{
    if (! namingMode)
        return false;

    if (key == juce::KeyPress::returnKey)  { commitNaming(); return true; }
    if (key == juce::KeyPress::escapeKey)  { cancelNaming(); return true; }

    if (key == juce::KeyPress::backspaceKey)
    {
        typedName = typedName.dropLastCharacters (1);
        repaint();
        return true;
    }

    const auto character = key.getTextCharacter();

    if (character >= 32 && character != 127 && typedName.length() < ProgramManager::maxProgramNameLength)
    {
        typedName += juce::String::charToString (character).toUpperCase();
        repaint();
        return true;
    }

    return true;    // swallow everything else while naming, so stray keys can't escape the field
}

void ProgramHeader::focusLost (FocusChangeType)
{
    if (namingMode)
        cancelNaming();
}

//==============================================================================
void ProgramHeader::paintLegend (juce::Graphics& g, const juce::String& text,
                                 juce::Rectangle<float> lineBox, bool lit)
{
    const auto font = Font::mono (Layout::legendTextSize);
    const float tracking = Font::trackingPx (Layout::legendTracking, Layout::legendTextSize);

    // The spec's `text-indent: .20em` cancels the trailing tracking CSS leaves on the last
    // character, so the legend centres optically rather than metrically. drawTracked adds tracking
    // BETWEEN glyphs only - the trailing gap never exists here - so there is nothing to cancel.

    if (lit)
    {
        /*  **The legend is the lamp**, so the bloom has to read as light coming from behind the
            type rather than as a soft edge on it.

            GUI-SPEC gives five text-shadow layers (1 / 4 / 9 / 18 / 30 px). JUCE has neither
            text-shadow nor a cheap blur for a string, so each layer here is the same tracked text
            drawn at eight points around a circle: overlapping copies sum to a halo, which is what
            a blur of that radius produces anyway at 10px type.

            **Three radii, not five, and that is deliberate.** The 18px and 30px layers are the
            faint wash a backlit legend throws onto the face *around* it - but this face is only
            34px tall, so at those radii the wash covers the whole button and spills onto the bezel,
            where it reads as a smudge rather than as light. The clip below would cut them square
            in any case. Folded into the 9px pass instead.

            Alphas are tuned by eye against the render, not derived: eight overlapping copies at
            alpha a reach 1-(1-a)^8 where they coincide, so the published per-layer figures cannot
            be used directly. */
        struct BloomLayer { float radius, alpha; };
        static constexpr BloomLayer bloom[] { { 9.0f, 0.030f }, { 4.0f, 0.055f }, { 1.0f, 0.100f } };

        for (const auto& layer : bloom)
            for (int i = 0; i < 8; ++i)
            {
                const float angle = juce::MathConstants<float>::twoPi * (float) i / 8.0f;

                Text::drawTracked (g, text, font, tracking,
                                   lineBox.translated (std::cos (angle) * layer.radius,
                                                       std::sin (angle) * layer.radius),
                                   juce::Justification::centred,
                                   Colour::legendLit.withAlpha (layer.alpha));
            }
    }

    Text::drawTracked (g, text, font, tracking, lineBox, juce::Justification::centred,
                       lit ? Colour::legendLit : Colour::legendUnlit);
}

void ProgramHeader::paintButton (juce::Graphics& g, juce::Rectangle<float> area,
                                 const juce::String& topLegend, const juce::String& bottomLegend,
                                 bool topLit, bool bottomLit)
{
    // **The face is identical in every state**, including the state that used to be "disabled".
    // Nothing here branches on enablement, and nothing should: a dark legend is not a disabled
    // control, it is a function with nothing to do.
    g.setGradientFill (Paint::verticalGradient (area, Colour::buttonFaceTop, Colour::buttonFaceBottom));
    g.fillRoundedRectangle (area, Layout::lcdRadius);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (area, Layout::lcdRadius, 1.0f);

    // inset 0 1px 0 rgba(255,255,255,.10) - a far shallower lip than the brass carried, because a
    // dark cap catches much less light along its top edge.
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawLine (area.getX() + 3.0f, area.getY() + 1.5f, area.getRight() - 3.0f, area.getY() + 1.5f, 1.0f);

    // Two 12px line boxes, 1px between, the pair centred on the face. Resting function on top,
    // what the button becomes while naming beneath it.
    const float lineH = Layout::legendLineHeight;
    const float blockH = lineH * 2.0f + Layout::legendGap;
    const float blockTop = area.getCentreY() - blockH * 0.5f;

    const juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area.getSmallestIntegerContainer());

    paintLegend (g, topLegend,    { area.getX(), blockTop, area.getWidth(), lineH }, topLit);
    paintLegend (g, bottomLegend, { area.getX(), blockTop + lineH + Layout::legendGap,
                                    area.getWidth(), lineH }, bottomLit);
}

void ProgramHeader::paint (juce::Graphics& g)
{
    const auto display = displayArea();
    const bool displayHovered = hoveredRegion == Region::display && ! namingMode;

    Paint::drawLcdWell (g, display, displayHovered);

    // --- FACT / USER, printed ON the glass -----------------------------------
    // Derived, never stored. It flips to USER the moment naming starts, because what is about to
    // be written is a User Program regardless of what was loaded.
    //
    // **Not a badge.** Section 9 makes it printed text in the SAME face and size as the program
    // name - no border, no fill, no radius - separated from the name by a 1px rule. A bordered chip
    // reads as a control you could press; this reads as what it is, a legend on the display. There
    // is no separate bank control anywhere on the panel, so nothing here should look like one.
    float nameCellLeft = 0.0f;
    {
        // **An em-dash where the Program is in neither bank** - INIT, or an unresolved identifier.
        const bool onInit = ! namingMode && (displayedId.bank == ProgramBank::init
                                              || displayedId.bank == ProgramBank::unresolved);

        const auto font = Font::mono (Layout::lcdTextSize);
        const float tracking = Font::trackingPx (Layout::lcdTextTracking, Layout::lcdTextSize);

        /*  **NAME while typing, not USER.** The Program is not in the user bank until the name is
            committed, so USER there claims a thing that does not exist yet - and if the user
            cancels, it never will. Elmer had this right first and it is the suite standard now.

            Cancelling is what makes the distinction visible: the tag has to go back to whatever
            bank the Program actually came from, which it does for free because refreshFromProcessor
            never writes the displayed mirrors while naming. */
        const juce::String bank = namingMode ? juce::String ("NAME")
                                : onInit     ? Text::emDash()
                                : juce::String (displayedId.bank == ProgramBank::user ? "USER" : "FACT");

        const float textW = Text::trackedWidth (bank, font, tracking);
        const float cellW = Layout::lcdBankPadX * 2.0f + textW;

        Text::drawTracked (g, bank, font, tracking,
                           { Layout::lcdBankPadX, 0.0f, textW, display.getHeight() },
                           juce::Justification::centredLeft, Colour::phosphor);

        // The 1px rule, inset 7px top and bottom - furniture on the glass, not a border round a box.
        g.setColour (Colour::phosphor.withAlpha (0.35f));
        g.fillRect (cellW, Layout::lcdRuleInsetY, 1.0f,
                    display.getHeight() - Layout::lcdRuleInsetY * 2.0f);

        nameCellLeft = cellW + 1.0f;
    }

    // --- Name, or the text field while naming --------------------------------
    {
        // From the theme, not repeated literals - the two disagreed for a while, and the budget
        // was derived from the declaration rather than from what was drawn.
        const auto font = Font::mono (Layout::lcdNameTextSize);
        const float tracking = Font::trackingPx (Layout::lcdNameTextTracking, Layout::lcdNameTextSize);

        // The phosphor glow: the same text drawn soft and wide underneath the crisp pass.
        const auto drawPhosphor = [&] (const juce::String& text,
                                       juce::Rectangle<float> area,
                                       juce::Justification justification)
        {
            Text::drawTracked (g, text, font, tracking, area, justification,
                               Colour::phosphor.withAlpha (0.35f));
            Text::drawTracked (g, text, font, tracking, area, justification, Colour::phosphor);
        };

        if (namingMode)
        {
            // Left-aligned while typing, so the caret does not jump about as the name grows.
            const auto field = display.withTrimmedLeft (nameCellLeft + 16.0f)
                                      .withTrimmedRight (nf::LcdCell::chevronTrim);

            const auto caret = juce::String::charToString ((juce::juce_wchar) 0x2588);   // U+2588 FULL BLOCK
            drawPhosphor (typedName + (caretVisible ? caret : juce::String()),
                          field, juce::Justification::left);
        }
        else
        {
            // 1-based two-digit numbering, computed here and never stored - the code's own
            // indices stay 0-based. CHORUS-60's convention, and the design mock already uses it.
            // The live readout takes the glass while a control is moving, then the Program name
            // comes back 900ms after release. Naming mode already returned above, so the three
            // states never contend for the cell.
            // An identifier the session named but the bank no longer has: the VALUES are correct
            // and untouched, only the name is unknown, so the panel says so rather than pretending.
            // Otherwise the number is a label computed from the Factory position at paint time;
            // INIT and User Programs carry none.
            // **A trailing " *" while the loaded Program has been edited**, matching the other
            // five castings. REFLECT-84 signalled dirty only through SAVE's enabled state, which
            // has to be looked for; the marker is seen at a glance.
            //
            // No marker on an unresolved identifier: there is no baseline to differ from, so an
            // asterisk there would be claiming something it cannot know.
            const auto programLabel =
                displayedId.bank == ProgramBank::unresolved
                    ? displayedId.displayName + "?"
                    : processorRef.getProgramManager().displayLabelFor (displayedId)
                        + (displayedIsModified ? " *" : "");

            const auto takeover = readout.textAt (juce::Time::getMillisecondCounter());
            const auto label = takeover.isNotEmpty() ? takeover : programLabel;

            drawPhosphor (label,
                          display.withTrimmedLeft (nameCellLeft)
                                 .withTrimmedRight (nf::LcdCell::chevronTrim),
                          juce::Justification::centred);
        }
    }

    // --- Chevron -------------------------------------------------------------
    if (! namingMode)
    {
        /*  **`nf::Chevron`, the shared 14 x 8 path — this was one of the nine sites.**

            HEADER-PART §10 leads its propagation argument with this glyph: it reached one casting
            and missed nine sites, which drew a 9 x 9 rotated box at 84.5 degrees against the drawn
            path's 77. Two of those nine are this panel's scroll chevrons and the third is here.

            **The box inset is 16 px and the trim is 30**, per §5. Those are one figure seen twice:
            a 14-wide glyph whose box ends 16 px from the cell's inner right edge starts 30 px from
            it, which is exactly the trim the name area is measured against. Deriving the box from
            the inset rather than restating 30 is what keeps the two from drifting — the budget is
            538.00 only while they agree.

            It still inverts while the list is open, and core's `up` is the path **mirrored, not
            rotated**: a rotated V puts its round caps on the wrong axis. */
        const float boxRight = display.getRight() - 16.0f;
        const juce::Rectangle<float> chevronBox { boxRight - nf::Chevron::width,
                                                  display.getCentreY() - nf::Chevron::height * 0.5f,
                                                  nf::Chevron::width, nf::Chevron::height };

        const auto chevron = menuOpen ? nf::Chevron::up (chevronBox)
                                      : nf::Chevron::down (chevronBox);

        g.setColour (Colour::bezelGoldBright);
        g.strokePath (chevron, { 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::square });
    }

    // --- Buttons -------------------------------------------------------------
    /*  GUI-SPEC section 9's state matrix, in full. Five panel states, four legends, and the four
        expressions below are the whole contract:

        | Panel state                 | SAVE | STORE | DELETE | CANCEL |
        | Factory Program, unmodified | dark | dark  | dark   | dark   |
        | Factory Program, edited     | LIT  | dark  | dark   | dark   |
        | User Program, unmodified    | dark | dark  | LIT    | dark   |
        | User Program, edited        | LIT  | dark  | LIT    | dark   |
        | Naming a Program            | dark | LIT   | dark   | LIT    |

        **Lighting and clickability come from one source each**, not from two that have to be kept
        in step: each `lit` below is exactly `isRegionEnabled` for that region, so a legend cannot
        glow at something that will ignore the click, or sit dark on something that responds.

        The naming row is why STORE and CANCEL are gated on `namingMode` rather than on the region
        being enabled: while naming, `isRegionEnabled` reports both regions live, and it is the row
        of the matrix that decides which of each button's two legends that liveness belongs to. */
    /*  **The matrix itself is `nf::programButtonLegends` now.** It is a decision table, and a
        decision table held six times is the shape that had one casting printing the meter's plus
        sign at `db >= 0.0f` and another at `db > 0.0f` — one value, two castings, no reason.

        What stays here is the panel's own knowledge: which region is live, and that a lit legend
        must never sit on something that will ignore the click. The assertion below is what keeps
        those two in step now that they come from different places. */
    const nf::ProgramPanelState panelState { displayedId.bank == ProgramBank::user,
                                             displayedIsModified,
                                             namingMode };

    const auto legends = nf::programButtonLegends (panelState);

    // A lit legend on a dead region is the defect the old single-source construction ruled out by
    // construction; sourcing the matrix from core reopens the possibility, so it is asserted rather
    // than assumed. Debug-only: it is an invariant between two pure functions, not a runtime risk.
    jassert (! legends.save || isRegionEnabled (Region::save));
    jassert (! legends.deleteLegend || isRegionEnabled (Region::deleteOrCancel));

    paintButton (g, saveArea(), "SAVE", "STORE", legends.save, legends.store);
    paintButton (g, deleteArea(), "DELETE", "CANCEL", legends.deleteLegend, legends.cancel);
}
