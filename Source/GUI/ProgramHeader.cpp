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

    // Straight through the parameter's own name and JUCE's own text conversion, so the LCD and the
    // host's automation lane cannot disagree - the same construction TapeRot uses. Section 9 wants
    // the printed control names in full (PRE-DELAY, STEREO WIDTH, DAMPING HF), which is what the
    // parameter names already are.
    const auto name = param.getName (Layout::lcdCharacterBudget).toUpperCase();
    const auto value = param.getText (param.getValue(), 0).toUpperCase();

    liveReadout = name + ": " + value;
    readoutRevertAtMs = 0;
    repaint();
}

void ProgramHeader::releaseParameter()
{
    if (liveReadout.isNotEmpty())
        readoutRevertAtMs = juce::Time::getMillisecondCounter() + Layout::lcdReadoutHoldMs;
}

void ProgramHeader::timerCallback()
{
    bool needsRepaint = refreshFromProcessor();

    if (readoutRevertAtMs != 0 && juce::Time::getMillisecondCounter() >= readoutRevertAtMs)
    {
        liveReadout.clear();
        readoutRevertAtMs = 0;
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
    auto& manager = processorRef.getProgramManager();

    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());

    const auto current = manager.getCurrentProgramId();

    // **Row IDs are positions in THIS menu, not Program indices.** PopupMenu needs an int per row
    // and reserves 0 for "dismissed"; the callback maps the row back to the ProgramId it was built
    // from, so no Program is addressed by a bank position here.
    //
    // That also removes a latent collision: the empty-User placeholder used item ID -1, which was
    // numerically equal to the old initProgramIndex.
    // A row ID no real Program can take. Row IDs start at 1, so any non-positive value is safe;
    // this used to be -1, which was numerically equal to the old initProgramIndex.
    constexpr int menuPlaceholderId = -1000;

    menuRows = manager.listPrograms();

    bool factoryHeaderDone = false;
    bool userHeaderDone = false;

    for (size_t i = 0; i < menuRows.size(); ++i)
    {
        const auto& id = menuRows[i];

        if (id.bank == ProgramBank::factory && ! std::exchange (factoryHeaderDone, true))
        {
            menu.addSeparator();
            menu.addSectionHeader ("Factory");
        }

        if (id.bank == ProgramBank::user && ! std::exchange (userHeaderDone, true))
        {
            menu.addSeparator();
            menu.addSectionHeader ("User");
        }

        menu.addItem ((int) i + 1, manager.displayLabelFor (id), true, id == current);
    }

    // **Both groups are always present, and the USER header is never hidden** - section 9. An empty
    // bank shows a disabled placeholder rather than vanishing.
    if (! userHeaderDone)
    {
        menu.addSeparator();
        menu.addSectionHeader ("User");
        menu.addItem (menuPlaceholderId, Text::emDash() + juce::String (" none saved ") + Text::emDash(),
                      false, false);
    }

    const auto well = displayArea().getSmallestIntegerContainer();

    auto options = juce::PopupMenu::Options()
                       .withTargetComponent (this)
                       .withTargetScreenArea (localAreaToGlobal (well))
                       .withMaximumNumColumns (1);

    if (menuParent != nullptr)
    {
        // The list is laid out INSIDE menuHost rather than as its own desktop window. JUCE fits a
        // menu to its parent area, so an area running from the well's bottom edge to the panel's
        // gives both guarantees at once: the top cannot move and the height cannot exceed the
        // panel. A bank too long to fit scrolls. See ../../CLAUDE.md, "The Program dropdown".
        //
        // Anchor to a 1px strip on the well's bottom EDGE, not the well. With a parent, JUCE first
        // does constrainedWithin(parentArea), which slides the whole 42px well down into the host
        // before measuring and opens the list 42px too low. 1px and not zero: a zero-height
        // rectangle is isEmpty(), which drops the list out of align-to-rectangle into the sideways
        // placement meant for submenus.
        //
        // Built in LOCAL coordinates - this component's origin is the well's own top-left, so the
        // well's bottom edge is simply well.getBottom(). menuAnchorY() is the canvas-space twin of
        // the same row, for menuHost.
        const juce::Rectangle<int> anchor { well.getX(), well.getBottom() - 1, well.getWidth(), 1 };

        options = options.withTargetScreenArea (localAreaToGlobal (anchor))
                         .withParentComponent (menuParent)
                         .withMinimumWidth (well.getWidth());
    }

    const juce::Component::SafePointer<ProgramHeader> safeThis { this };

    menuOpen = true;
    repaint();

    menu.showMenuAsync (options,
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr)
                                return;

                            // Cleared here rather than on selection: JUCE runs this callback on a
                            // dismissal too, so clicking away cannot leave the mark inverted.
                            safeThis->menuOpen = false;
                            safeThis->repaint();

                            if (result == 0)
                                return;

                            // No forced refresh: the async apply plus the 20 Hz poll handle it.
                            const auto row = (size_t) (result - 1);

                            if (row < safeThis->menuRows.size())
                                safeThis->processorRef.getProgramManager()
                                    .requestProgramChange (safeThis->menuRows[row]);
                        });
}

