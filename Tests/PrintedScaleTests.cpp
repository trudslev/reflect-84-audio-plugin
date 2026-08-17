#include "../Source/Parameters.h"
#include "../Source/GUI/ReflectLookAndFeel.h"
#include "../Source/GUI/ReflectTheme.h"

#include <nf/PrintedScale.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
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

            checkNumerals (percentStdMarks, 5, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::densityPercent (f))); },
                            "percent (standard)");

            // **Both percent rings, because they are two arrays now.** The standard class prints
            // three numerals and the primary five, so one shared table stopped being expressible -
            // and a test that checked only one would pass while the other drifted.
            checkNumerals (percentPrimMarks, 9, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::densityPercent (f))); },
                            "percent (primary)");

            checkNumerals (widthMarks, 5, [] (float f)
                            { return juce::String (juce::roundToInt (ParamFormat::widthPercent (f))); },
                            "STEREO WIDTH");

            // **DAMPING HF is the case worth having.** Its mapping is logarithmic, so its marks are
            // at 0, 1/3, 2/3 and 1 - exact octaves over an exact three-octave range. If anyone
            // "tidies" that curve to linear, or evens out the fractions, these four numerals stop
            // being octaves and this fails. Linear in Hz would put the midpoint at 9 kHz, which is
            // what the mapping's own comment records rejecting.
            checkNumerals (dampHFMarks, 5, [] (float f)
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

        beginTest ("§2.1's numeral cut removed NUMERALS, not marks — the demoted values kept their ticks");
        {
            /*  **This is the property §2.1 actually claims**, and it is not the one the change is
                easy to make wrong in. Cutting the standard class from five numerals to three could
                be done two ways: drop the numeral and keep the tick, or drop the mark. Both leave a
                ring that draws and a panel that looks deliberate; only the first keeps the
                resolution, which §2.1 states in as many words — *"the retired values keep their
                ticks as minors, so the resolution is carried without the numerals."*

                So the assertion is against the PRE-CUT fractions, spelled as literals here. That is
                a second source: the arrays hold what the ring draws now, and these hold what it drew
                before, and a change that quietly dropped a mark moves one and not the other. */
            using ScaleMark = ReflectTheme::Layout::ScaleMark;

            const auto carriesMark = [] (const ScaleMark* marks, int count, float f)
            {
                for (int i = 0; i < count; ++i)
                    if (std::abs (marks[i].f - f) < 0.0005f)
                        return true;

                return false;
            };

            const auto keptEvery = [&] (const ScaleMark* marks, int count,
                                        const std::vector<float>& before, const juce::String& ring)
            {
                int majors = 0;

                for (int i = 0; i < count; ++i)
                    if (marks[i].isMajor())
                        ++majors;

                for (const float f : before)
                    expect (carriesMark (marks, count, f),
                            ring + ": the mark at fraction " + juce::String (f, 4)
                                 + " is gone. §2.1 cut the NUMERAL at that position, not the tick - "
                                   "dropping the mark loses the resolution the cut was meant to keep");

                logMessage ("  " + ring + ": " + juce::String (count) + " marks, "
                            + juce::String (majors) + " numeralled");

                expectEquals (majors, 3, ring + ": the standard class prints exactly three numerals");
            };

            const std::vector<float> quarters { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

            keptEvery (sizeMarks,       5, quarters, "SIZE");
            keptEvery (preDelayMarks,   5, quarters, "PRE-DELAY");
            keptEvery (percentStdMarks, 5, quarters, "percent (standard)");
            keptEvery (widthMarks,      5, quarters, "STEREO WIDTH");
            keptEvery (trimMarks,       5, quarters, "OUTPUT TRIM");

            keptEvery (decayMarks, 5, { 0.0f, 0.2105f, 0.4737f, 0.7368f, 1.0f }, "DECAY");
            keptEvery (dampLFMarks, 5, { 0.0f, 0.2744f, 0.5489f, 0.8233f, 1.0f }, "DAMPING LF");

            /*  **DAMPING HF is the one ring that is not the old one with numerals removed**, and it
                is worth asserting separately rather than folding into the loop. Its pre-cut ring had
                four marks at 0 / .3333 / .6667 / 1; §2.1 gives it five, adding a minor at .8333 that
                never existed. So its old set is a strict SUBSET rather than a re-labelling.

                That is the reason to read §2.1's minor column rather than infer minors from which
                numerals were dropped: inference would have produced four marks here and been wrong
                in a way nothing else on the panel would show. */
            keptEvery (dampHFMarks, 5, { 0.0f, 0.3333f, 0.6667f, 1.0f }, "DAMPING HF");

            expect (carriesMark (dampHFMarks, 5, 0.8333f),
                    "DAMPING HF lost the minor at .8333, which §2.1 ADDS - this ring is not the old "
                    "four-mark ring with a numeral removed");
        }

        beginTest ("The primary class keeps all five numerals, which is what makes it a class");
        {
            int majors = 0, minors = 0;

            for (const auto& m : percentPrimMarks)
                (m.isMajor() ? majors : minors) += 1;

            logMessage ("  percent (primary): " + juce::String (majors) + " numeralled, "
                        + juce::String (minors) + " minor");

            expectEquals (majors, 5, "the primary class prints five numerals - §2's 'up to five'");
            expectEquals (minors, 4, "with minors at the eighths between them");

            // The distinguishing property: if the two percent rings ever became one array again,
            // one of these two counts would have to be wrong. Asserting both is what keeps the
            // split honest rather than merely present.
            int stdMajors = 0;

            for (const auto& m : percentStdMarks)
                if (m.isMajor())
                    ++stdMajors;

            expectNotEquals (majors, stdMajors,
                             "the two percent rings print the same number of numerals, so the split "
                             "into two arrays is no longer doing anything");
        }

        beginTest ("Every ring passes the suite's shared structural check");
        {
            // The taper check above is this casting's own, because its ranges are all 0-1. These
            // are the checks that apply whatever the taper is, and they are the same ones the other
            // five castings run: marks in order, no two on the same tick, none outside the sweep.
            checkShared (sizeMarks,     5, "SIZE");
            checkShared (decayMarks,    5, "DECAY");
            checkShared (preDelayMarks, 5, "PRE-DELAY");
            checkShared (percentStdMarks,  5, "percent (standard)");
            checkShared (percentPrimMarks, 9, "percent (primary)");
            checkShared (widthMarks,    5, "STEREO WIDTH");
            checkShared (trimMarks,     5, "OUTPUT TRIM");
            checkShared (dampHFMarks,   5, "DAMPING HF");
            checkShared (dampLFMarks,   5, "DAMPING LF");
        }

        beginTest ("The knob cache is keyed on SCALE, not on value — shown by counting rebuilds");
        {
            /*  **Call 5's cache, asserted on the property that distinguishes it from the obvious
                wrong implementation.** `setBufferedToImage` would also compile, also look identical,
                and also report "cached" — while re-rendering the whole knob on every value change,
                because JUCE refreshes that buffer on every repaint and a Slider repaints whenever
                its value moves. The panel would be pixel-identical and the cache worth nothing.

                So the test drives values through one knob at a fixed scale and counts renders of
                the static layer. One rebuild for any number of values; a further rebuild only when
                the device scale changes. */
            ReflectKnob knob { ReflectTheme::Layout::KnobSize::standard,
                               { ReflectTheme::Layout::sizeMarks, 5, nullptr } };
            knob.setBounds (0, 0, 120, 120);

            const auto paintAt = [&] (float deviceScale, float value)
            {
                knob.setValue (value, juce::dontSendNotification);

                juce::Image target { juce::Image::ARGB,
                                     juce::roundToInt (120.0f * deviceScale),
                                     juce::roundToInt (120.0f * deviceScale), true };
                juce::Graphics g { target };
                g.addTransform (juce::AffineTransform::scale (deviceScale));
                knob.paint (g);
            };

            paintAt (1.0f, 0.10f);
            const int afterFirst = knob.staticLayerBuildCount();
            expectEquals (afterFirst, 1, "the first paint must render the static layer once");

            for (float v : { 0.25f, 0.40f, 0.55f, 0.70f, 0.85f, 1.00f })
                paintAt (1.0f, v);

            logMessage ("  6 further values at scale 1.0 -> "
                        + juce::String (knob.staticLayerBuildCount()) + " build(s) total");

            expectEquals (knob.staticLayerBuildCount(), 1,
                          "the static layer was re-rendered while only the VALUE changed, so this "
                          "is a per-frame buffer rather than a cache - which is exactly what "
                          "setBufferedToImage would have given, at no benefit");

            // The other direction, without which the arm above would pass on a cache that never
            // rebuilds at all — including one wrongly keyed on nothing, which would go soft on a
            // resize instead of costing a frame.
            paintAt (2.0f, 0.50f);

            logMessage ("  scale 1.0 -> 2.0 -> "
                        + juce::String (knob.staticLayerBuildCount()) + " build(s) total");

            expectEquals (knob.staticLayerBuildCount(), 2,
                          "a device-scale change did NOT rebuild the static layer, so the cache "
                          "would be blitted up from the wrong resolution and the knob would go soft "
                          "at any scale but the one it was first painted at");
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
        int majors = 0;

        for (int i = 0; i < count; ++i)
        {
            // **A minor mark has no numeral to check.** §2.1's numeral cut kept the demoted values
            // as ticks, so a ring's array now mixes both kinds; comparing a null against the
            // control's reading would fail every minor for having no opinion.
            if (! marks[i].isMajor())
                continue;

            ++majors;

            const auto printed = juce::String (marks[i].printed);
            const auto actual  = numeralAt (marks[i].f);

            expectEquals (actual, printed,
                          ring + ": the ring prints \"" + printed + "\" at fraction "
                              + juce::String (marks[i].f, 4)
                              + " but the control reads \"" + actual + "\" there");
        }

        // **A ring of nothing but minors would pass the loop above silently**, which is the vacuity
        // this guards: the loop's only failure mode is a mismatch, so zero comparisons is zero
        // failures. Named per ring so the message says which one went empty.
        expectGreaterThan (majors, 0, ring + ": no numeral was checked at all - every mark in this "
                                             "ring is a minor, so the comparison ran on nothing");
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
