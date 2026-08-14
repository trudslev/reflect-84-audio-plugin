#include "../Source/PluginProcessor.h"

#include <nf/testing/ProcessorHarness.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Category 1's lock question, MEASURED rather than read.

    ## Why this exists after the answer was already "reported"

    The claim "`processBlock` reaches no lock in any of the six" was established by grepping
    `processBlock`'s body for lock tokens, and later corroborated by a second code read — no DSP
    stage references a lock-holding class, and no casting's `processBlock` calls into Program state.
    Two agreeing code reads.

    **That is the same class of evidence as an uncrashing over-delivery.** The TapeRot bisect was
    measured, precise and wrong, because "nothing happened" is not evidence about an out-of-bounds
    write. A scoped survey saying "no lock in the body I looked at" is not evidence about a lock
    three calls down, and the sweep's own rule is that a survey has no reverse direction.

    ## Two questions, two drives

    | | Question | Drive |
    |---|---|---|
    | 1 | Does `processBlock`'s normal path take a lock? | time it with nothing contending |
    | 2 | Does the audio thread reach a lock via `setCurrentProgram`? | hammer `setCurrentProgram` from another thread while timing `processBlock` |

    **Timing the quiet path alone cannot answer question 1**, which is the trap this is scoped
    against: an uncontended lock is nearly free, so a fast quiet timing cannot distinguish "takes no
    lock" from "takes an uncontended one". Only contention separates them.

    And question 2 is the one with a finding already attached: `requestProgramChange` takes
    `pendingLock` and performs **two `free()`s inside it** (measured — a `ProgramId` assignment
    releases two `juce::String`s), on whatever thread calls it. **VST3 delivers a program change as
    an automatable parameter, so that thread can be the audio thread.**

    So a probe timing only the quiet path would report "lock-free" while the known contention sat
    one entry point away. **"Lock-free in steady state" and "lock-free" are different claims and only
    one of them is true here.**
*/
class LockContentionTests final : public juce::UnitTest
{
public:
    LockContentionTests() : juce::UnitTest ("Lock contention", "dsp") {}

    void runTest() override
    {
        beginTest ("processBlock, quiet against contended by a concurrent program change");
        {
            Reflect84AudioProcessor processor;

            const auto quiet = nf::testing::timeProcessBlock (processor, 48000.0, 512, 2, 400);
            logMessage ("  quiet     -> " + quiet.describe());

            // The contender drives the REAL entry point, not a synthetic lock grab: this is the call
            // a VST3 host makes, and the one that reaches pendingLock.
            std::atomic<int> next { 0 };

            const auto contended = nf::testing::timeProcessBlock (
                processor, 48000.0, 512, 2, 400,
                [&processor, &next]
                {
                    processor.setCurrentProgram (next.fetch_add (1) % processor.getNumPrograms());
                });

            logMessage ("  contended -> " + contended.describe());

            const auto medianRatio = quiet.medianNs > 0.0 ? contended.medianNs / quiet.medianNs : 0.0;
            const auto tailRatio   = quiet.p95Ns > 0.0 ? contended.p95Ns / quiet.p95Ns : 0.0;

            logMessage ("  median x" + juce::String (medianRatio, 2)
                            + ", p95 x" + juce::String (tailRatio, 2));

            // **Reported, and the assertion is deliberately loose.** A machine under load moves these
            // figures around, and the sweep does not fix anything: what matters is whether the
            // contended run shows the ORDER-OF-MAGNITUDE stall a contended lock produces, not
            // whether it is 1.1x or 1.3x. A 10x tail would be a finding; noise is not.
            expect (quiet.samples > 0 && contended.samples > 0, "no timing samples collected");

            expectLessThan (tailRatio, 10.0,
                            "processBlock's tail latency grew 10x while another thread drove "
                            "setCurrentProgram, which means it contends for pendingLock: quiet "
                                + quiet.describe() + " / contended " + contended.describe());
        }

        beginTest ("setCurrentProgram itself — the call VST3 may deliver on the audio thread");
        {
            // The other half, and the one already known to touch the heap. This does not assert a
            // budget: it records what the call costs, so the ruling on those two frees has a figure
            // rather than an argument.
            Reflect84AudioProcessor processor;
            processor.prepareToPlay (48000.0, 512);

            std::atomic<int> next { 0 };

            const auto cost = nf::testing::timeCallable (
                [&processor, &next]
                {
                    processor.setCurrentProgram (next.fetch_add (1) % processor.getNumPrograms());
                }, 400);

            logMessage ("  setCurrentProgram -> " + cost.describe());

            // For scale: one 512-sample block at 48 kHz is 10 667 us of wall clock budget... no, of
            // real time. 512 / 48000 = 10.67 ms = 10 667 us. A call costing a few microseconds is
            // not a dropout on its own; one costing milliseconds would be.
            logMessage ("  for scale, one 512-sample block at 48 kHz is 10667 us of real time");

            expect (cost.samples > 0);
        }
    }
};

static LockContentionTests lockContentionTests;
