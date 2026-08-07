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

private:
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

    bool namingMode = false;
    juce::String typedName;
    bool caretVisible = false;

    Region pressedRegion = Region::none;
    Region hoveredRegion = Region::none;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgramHeader)
};
