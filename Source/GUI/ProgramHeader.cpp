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
        case Region::save:           return displayedIsModified;   // nothing moved, nothing to save
        case Region::deleteOrCancel: return ! displayedIsFactory;  // factory Programs are read-only
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

    const int index = manager.getCurrentProgram();
    const auto name = manager.getProgramName (index);
    const bool factory = ProgramManager::isFactoryProgram (index);
    const bool modified = manager.isModifiedFromLoadedProgram();

    if (index == displayedIndex && name == displayedName
        && factory == displayedIsFactory && modified == displayedIsModified)
        return false;

    displayedIndex = index;
    displayedName = name;
    displayedIsFactory = factory;
    displayedIsModified = modified;
    return true;
}

void ProgramHeader::timerCallback()
{
    bool needsRepaint = refreshFromProcessor();

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
            if (! displayedIsFactory)
                processorRef.getProgramManager().deleteUserProgram (displayedIndex);
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

    const int current = manager.getCurrentProgram();
    const int total = manager.getNumPrograms();

    menu.addSectionHeader ("Factory");

    for (int i = 0; i < kNumFactoryPrograms; ++i)
        menu.addItem (i + 1,                                        // 0 means "dismissed"
                      juce::String (i + 1).paddedLeft ('0', 2) + " " + manager.getProgramName (i),
                      true,
                      i == current);

    if (total > kNumFactoryPrograms)
    {
        menu.addSeparator();
        menu.addSectionHeader ("User");

        for (int i = kNumFactoryPrograms; i < total; ++i)
            menu.addItem (i + 1,
                          juce::String (i + 1).paddedLeft ('0', 2) + " " + manager.getProgramName (i),
                          true,
                          i == current);
    }

    const juce::Component::SafePointer<ProgramHeader> safeThis { this };

    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (this)
                            .withTargetScreenArea (localAreaToGlobal (displayArea().getSmallestIntegerContainer())),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr || result == 0)
                                return;

                            // No forced refresh: the async apply plus the 20 Hz poll handle it.
                            safeThis->processorRef.setCurrentProgram (result - 1);
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

    // --- FACT / USER badge ---------------------------------------------------
    // Derived, never stored. It flips to USER the moment naming starts, because what is about to
    // be written is a User Program regardless of what was loaded.
    {
        const bool showUser = namingMode || ! displayedIsFactory;

        const juce::Rectangle<float> badge { Layout::badgeInsetX,
                                             (display.getHeight() - Layout::badgeH) * 0.5f,
                                             Layout::badgeW, Layout::badgeH };

        g.setColour (Colour::phosphor.withAlpha (0.4f));
        g.drawRoundedRectangle (badge, 2.0f, 1.0f);

        Text::drawTracked (g, showUser ? "USER" : "FACT",
                           Font::mono (11.0f), Font::trackingPx (0.16f, 11.0f),
                           badge, juce::Justification::centred, Colour::phosphor);
    }

    // --- Name, or the text field while naming --------------------------------
    {
        const auto font = Font::mono (17.0f);
        const float tracking = Font::trackingPx (0.16f, 17.0f);

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
            const auto field = display.withTrimmedLeft (Layout::badgeInsetX + Layout::badgeW + 16.0f)
                                      .withTrimmedRight (Layout::chevronInsetX + 12.0f);

            const auto caret = juce::String::charToString ((juce::juce_wchar) 0x2588);   // U+2588 FULL BLOCK
            drawPhosphor (typedName + (caretVisible ? caret : juce::String()),
                          field, juce::Justification::left);
        }
        else
        {
            // 1-based two-digit numbering, computed here and never stored - the code's own
            // indices stay 0-based. CHORUS-60's convention, and the design mock already uses it.
            const auto label = juce::String (displayedIndex + 1).paddedLeft ('0', 2) + " " + displayedName;

            drawPhosphor (label,
                          display.withTrimmedLeft (Layout::badgeInsetX + Layout::badgeW)
                                 .withTrimmedRight (30.0f),
                          juce::Justification::centred);
        }
    }

    // --- Chevron -------------------------------------------------------------
    if (! namingMode)
    {
        const float s = Layout::chevronSize;
        const float x = display.getRight() - Layout::chevronInsetX - s;
        const float y = display.getCentreY() - s * 0.3f;

        juce::Path chevron;
        chevron.startNewSubPath (x, y);
        chevron.lineTo (x + s * 0.5f, y + s * 0.55f);
        chevron.lineTo (x + s, y);

        g.setColour (Colour::bezelGoldBright);
        g.strokePath (chevron, { 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::square });
    }

    // --- Buttons -------------------------------------------------------------
    paintButton (g, saveArea(), namingMode ? "STORE" : "SAVE",
                 isRegionEnabled (Region::save), hoveredRegion == Region::save);

    paintButton (g, deleteArea(), namingMode ? "CANCEL" : "DELETE",
                 isRegionEnabled (Region::deleteOrCancel), hoveredRegion == Region::deleteOrCancel);
}