//==============================================================================
void ProgramHeader::enterNamingMode()
{
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
void ProgramHeader::paintButton (juce::Graphics& g, juce::Rectangle<float> area,
                                 const juce::String& text, bool enabled, bool hovered)
{
    if (enabled)
    {
        g.setGradientFill (Paint::verticalGradient (area,
                                                    hovered ? Colour::brassTopHover : Colour::brassTop,
                                                    hovered ? Colour::brassBottomHover : Colour::brassBottom));
    }
    else
    {
        g.setGradientFill (Paint::verticalGradient (area, Colour::buttonOffTop, Colour::buttonOffBottom));
    }

    g.fillRoundedRectangle (area, Layout::lcdRadius);

    g.setColour (juce::Colours::black.withAlpha (enabled ? 0.5f : 0.45f));
    g.drawRoundedRectangle (area, Layout::lcdRadius, 1.0f);

    if (enabled)
    {
        // inset 0 1px 0 rgba(255,255,255,.55) - the machined top lip on the brass
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (area.getX() + 3.0f, area.getY() + 1.5f, area.getRight() - 3.0f, area.getY() + 1.5f, 1.0f);
    }

    Text::drawTracked (g, text, Font::mono (10.0f), Font::trackingPx (0.20f, 10.0f),
                       area, juce::Justification::centred,
                       enabled ? Colour::brassText : Colour::buttonOffText);
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
        const bool showUser = namingMode || displayedId.bank == ProgramBank::user;

        const auto font = Font::mono (Layout::lcdTextSize);
        const float tracking = Font::trackingPx (Layout::lcdTextTracking, Layout::lcdTextSize);
        const juce::String bank = onInit ? Text::emDash()
                                         : juce::String (showUser ? "USER" : "FACT");

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
                                      .withTrimmedRight (Layout::lcdChevronInsetRight + 18.0f);

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

            const auto label = liveReadout.isNotEmpty() ? liveReadout : programLabel;

            drawPhosphor (label,
                          display.withTrimmedLeft (nameCellLeft)
                                 .withTrimmedRight (Layout::lcdChevronInsetRight + 18.0f),
                          juce::Justification::centred);
        }
    }

    // --- Chevron -------------------------------------------------------------
    if (! namingMode)
    {
        const float s = Layout::chevronSize;
        const float x = display.getRight() - Layout::chevronInsetX - s;
        const float y = display.getCentreY() - s * 0.3f;

        // It inverts while the list is open, mirrored about the mark's own centre line rather than
        // rotated, so the apex stays on one vertical axis and it reads as flipping in place.
        // Without it the mark still points down at a list that is already down.
        const float outerY = menuOpen ? y + s * 0.55f : y;
        const float apexY = menuOpen ? y : y + s * 0.55f;

        juce::Path chevron;
        chevron.startNewSubPath (x, outerY);
        chevron.lineTo (x + s * 0.5f, apexY);
        chevron.lineTo (x + s, outerY);

        g.setColour (Colour::bezelGoldBright);
        g.strokePath (chevron, { 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::square });
    }

    // --- Buttons -------------------------------------------------------------
    paintButton (g, saveArea(), namingMode ? "STORE" : "SAVE",
                 isRegionEnabled (Region::save), hoveredRegion == Region::save);

    paintButton (g, deleteArea(), namingMode ? "CANCEL" : "DELETE",
                 isRegionEnabled (Region::deleteOrCancel), hoveredRegion == Region::deleteOrCancel);
}
