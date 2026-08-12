#include "../Source/Parameters.h"
#include "../Source/GUI/ReflectTheme.h"

#include <nf/PrintedScale.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include <vector>

/**
    The printed scales, checked against the mappings that actually drive the pointer.

    **`ReflectTheme.h` claimed this file existed for some time before it did.** Its comment on the
    mark tables read "the tick-angle column is what the PrintedScaleTests assert against", and there
    was no such test — a claim of coverage is worse than none, because it stops anyone looking. The
    comment is now true.

    **Reflect-84 checks a different thing from its siblings, and the difference is structural.**
    Every parameter here has a plain 0-1 `NormalisableRange`; the taper lives in `ParamFormat`,
    which is the single place a normalised position becomes a physical value. So a mark's stored
    rotation fraction IS its parameter position, `range.convertTo0to1` is the identity, and
    comparing the two would assert nothing.

    What is worth asserting instead is stronger: **the numeral printed at a fraction must be what
    the control actually reads there.** If a mapping in `ParamFormat` changes and a ring does not,
    the panel prints numerals the pointer never reaches — the failure BRAND.md's printed-scale rule
    exists to prevent, and the one Fifth Member's siblings catch through the range.

    `nf::printedScaleDefects` still runs over every ring, for the structural checks that apply
    whatever the taper is: marks in order, no two sharing a tick, none outside the sweep.
*/
class PrintedScaleTests final : public juce::UnitTest
{
public:
    PrintedScaleTests() : juce::UnitTest ("PrintedScale", "GUI") {}

    void runTest() override
    {
        using namespace ReflectTheme::Layout;

        beginTest ("Every printed numeral is what the control reads at that fraction");
        {
            // The numerals are what the ring prints; the readouts carry a unit and, on three
            // controls, a different precision - so each case states what it expects the numeral to
            // be rather than comparing whole strings blindly.
            checkNumerals (sizeMarks, 5, [] (float f) { return juce::String (ParamFormat::sizeScale (f), 1); },
                            "SIZE");

            // **DECAY's ring prints "0.4" and then "2", "4", "6", "8".** A printed scale elides a
            // trailing ".0" where the readout does not - the readout is a live value and wants a
            // stable width, a silk-screened numeral wants to be short. That is a real difference in
            // convention rather than a discrepancy, so the comparator models it.
            checkNumerals (decayMarks, 5, [] (float f)
                            { return printedNumeral (ParamFormat::decaySeconds (f), 1); },
                            "DECAY");

            checkNumerals (preDelayMarks, 5, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::preDelayMs (f))); },
                            "PRE-DELAY");

            checkNumerals (percentMarks, 5, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::densityPercent (f))); },
                            "percent");

            checkNumerals (widthMarks, 5, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::widthPercent (f))); },
                            "STEREO WIDTH");

            // **DAMPING HF is the case worth having.** Its mapping is logarithmic, so its marks are
            // at 0, 1/3, 2/3 and 1 - exact octaves over an exact three-octave range. If anyone
            // "tidies" that curve to linear, or evens out the fractions, these four numerals stop
            // being octaves and this fails. Linear in Hz would put the midpoint at 9 kHz, which is
            // what the mapping's own comment records rejecting.
            checkNumerals (dampHFMarks, 4, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::dampHFHz (f) * 0.001f)); },
                            "DAMPING HF");

            checkNumerals (dampLFMarks, 5, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::dampLFHz (f))); },
                            "DAMPING LF");
        }

        beginTest ("DAMPING HF's marks really are octaves, and DAMPING LF's last step is short");
        {
            // Both are stated in ReflectTheme's own comment as deliberate. Asserting them stops a
            // later reader "correcting" either one: uneven spacing looks like a mistake.
            expectWithinAbsoluteError (ParamFormat::dampHFHz (0.0f),     2000.0f, 1.0f);
            expectWithinAbsoluteError (ParamFormat::dampHFHz (1.0f / 3.0f), 4000.0f, 1.0f);
            expectWithinAbsoluteError (ParamFormat::dampHFHz (2.0f / 3.0f), 8000.0f, 1.0f);
            expectWithinAbsoluteError (ParamFormat::dampHFHz (1.0f),    16000.0f, 1.0f);

            // 500 is not an octave above 320, so DAMPING LF's last interval is genuinely shorter
            // than the four before it. That is the spec, not a rounding error.
            const float lastStep  = dampLFMarks[4].f - dampLFMarks[3].f;
            const float priorStep = dampLFMarks[3].f - dampLFMarks[2].f;
            expect (lastStep < priorStep,
                    "DAMPING LF's final interval should be shorter - 500 Hz is not an octave above 320");
        }

        beginTest ("Every ring passes the suite's shared structural check");
        {
            // The taper check above is this casting's own, because its ranges are all 0-1. These
            // are the checks that apply whatever the taper is, and they are the same ones the other
            // five castings run: marks in order, no two on the same tick, none outside the sweep.
            checkShared (sizeMarks,     5, "SIZE");
            checkShared (decayMarks,    5, "DECAY");
            checkShared (preDelayMarks, 5, "PRE-DELAY");
            checkShared (percentMarks,  5, "percent");
            checkShared (widthMarks,    5, "STEREO WIDTH");
            checkShared (trimMarks,     5, "OUTPUT TRIM");
            checkShared (dampHFMarks,   4, "DAMPING HF");
            checkShared (dampLFMarks,   5, "DAMPING LF");
        }

        beginTest ("Every knob in the layout carries a scale with at least two marks");
        {
            // A ring with one mark legends nothing, and an empty one silently draws no scale at
            // all - which reads as the designers having omitted it rather than as a wiring fault.
            for (const auto& knob : knobs)
                expect (knob.scale.count >= 2,
                        juce::String (knob.label) + " has " + juce::String (knob.scale.count)
                            + " printed marks");
        }
    }

