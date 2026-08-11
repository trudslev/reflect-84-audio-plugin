#include "TestUtils.h"
#include "../Source/GUI/ReflectTheme.h"
#include "../Source/DSP/FactoryPrograms.h"
#include "../Source/DSP/ProgramManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

/**
    The PROGRAM display's character budget, **measured against the font that is actually drawn**.

    This casting had the budget wrong twice over. The theme declared 16px / .13em while
    ProgramHeader hard-coded 17px / .16em at the draw call, so the declared 36-character budget
    described a string nothing rendered; and the 22-character name cap had no stated derivation at
    all. Both are now taken from the same constants the paint path uses, and measured here rather
    than asserted from arithmetic on figures that disagreed.
*/
class DisplayBudgetTests final : public juce::UnitTest
{
public:
    DisplayBudgetTests() : juce::UnitTest ("PROGRAM display budget", "GUI") {}

    void runTest() override
    {
        using namespace ReflectTheme;

        // **Two faces, because the cell uses two.** The bank tag is the LCD's general 16px type;
        // the program name is a point larger. Measuring the name at the bank's size - or sizing the
        // bank cell with the name's - is exactly the conflation that made the old budget wrong.
        const auto bankFont = Font::mono (Layout::lcdTextSize);
        const float bankTracking = Font::trackingPx (Layout::lcdTextTracking, Layout::lcdTextSize);

        const auto font = Font::mono (Layout::lcdNameTextSize);
        const float tracking = Font::trackingPx (Layout::lcdNameTextTracking, Layout::lcdNameTextSize);

        // The name area exactly as ProgramHeader::paint builds it: the well, less the bank cell
        // (whose width is the tracked width of "FACT" AT THE BANK'S FONT plus its padding, computed
        // at runtime), less the chevron inset.
        const float bankCellW = Layout::lcdBankPadX * 2.0f
                                 + Text::trackedWidth ("FACT", bankFont, bankTracking);
        const float nameAreaW = Layout::programWellW - (bankCellW + 1.0f)
                                 - (Layout::lcdChevronInsetRight + 18.0f);

        // Per character, from the font itself rather than a quoted figure. A 20-character sample
        // divided by 20 averages out any single glyph's width.
        const juce::String sample ("MMMMMMMMMMMMMMMMMMMM");
        const float advance = Text::trackedWidth (sample, font, tracking) / (float) sample.length();

        const int budget = (int) std::floor (nameAreaW / advance);

        logMessage ("  name area " + juce::String (nameAreaW, 1) + "px, advance "
                    + juce::String (advance, 2) + "px, budget " + juce::String (budget)
                    + " characters at " + juce::String (Layout::lcdNameTextSize, 0) + "px");

        beginTest ("The measured budget is what the theme declares");
        expectEquals (budget, Layout::lcdCharacterBudget,
                      "lcdCharacterBudget must be measured against the font the paint path draws");

        // **Three cases, because the display has three shapes now.** A single
        // prefix + cap + marker formula would be wrong: only FACTORY Programs carry the "NN "
        // prefix, and only the naming field carries a cursor.
        beginTest ("Factory: the longest name plus its NN prefix fits");
        {
            constexpr int indexPrefix = 3;      // "01 "
            int longest = 0;
            juce::String longestName;

            for (const auto& fp : kFactoryPrograms)
                if (const juce::String n { fp.name }; n.length() > longest)
                {
                    longest = n.length();
                    longestName = n;
                }

            expect (indexPrefix + longest + dirtyMarker <= budget,
                    "\"" + longestName + "\" overruns the name cell with its index");
        }

        beginTest ("User: a maximum-length name fits, with no prefix");
        {
            // User Programs carry no number - they sort alphabetically, so one would change
            // whenever a Program was saved. That is what freed the characters the cap gained.
            expect (ProgramManager::maxProgramNameLength + dirtyMarker <= budget);
        }

        beginTest ("Naming: a maximum-length name plus the cursor fits");
        {
            constexpr int cursorCell = 1;
            expect (ProgramManager::maxProgramNameLength + cursorCell <= budget);
        }

        beginTest ("The cap is the budget less whichever of the marker and the cursor is larger");
        {
            // A relationship between independent constants, not a hard-coded result: this is what
            // catches one of them moving without the other.
            constexpr int cursorCell = 1;
            expectEquals (ProgramManager::maxProgramNameLength,
                          budget - juce::jmax (dirtyMarker, cursorCell));
        }
    }

private:
    // " *", which every casting now draws. TapeRot and REFLECT-84 gated dirty through SAVE alone
    // until this change; the caps are sized for the marker from the start rather than having to
    // contract by two if it were added later.
    static constexpr int dirtyMarker = 2;
};

static DisplayBudgetTests displayBudgetTests;
