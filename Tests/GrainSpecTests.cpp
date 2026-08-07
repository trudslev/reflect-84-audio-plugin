#include "../Source/DSP/GrainSpec.h"

#include <juce_core/juce_core.h>

/**
    GrainSpec is the single interpretation of DIGITAL GRAIN shared by the audio path and the TANK
    LIVE scope, so these are the tests that keep the two honest. If the scope's step count and the
    audio's quantization can drift apart, the plugin's whole premise - that the display
    demonstrates what the knob is doing to the audio - stops being true.
*/
class GrainSpecTests final : public juce::UnitTest
{
public:
    GrainSpecTests() : juce::UnitTest ("GrainSpec", "dsp") {}

    void runTest() override
    {
        beginTest ("levels and stepPx match design/README.md section 6 verbatim");
        {
            // levels = max(3, round(30 - g*26)), stepPx = 3 + g*24, both only when g > 0.03.
            const auto atThreshold = GrainSpec::fromNormalized (0.031f);
            expect (atThreshold.active);
            expectEquals (atThreshold.levels, 29);
            expectWithinAbsoluteError (atThreshold.stepPx, 3.744f, 0.001f);

            const auto atDefault = GrainSpec::fromNormalized (0.46f);
            expectEquals (atDefault.levels, 18);
            expectWithinAbsoluteError (atDefault.stepPx, 14.04f, 0.001f);

            const auto atFull = GrainSpec::fromNormalized (1.0f);
            expectEquals (atFull.levels, 4);
            expectWithinAbsoluteError (atFull.stepPx, 27.0f, 0.001f);
        }

        beginTest ("the design mock's own readout reproduces exactly");
        {
            // design/screenshots/01-panel.png shows "GRAIN 46 . 18 STEP" at the default.
            const auto spec = GrainSpec::fromNormalized (0.46f);
            expect (spec.describe().contains ("GRAIN 46"));
            expect (spec.describe().contains ("18 STEP"));
        }

        beginTest ("below the threshold the stage is fully bypassed");
        {
            for (const float g : { 0.0f, 0.01f, 0.03f })
            {
                const auto spec = GrainSpec::fromNormalized (g);
                expect (! spec.active);
                expectEquals (spec.levels, 0);
                expectWithinAbsoluteError (spec.stepPx, GrainSpec::kSmoothStepPx, 0.0001f);
                expectWithinAbsoluteError (spec.holdRatio, 1.0f, 0.0001f);
                expectEquals (spec.holdSamples(), 1);
                // Built from the codepoint, not a \x escape: juce::String's const char*
                // constructor decodes as Latin-1, not UTF-8, so a literal "\xc2\xb7" here would
                // compare against mojibake rather than against U+00B7.
                expectEquals (spec.describe(),
                              "GRAIN OFF " + juce::String::charToString ((juce::juce_wchar) 0x00B7) + " SMOOTH");

                // Bypassed means bit-transparent, not "nearly transparent".
                for (const float x : { -1.0f, -0.3f, 0.0f, 0.12345f, 0.9f })
                    expect (spec.quantize (x) == x);
            }
        }

        beginTest ("holdRatio spans 1x to 9x and means the same in both domains");
        {
            // Pixels-per-step on the scope, samples-per-hold in the tank. Both are stepPx / 3.
            expectWithinAbsoluteError (GrainSpec::fromNormalized (1.0f).holdRatio, 9.0f, 0.0001f);
            expectEquals (GrainSpec::fromNormalized (1.0f).holdSamples(), 9);

            float previous = 0.0f;

            for (int i = 4; i <= 100; ++i)
            {
                const auto spec = GrainSpec::fromNormalized ((float) i / 100.0f);
                expect (spec.holdRatio >= previous);
                expect (spec.holdRatio >= 1.0f && spec.holdRatio <= 9.0f);
                previous = spec.holdRatio;
            }
        }

        beginTest ("audio word length lands in the intended range and only shortens");
        {
            // The span is fixed by levels (29 down to 4, so log2(29/4) = 2.86 bits); kHeadroom
            // only chooses where it sits. These bounds are what makes the knob a character
            // control rather than a destroy-everything effect.
            const float atThreshold = GrainSpec::fromNormalized (0.031f).bits();
            const float atFull = GrainSpec::fromNormalized (1.0f).bits();

            expectWithinAbsoluteError (atThreshold, 11.44f, 0.05f);
            expectWithinAbsoluteError (atFull, 8.58f, 0.05f);
            expectWithinAbsoluteError (atThreshold - atFull, 2.86f, 0.05f);

            float previousBits = 100.0f;

            for (int i = 4; i <= 100; ++i)
            {
                const float bits = GrainSpec::fromNormalized ((float) i / 100.0f).bits();
                expect (bits <= previousBits);
                previousBits = bits;
            }
        }

        beginTest ("quantize truncates toward zero onto its own step grid");
        {
            const auto spec = GrainSpec::fromNormalized (1.0f);
            const float step = spec.quantStep();

            expect (step > 0.0f);

            for (const float x : { -0.9f, -0.4f, -0.011f, 0.011f, 0.4f, 0.9f })
            {
                const float q = spec.quantize (x);

                // On the grid...
                expectWithinAbsoluteError (q / step - (float) juce::roundToInt (q / step), 0.0f, 1.0e-3f);
                // ...never further from zero than the input, and never more than one step away.
                expect (std::abs (q) <= std::abs (x) + 1.0e-6f);
                expect (std::abs (x - q) < step);
            }

            expect (spec.quantize (0.0f) == 0.0f);
        }

        beginTest ("envelope quantization rounds to exactly `levels` steps");
        {
            const auto spec = GrainSpec::fromNormalized (0.46f);   // 18 levels

            expectWithinAbsoluteError (spec.quantizeEnvelope (0.0f), 0.0f, 1.0e-6f);
            expectWithinAbsoluteError (spec.quantizeEnvelope (1.0f), 1.0f, 1.0e-6f);

            for (int i = 0; i <= 100; ++i)
            {
                const float q = spec.quantizeEnvelope ((float) i / 100.0f);
                const float steps = q * (float) spec.levels;
                expectWithinAbsoluteError (steps - (float) juce::roundToInt (steps), 0.0f, 1.0e-4f);
            }
        }

        beginTest ("out-of-range input is clamped rather than extrapolated");
        {
            expectEquals (GrainSpec::fromNormalized (-1.0f).levels, GrainSpec::fromNormalized (0.0f).levels);
            expectEquals (GrainSpec::fromNormalized (2.0f).levels, GrainSpec::fromNormalized (1.0f).levels);
        }
    }
};

static GrainSpecTests grainSpecTests;
