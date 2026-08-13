#pragma once

#include "ReflectTheme.h"
#include "../DSP/FactoryPrograms.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

/**
    The Program list: **a Component, not a `juce::PopupMenu`.**

    GUI-SPEC.md § 9 *The Program list* replaced *Dropdown — follows TapeRot* in the 2026-08-12
    bundle, and three of its requirements are things `PopupMenu` structurally cannot do — no amount
    of look-and-feel reaches them:

    - **The height is a measurement, not a maximum.** 554px, panel bottom less LCD bottom, and
      *"it does not shrink-wrap the rows — a short list leaves empty glass below."* `PopupMenu`
      always shrink-wraps to its content.
    - **No platform scrollbar in any state**, with 20px opaque chevron bands at the ends and the
      list *inset* by them so no row ever passes underneath. `PopupMenu` owns its own scrolling
      furniture.
    - **A chevron click scrolls exactly 104px**, four item rows.

    **This is not recorded as Reflect-84 diverging, nor as the suite's new direction.** What is on
    the record is the evidence: four separate fights with `PopupMenu` sit behind the flush-to-display
    contract in the root `CLAUDE.md` — the `menuHost` sibling, the 1px anchor strip that stops JUCE
    sliding the whole display rectangle down, the 8px lead that stops it opening one pixel low, and
    the parent-coordinate width — and now three requirements it cannot meet at all. Whether the other
    five castings follow gets decided when one of them next needs list work, against its own design
    rather than against this one.

    **What this retires here.** `menuHost`, `menuAnchorY`, `menuHostTop` and `menuHostHeight` all
    existed to make `PopupMenu` land in the right place. A Component sets its own bounds, so they go.
*/
class ReflectProgramList final : public juce::Component
{
public:
    ReflectProgramList();

    /** Rebuilds the list. `current` is marked with the lit bar. */
    void setPrograms (std::vector<ProgramId> programs,
                      const ProgramId& current,
                      const std::function<juce::String (const ProgramId&)>& labelFor);

    /** Fired with the chosen Program. The caller closes the list. */
    std::function<void (const ProgramId&)> onProgramChosen;

    /** Fired when a click lands on the list but on nothing selectable, so the caller can decide
        whether that dismisses. */
    std::function<void()> onDismissRequested;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    //==============================================================================
    /** § 9's row table. Pinned, and never grown to any platform standard. */
    static constexpr int itemHeight        = 26;
    static constexpr int captionHeight     = 22;
    static constexpr int separatorHeight   = 9;
    static constexpr int placeholderHeight = 26;

    /** 20px, opaque, at each end. When present the list is inset by them, so a row never passes
        under a band — they are fixed furniture rather than a fade over moving content. */
    static constexpr int chevronBandHeight = 20;

    /** **104px, and it is four item rows rather than a round number.** Scrolling by a whole number
        of rows means a row is never left half-under a band. */
    static constexpr int scrollStep = 4 * itemHeight;

    static constexpr int textInsetLeft  = 16;   // the LCD's own, so the two read as one column
    static constexpr int textInsetRight = 14;
    static constexpr int markerWidth    = 3;

private:
    struct Row
    {
        enum class Kind { item, caption, separator, placeholder };

        Kind kind = Kind::item;
        ProgramId id;
        juce::String text;
        bool isCurrent = false;

        int height() const noexcept
        {
            switch (kind)
            {
                case Kind::caption:     return captionHeight;
                case Kind::separator:   return separatorHeight;
                case Kind::placeholder: return placeholderHeight;
                case Kind::item:
                default:                return itemHeight;
            }
        }

        bool selectable() const noexcept { return kind == Kind::item; }
    };

    int contentHeight() const noexcept;
    int viewportHeight() const noexcept;
    bool scrollable() const noexcept;
    int maxScroll() const noexcept;
    void setScroll (int y);

    /** The row under a point, or -1. Accounts for the scroll offset and the bands. */
    int rowAt (juce::Point<int> p) const;

    juce::Rectangle<int> topBandArea() const;
    juce::Rectangle<int> bottomBandArea() const;

    void paintRow (juce::Graphics&, const Row&, juce::Rectangle<int> area, bool hovered) const;
    void paintBands (juce::Graphics&) const;
    void paintChevron (juce::Graphics&, juce::Rectangle<int> band, bool pointingUp, bool enabled) const;

    std::vector<Row> rows;
    int scrollY = 0;
    int hoveredRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReflectProgramList)
};