private:
    /** A value as a printed NUMERAL: up to `maxDecimals` places, with a trailing ".0" elided.

        A live readout keeps its decimal for a stable width; a silk-screened numeral drops it,
        because "8" and "8.0" are the same number and the shorter one crowds its neighbours less.
        DECAY prints "0.4" then "2", "4", "6", "8" for exactly that reason. */
    static juce::String printedNumeral (float value, int maxDecimals)
    {
        auto text = juce::String (value, maxDecimals);

        if (text.containsChar ('.'))
        {
            text = text.trimCharactersAtEnd ("0");

            if (text.endsWithChar ('.'))
                text = text.dropLastCharacters (1);
        }

        return text;
    }

    template <typename NumeralAt>
    void checkNumerals (const ReflectTheme::Layout::ScaleMark* marks, int count,
                        NumeralAt numeralAt, const juce::String& ring)
    {
        for (int i = 0; i < count; ++i)
        {
            const auto printed = juce::String (marks[i].printed);
            const auto actual  = numeralAt (marks[i].f);

            expectEquals (actual, printed,
                          ring + ": the ring prints \"" + printed + "\" at fraction "
                              + juce::String (marks[i].f, 4)
                              + " but the control reads \"" + actual + "\" there");
        }
    }

    void checkShared (const ReflectTheme::Layout::ScaleMark* marks, int count,
                      const juce::String& ring)
    {
        // The parameters are all plain 0-1, so the mark's fraction is its value.
        const juce::NormalisableRange<float> range { 0.0f, 1.0f };
        std::vector<nf::PrintedMark> printed;

        for (int i = 0; i < count; ++i)
            printed.push_back ({ marks[i].f, nf::sweepAngleDegrees (marks[i].f) });

        for (const auto& defect : nf::printedScaleDefects (range, printed))
            expect (false, ring + ": " + defect);
    }
};

static PrintedScaleTests printedScaleTests;
