#include "../Source/PluginProcessor.h"

#include <nf/testing/ProcessorHarness.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Category 2 of the suite-wide bug sweep, for REFLECT-84.

    ## The scanner is the PRIMARY instrument here, not the survey's confirmer

    Category 2's survey — "where does state decay" — is answered by looking where decay was expected,
    and it has the blind spot every scoped survey has: it finds **named** decaying state and cannot
    find an accumulator called `s` in a loop. That is the same shape as the `PluginProcessor.cpp`
    scoping error that hid two `dryBuffer` growth sites.

    So the scanner runs against the **whole audio path** — every sample this processor emits — rather
    than only against the paths the grep named. An unnamed accumulator reaching subnormal territory
    is exactly the finding the survey cannot produce and this can.

    **Where the scanner finds something the survey did not list, that is a finding about the survey
    as well as about the casting**, and both get recorded.

    ## What a positive result would mean, stated before running

    `processBlock` opens with `juce::ScopedNoDenormals`, which sets the CPU's flush-to-zero mode for
    its duration. So on a correctly guarded path, a subnormal intermediate is flushed to zero by the
    hardware and **never reaches the output**.

    That makes the scanner's reading unusually sharp: **subnormals in the output mean the guard is
    not covering that path.** A clean result is not "no denormals happen" — it is "the guard is doing
    its job everywhere the output can see", which is the useful claim.

    It is also the limit. Subnormals in *internal state* that never reach the output are invisible
    here, and on a platform where FTZ is honoured they cost nothing anyway. Stated so the next reader
    does not read a clean row as more than it is.

    ## Why this casting first

    Two leads from the survey, both inferred, both one-of-six shapes:

      - `ScopedNoDenormals` appears in exactly **one file per casting** — the processor. No DSP stage
        carries its own guard, in any of the six.
      - Tiny-constant / flush guards range from TapeRot's 10 down to **Reflect-84's 2**, and this is
        the casting with four tanks. If any casting has something to say here, the survey says it is
        this one.
*/
class NumericalRobustnessTests final : public juce::UnitTest
{
public:
    NumericalRobustnessTests() : juce::UnitTest ("Numerical robustness", "dsp") {}

    void runTest() override
    {
        beginTest ("A long decaying tail, whole audio path, every algorithm");
        {
            // The per-casting meaning core must not have: which parameters make this reverb decay
            // longest, and that ALGORITHM selects between four independent tanks — so a scan of one
            // says nothing about the other three.
            for (int algorithm = 0; algorithm < 4; ++algorithm)
            {
                Reflect84AudioProcessor processor;

                // **Normalised, not the raw index.** The first version passed (float) algorithm
                // straight in, so 1/2/3 all clamped to 1.0 and selected the LAST choice: the loop
                // claimed four algorithms and tested two. The tell was three byte-identical peak and
                // silence figures — a metric that cannot distinguish what it is asked to rank looks
                // exactly like a result.
                set (processor, ParamIDs::algorithm, (float) algorithm / 3.0f);
                set (processor, ParamIDs::size,   1.0f);
                set (processor, ParamIDs::decay,  1.0f);   // longest tail this plugin has
                set (processor, ParamIDs::mix,    1.0f);   // fully wet: nothing dilutes the tail
                set (processor, ParamIDs::dampHF, 1.0f);   // HF damping OPEN — see ReflectTheme note
                set (processor, ParamIDs::dampLF, 0.0f);   // LF damping open, opposite direction

                nf::testing::RenderSpec spec;
                spec.numBlocks = 16;

                // **Thousands of blocks, not tens.** A decaying value is normal for its first
                // hundred-odd halvings, so a short tail scans the loud part and reports clean —
                // which is how a real denormal problem hides.
                const auto report = nf::testing::scanTail (processor, spec, 4000);

                // Log what the parameter actually reads, not what the loop index says — the two
                // disagreeing is the defect this comment describes.
                const auto* chosen = processor.apvts.getParameter (ParamIDs::algorithm);
                logMessage ("  algorithm " + juce::String (algorithm) + " (\""
                                + (chosen != nullptr ? chosen->getCurrentValueAsText() : juce::String ("?"))
                                + "\") -> " + report.describe());

                expectEquals (report.nans, 0, "NaN in algorithm " + juce::String (algorithm));
                expectEquals (report.infinities, 0, "Inf in algorithm " + juce::String (algorithm));

                // Subnormals reaching the output mean ScopedNoDenormals is not covering this path.
                expectEquals (report.subnormals, 0,
                              "subnormals reached the output in algorithm " + juce::String (algorithm)
                                  + " — the flush-to-zero guard is not covering it: "
                                  + report.describe());
            }
        }

        beginTest ("The tail actually reaches silence rather than stalling");
        {
            // A tail that never falls below the threshold is either a very long reverb or state that
            // has stopped decaying. Which one is a per-casting judgement, so it is logged and only
            // the finite-ness is asserted.
            Reflect84AudioProcessor processor;
            set (processor, ParamIDs::decay, 1.0f);
            set (processor, ParamIDs::mix, 1.0f);

            nf::testing::RenderSpec spec;
            spec.numBlocks = 16;

            const auto report = nf::testing::scanTail (processor, spec, 8000);
            logMessage ("  longest decay -> " + report.describe());

            expect (report.clean(), "non-finite or subnormal output in the long tail");
        }
    }

private:
    static void set (Reflect84AudioProcessor& p, const char* id, float normalised)
    {
        if (auto* param = p.apvts.getParameter (id))
            param->setValueNotifyingHost (normalised);
    }
};

static NumericalRobustnessTests numericalRobustnessTests;
