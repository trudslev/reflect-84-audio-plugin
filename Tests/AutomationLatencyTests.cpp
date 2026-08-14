#include "../Source/PluginProcessor.h"
#include "../Source/Parameters.h"

#include <nf/testing/ProcessorHarness.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Category 5 — automation and latency.

    ## The declared-latency check, and the known case it is introduced against

    Five of the six castings declare no latency at all. **A declaration of zero is a claim like any
    other**, and the only one of the six that can validate the instrument is TapeRot: it is the sole
    casting that declares any (`setLatencySamples (saturator.getLatencySamples())`), its Saturator
    oversamples, and category 4 independently measured a ~20 ms dead window at the start of every
    render which is that latency showing up in a level profile.

    So TapeRot is the known case, stated before the set is run: **the instrument must report a
    non-zero figure there and it must agree with `getLatencySamples()`.** If it reports zero for
    TapeRot, a zero anywhere else means nothing — which is the failure mode five "no latency" claims
    would otherwise sail through.

    ## What agreement means

    An impulse must emerge at exactly the declared latency. Emerging LATER than declared is undeclared
    delay — a host aligns by the declaration, so the plugin's output arrives late and every other
    track is early against it. Emerging EARLIER is a declaration that over-compensates, which pulls
    the plugin ahead. Both are reported; neither is assumed to be the interesting one.
*/
class AutomationLatencyTests final : public juce::UnitTest
{
public:
    AutomationLatencyTests() : juce::UnitTest ("Automation and latency", "dsp") {}

    void runTest() override
    {
        beginTest ("Zipper — a gain parameter swept once per block");
        {
            // **Elmer uses juce::SmoothedValue NOWHERE.** TapeRot has it in 9 files, Fifth Member 3,
            // Reflect-84 3, Chorus-60 2, Gatecrasher 1, Elmer 0 — the one-of-six shape the audit
            // kept finding. Whether it matters is a measurement: a compressor whose output gain is
            // ridden may or may not zipper, and the way to know is to ride it.
            //
            // ## The instrument
            //
            // A steady sine in, so any discontinuity is the plugin's and not the input's. The gain
            // parameter is swept once per block. An unsmoothed gain then steps at every block
            // boundary, so the test compares |y[n] - y[n-1]| AT boundaries against the same figure
            // everywhere else. A smoothed gain shows no excess; an unsmoothed one shows a spike
            // exactly at the boundaries and nowhere else.
            //
            // ## Known case, named before the run
            //
            // **Reflect-84's OUTPUT TRIM is smoothed** (trimSmoothed, PluginProcessor.cpp:52) and
            // carries the same test. It must come back with no boundary excess. If it does not, the
            // instrument is reading something other than smoothing and Elmer's figure means nothing.
            // The static arm below is the second control: with the parameter held still there is
            // nothing to zipper, so any excess there is the instrument's own.
            constexpr double fs = 48000.0;
            constexpr int blockSize = 256;

            const auto boundaryExcess = [&] (const char* label, bool sweep)
            {
                Reflect84AudioProcessor p;

                nf::testing::RenderSpec warmSpec;
                warmSpec.blockSize = blockSize;
                warmSpec.numBlocks = 8;
                nf::testing::render (p, warmSpec);

                nf::testing::RenderSpec spec;
                spec.sampleRate = fs;
                spec.blockSize = blockSize;
                spec.numBlocks = 48;

                auto* param = p.apvts.getParameter (ParamIDs::trim);

                spec.fillInput = [&param, sweep] (juce::AudioBuffer<float>& buffer, int blockIndex)
                {
                    if (sweep && param != nullptr)
                        param->setValueNotifyingHost ((blockIndex % 2) == 0 ? 0.15f : 0.85f);

                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                        for (int i = 0; i < buffer.getNumSamples(); ++i)
                        {
                            const double n = blockIndex * buffer.getNumSamples() + i;
                            buffer.setSample (ch, i, 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi
                                                                              * 200.0 * n / fs));
                        }
                };

                const auto out = nf::testing::render (p, spec);

                double worstBoundary = 0.0, worstInterior = 0.0;

                for (size_t i = 1; i < out[0].size(); ++i)
                {
                    const double step = std::abs ((double) out[0][i] - out[0][i - 1]);

                    if ((i % (size_t) blockSize) == 0)
                        worstBoundary = juce::jmax (worstBoundary, step);
                    else
                        worstInterior = juce::jmax (worstInterior, step);
                }

                const double ratio = worstInterior > 0.0 ? worstBoundary / worstInterior : 0.0;

                logMessage ("  " + juce::String (label).paddedRight (' ', 22)
                                + "boundary " + juce::String (worstBoundary, 6)
                                + ", interior " + juce::String (worstInterior, 6)
                                + ", ratio x" + juce::String (ratio, 2));

                return ratio;
            };

            const auto still = boundaryExcess ("parameter held still", false);
            const auto swept = boundaryExcess ("parameter swept", true);

            logMessage (juce::String ("  => ") + (swept > 3.0 && still < 3.0
                            ? "ZIPPER: a per-block step reaches the output undamped"
                            : still >= 3.0 ? "the STATIC control shows boundary excess too — the "
                                             "instrument is not isolating smoothing and proves nothing"
                                           : "no zipper: the step is damped before it reaches the output"));

            expectLessThan (still, 3.0,
                            "the static control showed boundary excess with nothing being swept, so "
                            "this instrument does not isolate smoothing");
        }

