#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <nf/HeaderPart.h>

#include "ReflectTheme.h"
#include "ReflectProgramList.h"
#include "../DSP/FactoryPrograms.h"      // ProgramId / ProgramBank

#include <vector>

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

    /** The list this header opens. Set by the editor, which owns it and positions it.

        **It is a Component, not a `juce::PopupMenu`, and the menuHost machinery is gone with it.**
        `menuAnchorY`, `menuHostTop` and `menuHostHeight` existed only to make JUCE land a popup in
        the right place: a 1px anchor strip because a zero-height one drops out of align-to-rectangle,
        an 8px lead because JUCE clamps to `jmax (parentArea.getY() + 1, ...)`, and a parent area
        sized to the panel because a free desktop window overhangs it. A Component sets its own
        bounds, so none of that is needed. See ReflectProgramList.h for why the change was forced. */
    void setProgramList (ReflectProgramList* list) noexcept { programList = list; }

    /** Hides the list and clears the chevron. */
    void closeProgramMenu();

    /** The row the list's top edge lands on, in CANVAS coordinates - the well's own bottom edge, so
        the two read as one object rather than a bar with a list floating under it. */
    static int listTopY() noexcept
    {
        return (int) std::floor (ReflectTheme::Layout::programWellY
                                 + ReflectTheme::Layout::programWellH);
    }

    /** The list runs from that row to the panel's bottom. **A measurement, not a maximum** -
        GUI-SPEC.md section 9 is explicit that it does not shrink-wrap and a short bank leaves empty
        glass below. Section 9's earlier 260px cap is not followed, for the same reason its "4px
        below the LCD" is not: the suite settled this shape and the root CLAUDE.md carries it.

        **553 on the pinned 648 canvas — panel bottom less the LCD's bottom edge, flush.**

        This briefly carried 537 with a derivation: 16 as the chassis inset, so the list's foot
        landed on the same frame the block observes. §12 of HEADER-PART Revision 4 withdraws that,
        and the reasoning is worth keeping rather than just the figure.

        **The 16 was found by looking for something 537 could be derived from.** It appears in no
        spec, no panel and no changelog as a list margin, so it was a reconstruction rather than a
        base — and confirming that the block really is inset 16 established only that the inset
        exists, which was never in question. **That a figure can be derived is not evidence it was.**

        And the fit ran the wrong way: five castings hang a flush list, so changing the shared
        contract to make this casting's transcribed 537 correct is the drift the shared part exists
        to prevent. The honest correction was the figure. */
    static int listHeight (int panelHeight) noexcept { return panelHeight - listTopY(); }

private:
    ReflectProgramList* programList = nullptr;
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

    /** One face, two permanently printed legends, each lit or not. There is no `enabled` and no
        label argument: the button never relabels and never wears a disabled face, so the only
        thing that varies is which of the two legends is illuminated. */
    void paintButton (juce::Graphics& g, juce::Rectangle<float> area,
                      const juce::String& topLegend, const juce::String& bottomLegend,
                      bool topLit, bool bottomLit);

    void paintLegend (juce::Graphics& g, const juce::String& text,
                      juce::Rectangle<float> lineBox, bool lit);

    Reflect84AudioProcessor& processorRef;

    // Mirrors of the processor's state, refreshed by the timer. Deliberately not written to while
    // naming - that is what makes cancelling free.
    /** The Program the panel is showing, mirrored so the poll only repaints on a real change. */
    ProgramId displayedId;

    /** The Programs the open menu was built from, in row order. */
    bool displayedIsModified = false;

    // The live readout and when it reverts. Held as text rather than a parameter pointer so the
    // display cannot outlive what it is showing.
    /** The parameter takeover: what to show, and until when. The deadline is core's; the font, the
        cell and every pixel of the paint stay here. */
    nf::ReadoutTimer readout { ReflectTheme::Layout::readoutFormat() };

    /** Whether the takeover was up at the last poll, so the timer repaints on the EDGE rather than
        every tick. The deadline itself lives in `readout`. */
    bool readoutWasShowing = false;

    bool namingMode = false;
    juce::String typedName;
    bool caretVisible = false;

    Region pressedRegion = Region::none;
    Region hoveredRegion = Region::none;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgramHeader)
};
