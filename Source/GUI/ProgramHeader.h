#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ReflectTheme.h"

class Reflect84AudioProcessor;

/**
    The PROGRAM cluster: the LCD well with its FACT/USER badge and chevron, plus the SAVE and
    DELETE buttons.

    Three states, exactly as Gatecrasher and CHORUS-60 have it:

      idle + factory   name shown, SAVE enabled only if something moved, DELETE disabled
      idle + user      as above, DELETE enabled
      naming           the LCD becomes a text field with a blinking block caret, SAVE reads
                       STORE and DELETE reads CANCEL

    There is no dialog anywhere in this - no AlertWindow, no FileChooser. Naming happens in the
    display itself, because a modal box in front of a 1980s rack fascia would break the illusion
    the whole panel exists to maintain.

    CANCEL never touches the APVTS. Whatever the user had tweaked before pressing SAVE is still
    there afterwards; the displayed Program name was never written to while naming, so leaving the
    mode is all the revert that is needed.
*/
class ProgramHeader final : public juce::Component,
                            private juce::Timer
{
public:
    explicit ProgramHeader (Reflect84AudioProcessor& processor);
    ~ProgramHeader() override;

    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    bool keyPressed (const juce::KeyPress& key) override;
    void focusLost (FocusChangeType cause) override;

    static juce::Rectangle<int> canvasBounds();

    /** Section 9's live parameter readout. While a control is being moved the LCD shows
        `<PARAMETER NAME>: <value>` in place of the program name, reverting 900ms after release.

        **The CALLER guards on the control's own drag state.** A SliderAttachment also fires when a
        Program is applied and on every host automation step, and without that guard the display
        latches onto whichever parameter was written last and flickers for the length of a song -
        which BRAND.md forbids outright: "Only direct user manipulation triggers it."

        Naming mode wins over both; the glass belongs to the name field until it commits or
        cancels. */
    void showParameter (const juce::RangedAudioParameter& param);
    void releaseParameter();

    /** The component the Program list is laid out inside. Its bounds become the list's parent area,
        which is what fixes the list's top edge and caps its height - layout, not plumbing. Passing
        nullptr returns the list to being a free desktop window sized to its own content, which for
        a long bank overhangs the panel. See ../../CLAUDE.md, "The Program dropdown". */
    void setMenuParent (juce::Component* parent) noexcept { menuParent = parent; }

    /** The row the list's top edge lands on, in CANVAS coordinates - the well's own bottom edge, so
        the two read as one object rather than a bar with a list floating under it.

        Canvas, not local: unlike the siblings this component covers only the header strip, so its
        own origin is the well's top-left. The anchor rectangle inside showProgramMenu is therefore
        built in local coordinates while menuHost, which lives in the editor, needs canvas ones. */
    static int menuAnchorY() noexcept
    {
        return (int) std::floor (ReflectTheme::Layout::programWellY
                                 + ReflectTheme::Layout::programWellH);
    }

    /** Where menuHost has to start, and it is NOT the anchor: JUCE clamps a menu to
        `jmax (parentArea.getY() + 1, ...)`, so a host beginning exactly at the anchor can only open
        one pixel below it, leaving a hairline of panel between the bar and its list.

        The lead has a floor and a ceiling. Too small and the clamp bites again; too large and the
        list can grow past the panel, because JUCE sizes it to `parentArea.getHeight() - 24` while
        the room actually below the anchor is the well's own height less than that. */
    static int menuHostTop() noexcept { return menuAnchorY() - 8; }

    /** The host's HEIGHT is what caps the list at section 9's 260px - JUCE has no max-height option,
        but it sizes a menu to `parentArea.getHeight() - 24` and clamps it there, so 260 + 24 plus
        the 8px anchor lead gives exactly that. Beyond it the list scrolls, which is what the
        twelve-Program bank plus two group headers does. */
    static int menuHostHeight() noexcept { return 8 + 260 + 24; }

private:
    juce::Component* menuParent = nullptr;
    bool menuOpen = false;

    enum class Region { none, display, save, deleteOrCancel };

    void timerCallback() override;

    bool refreshFromProcessor();
    Region regionAt (juce::Point<float> position) const;
    bool isRegionEnabled (Region region) const;

    void showProgramMenu();
    void enterNamingMode();
    void commitNaming();
    void cancelNaming();

    juce::Rectangle<float> displayArea() const;
    juce::Rectangle<float> saveArea() const;
    juce::Rectangle<float> deleteArea() const;

    void paintButton (juce::Graphics& g, juce::Rectangle<float> area,
                      const juce::String& text, bool enabled, bool hovered);

    Reflect84AudioProcessor& processorRef;

    // Mirrors of the processor's state, refreshed by the timer. Deliberately not written to while
    // naming - that is what makes cancelling free.
    int displayedIndex = -1;
    juce::String displayedName;
    bool displayedIsFactory = true;
    bool displayedIsModified = false;

    // The live readout and when it reverts. Held as text rather than a parameter pointer so the
    // display cannot outlive what it is showing.
    juce::String liveReadout;
    juce::uint32 readoutRevertAtMs = 0;

    bool namingMode = false;
    juce::String typedName;
    bool caretVisible = false;

    Region pressedRegion = Region::none;
    Region hoveredRegion = Region::none;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgramHeader)
};