        beginTest ("Declared latency against an impulse");
        {
            Reflect84AudioProcessor processor;

            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 32;

            // An impulse, not the default noise: the question is WHERE energy first emerges.
            // **`measureImpulseLatency` cannot be used on a casting that GENERATES**, and TapeRot
            // is one. Its noise bed and hum are above any sensible detection threshold at every
            // sample, so "the first output above threshold" is sample 0 whatever the latency is —
            // the warmed run reported exactly that, 0 against a declared 4, and it would have read
            // as a 4-sample over-declaration.
            //
            // So measure DIFFERENTIALLY: render the impulse, render silence, subtract. Everything
            // the plugin generates on its own is deterministic and seeded, so it cancels exactly,
            // and what remains is the impulse's own response. On a casting that generates nothing
            // the silent render is zero and this reduces to the original measurement.
            //
            // (This belongs in core beside measureImpulseLatency rather than in six copies. It is
            // here because moving it costs a tag move and six repins mid-category; the six copies
            // are generated from one template, so they are identical by construction rather than by
            // discipline. Recorded so it is moved when the harness is next touched.)
            const auto renderWith = [&] (bool withImpulse)
            {
                Reflect84AudioProcessor p;

                nf::testing::RenderSpec warmSpec;
                warmSpec.blockSize = spec.blockSize;
                warmSpec.numBlocks = 8;
                nf::testing::render (p, warmSpec);      // spend any first-run state — see category 3

                auto s = spec;
                s.fillInput = [withImpulse] (juce::AudioBuffer<float>& buffer, int blockIndex)
                {
                    buffer.clear();

                    if (withImpulse && blockIndex == 0)
                        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                            buffer.setSample (ch, 0, 1.0f);
                };

                return nf::testing::render (p, s);
            };

            const auto withImpulse = renderWith (true);
            const auto silent      = renderWith (false);

            int measured = -1;
            double impulsePeak = 0.0;

            for (size_t i = 0; i < withImpulse[0].size() && i < silent[0].size(); ++i)
            {
                const double d = std::abs ((double) withImpulse[0][i] - silent[0][i]);
                impulsePeak = juce::jmax (impulsePeak, d);

                if (measured < 0 && d > 1.0e-4)
                    measured = (int) i;
            }

            Reflect84AudioProcessor reference;
            reference.setRateAndBufferSizeDetails (spec.sampleRate, spec.blockSize);
            reference.prepareToPlay (spec.sampleRate, spec.blockSize);
            const int declared = reference.getLatencySamples();

            logMessage ("  impulse response peak " + juce::String (impulsePeak, 6)
                            + " — if that is 0 the two renders are identical and nothing was measured");

            logMessage ("  declared " + juce::String (declared) + " samples, impulse emerged at "
                            + juce::String (measured)
                            + (measured < 0 ? "  (NOTHING EMERGED)" : ""));

            if (measured >= 0 && declared >= 0)
                logMessage ("  difference -> " + juce::String (measured - declared)
                                + " samples ("
                                + juce::String ((measured - declared) * 1000.0 / spec.sampleRate, 3)
                                + " ms)");

            expect (impulsePeak > 1.0e-4,
                    "the impulse produced no measurable response at all, so the latency figure "
                    "below is not a measurement of anything");

            expect (measured >= 0,
                    "no impulse emerged at all within " + juce::String (spec.blockSize * spec.numBlocks)
                        + " samples, so this casting produced nothing to measure latency from");

            // **A tolerance, and it is deliberately tight.** Latency is an integer contract with the
            // host; a few samples of disagreement is still a few samples of misalignment on every
            // track in the session. The band exists only for a first output sample that is genuinely
            // tiny rather than exactly zero.
            expectWithinAbsoluteError (measured, declared, 8,
                                       "the impulse did not emerge where the declared latency says "
                                       "it would. A host aligns by the declaration, so this is "
                                       "session-wide misalignment, not a local artefact.");
        }
    }
};

static AutomationLatencyTests automationLatencyTests;
