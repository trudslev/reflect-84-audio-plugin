#include "../Source/PluginProcessor.h"
#include "../Source/DSP/ReverbPrimitives.h"

#include <nf/testing/ProcessorHarness.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Category 3 for REFLECT-84 — and it is TWO tests with two different bars.

    | | Bar | Why |
    |---|---|---|
    | **Block size** | **sample-exact, zero tolerance** | re-cutting one sample stream into different blocks is not a change to the signal, so any difference is an assumption about block boundaries |
    | **Sample rate** | every duration and every **frequency** keeps its value in **its own units** | the same seconds of audio at 44.1 and 96 kHz cannot be sample-exact and should not be |

    Sample-exact would be the wrong bar for the second and would fail every correct casting.

    ## What must survive a rate change here, and what need not

    The test is *must this value survive a rate change in its own units*, **not** *is it a duration*.
    A frequency is exactly the quantity that must survive — a 500 Hz corner is still 500 Hz at
    96 kHz — and filter coefficients derived from a normalised frequency rather than from `fs` are
    where rate bugs actually live.

    | Must survive | Need not |
    |---|---|
    | `decay` (0.4–8.0 s), `preDelay` (0–180 ms), **`dampHF` (2–16 kHz)**, **`dampLF` (40–500 Hz)** | `size` (a 0.2–1.0 scale), `density` (%), `modulation` (%), `grain`, `width` (%), `mix` (%), `trim` (dB) |

    `dampHF` and `dampLF` were in the *second* column in the sweep's first table, filed as "geometry
    and level". That was the criterion applied inconsistently — TapeRot's wow and Chorus-60's rate
    are also Hz and were correctly in the first — and correcting it moved them here.

    ## `dampHF` is a 2×2, because the top of its range is near Nyquist

    At 44.1 kHz, `dampHF`'s 16 kHz top end sits at **0.726 of Nyquist**. That is close enough that a
    filter can misbehave near the top of its own range for reasons having nothing to do with sample
    rate, so a difference measured there needs distinguishing from a rate bug rather than being read
    as one.

    | | 44.1 kHz | 192 kHz |
    |---|---|---|
    | `dampHF` at 2 kHz | 0.091 of Nyquist | 0.021 |
    | `dampHF` at 16 kHz | **0.726** — the suspect corner | 0.167 |

    **`dampLF` is the control**, and a good one: its worst case, 500 Hz at 44.1 kHz, is **0.023 of
    Nyquist** — thirty times further from the edge. It cannot produce a Nyquist artefact at any rate
    here, so `dampLF` holding while `dampHF`'s top end moves is what isolates the cause.

    **Reading the four**, decided before the numbers arrived:

    | Pattern | Reading |
    |---|---|
    | all four hold | rate-correct across the whole range |
    | both ends move together | a **rate** bug |
    | only 16 kHz at 44.1 moves | a **range** finding, its own thing in the report |
    | **anything else** | **UNCLASSIFIED — report the four figures and stop** |

    That last row matters: a 2×2 has more patterns than the three named, and pushing an
    unanticipated result into the nearest bucket produces a confident misfiling, which reads exactly
    like a finding and is worse than a gap.

    ## Verified by RESPONSE, never by coefficient readback

    A filter computed from a normalised frequency still *reports* the cutoff it was asked for; only
    its response moves. The instrument plays sine tones and reads output RMS, and it was validated
    against an independent processor-level measurement on TapeRot — the one casting whose filters sit
    on the audio path. These do not: Reflect-84's damping is inside the tanks, so the callable is the
    only instrument that reaches it.
*/
class InvarianceTests final : public juce::UnitTest
{
public:
    InvarianceTests() : juce::UnitTest ("Invariance", "dsp") {}

    void runTest() override
    {

        beginTest ("FINDING — pre-delay glides up from zero on the FIRST run only");
        {
            // **This began as premise check 1 and it is the reason the whole category needed one.**
            // blockSizeInvariance compares each size against the first, so its first row is 64
            // against 64 — and that row reported DIFFERS. A self-comparison that differs means the
            // processor is not reproducible run to run, in which case every other row is measuring
            // non-determinism rather than block dependence. All four rows were.
            //
            // ## The defect
            //
            // `ReverbEngine::prepare` gives `preDelaySmoothed` only `reset (sampleRate, 0.05)` and
            // never a `setCurrentAndTargetValue`. It is the only smoother in this casting without
            // one — `switchCrossfade` gets one two lines above it (ReverbEngine.cpp:32) and the
            // processor's mix/trim/width trio get theirs in prepareToPlay (PluginProcessor.cpp:54).
            //
            // JUCE's `reset (rate, seconds)` is `setCurrentAndTargetValue (target)`
            // (juce_SmoothedValue.h:274-278) — checked rather than recalled. On a constructed object
            // `target` is 0; on a re-prepared one it is the pre-delay the engine last held. So the
            // FIRST prepare of an instance ramps pre-delay up from zero over 50 ms and no later one
            // does, and a player hears a pre-delay glide at the start of the first playback after
            // the plugin is inserted, once per instance.
            //
            // ## Why it is invisible above 50 ms, which is what the sweep below pins
            //
            // During the ramp the read pointer trails the write pointer by less than the target, so
            // it reads samples the constant-delay run has not reached yet. But if the target is
            // GREATER than the ramp length, every position the ramp passes through is still inside
            // the un-filled part of the line, so both runs read zeros and the glide is inaudible.
            // The boundary is the ramp length itself: 0.05 s x 48 kHz = 2400 samples.
            //
            // ## Three wrong diagnoses got here, and the third is the instructive one
            //
            // The tail surviving prepareToPlay (wrong — it does, but the processor overrides no
            // reset() at all, so nothing changed when render() started calling it). The algorithm
            // crossfade (wrong — the default is Plate = 0, matching the constructed
            // currentAlgorithm, so it never fires). And then this same smoother, PROBED AT 0.0 AND
            // 0.5, which come to 0 and 4320 samples — one below the ramp and one above it, the only
            // two values in range that show nothing. Both arms came back exact and the correct
            // diagnosis was dropped as a story that fit.
            //
            // **The rule this sweep already had was applied to one arm and not the other.** The
            // "it must vanish" arm was proved able to fail; the "it must come back" arm was run on
            // a processor whose first run was already spent, so it could not have failed either way.
            // A probe value chosen without asking what it can distinguish is the same defect as a
            // comparison never proved able to fail.
            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 16;

            // 180 ms full scale at 48 kHz: normalised n is n * 8640 samples. Ramp is 2400.
            constexpr double rampSamples = 0.05 * 48000.0;

            for (float normalised : { 0.0f, 0.1f, 0.2f, 0.25f, 0.11f, 0.22f, 0.33f, 0.5f })
            {
                Reflect84AudioProcessor fresh;
                set (fresh, ParamIDs::preDelay, normalised);

                const auto result = nf::testing::compareRenders (nf::testing::render (fresh, spec),
                                                                 nf::testing::render (fresh, spec));

                const double delaySamples = (double) normalised * 8640.0;
                const bool glideShows = delaySamples > 0.0 && delaySamples < rampSamples;

                logMessage ("  " + juce::String (normalised, 2) + " -> "
                                + juce::String (delaySamples, 1) + " samples, "
                                + (glideShows ? "inside" : "outside") + " the 2400-sample ramp: "
                                + result.describe());

                expect (result.sampleExact != glideShows,
                        "the first-run difference did not follow the ramp boundary at "
                            + juce::String (delaySamples, 1) + " samples: " + result.describe());
            }
        }

        beginTest ("PREMISE CHECK — a WARMED processor is reproducible, which is what the rest assume");
        {
            // Every driver below runs on a warmed processor for exactly this reason, and the
            // accommodation is stated rather than silent: it does not hide the finding above, which
            // has its own test and its own assertions. What it does is let an invariance result mean
            // block size or sample rate rather than "this instance had never been prepared before".
            Reflect84AudioProcessor processor;
            warm (processor);

            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 16;

            const auto same = nf::testing::compareRenders (nf::testing::render (processor, spec),
                                                           nf::testing::render (processor, spec));

            logMessage ("  warmed, same spec twice -> " + same.describe());

            expect (same.sampleExact,
                    "a warmed processor still differs from itself, so a SECOND source of "
                    "non-determinism exists and no invariance result below means anything: "
                        + same.describe());
        }

        beginTest ("PREMISE CHECK — is dampHF's CUTOFF right at both rates, or only its far field?");
        {
            // "Both ends move" was pre-labelled a rate bug. That label must not be applied without
            // checking WHAT moved: a one-pole's -3 dB point can be exactly right at both rates while
            // its far-field shape differs, because the shape depends on normalised frequency and
            // 2 kHz is a different fraction of 44.1 k than of 192 k.
            //
            // A correct one-pole reads -3.01 dB AT its own cutoff. That is the number that says
            // whether the coefficient is right.
            for (float cutoff : { 2000.0f, 16000.0f })
                for (double fs : { 44100.0, 192000.0 })
                {
                    ReverbPrimitives::OnePoleLP filter;
                    filter.setCutoff (cutoff, fs);

                    const auto at = nf::testing::measureMagnitudeResponse (
                        [&filter] (float x) { return filter.process (x); },
                        [&filter] { filter.reset(); },
                        fs, { (double) cutoff });

                    logMessage ("  cutoff " + juce::String (cutoff, 0) + " Hz at "
                                    + juce::String (fs / 1000.0, 1) + " kHz -> "
                                    + juce::String (at[0].gainDb, 3) + " dB at its own corner"
                                    + "  (a correct one-pole reads -3.01)");
                }
        }
        beginTest ("Block size — sample-exact at 64 / 128 / 511 / 2048");
        {
            Reflect84AudioProcessor processor;
            warm (processor);                    // see the pre-delay finding above

            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 64;

            const auto results = nf::testing::blockSizeInvariance (processor, spec,
                                                                   { 64, 128, 511, 2048 });

            for (const auto& r : results)
                logMessage ("  " + r.describe());

            // 511 is prime and shares no factor with the others, so it catches any assumption that a
            // block divides evenly into an internal chunk — the failure a 64/128/2048 sweep walks
            // past because all three share factors.
            for (const auto& r : results)
                expect (r.sampleExact,
                        "block-size invariance failed — the same sample stream cut differently "
                        "produced different output: " + r.describe());
        }

        beginTest ("Sample rate — the decay measured in SECONDS at 44.1 / 48 / 96 / 192");
        {
            Reflect84AudioProcessor processor;

            set (processor, ParamIDs::decay, 0.5f);
            set (processor, ParamIDs::mix, 1.0f);

            nf::testing::RenderSpec spec;
            spec.numBlocks = 16;

            const auto rows = nf::testing::sampleRateSweep (processor, spec,
                                                            { 44100.0, 48000.0, 96000.0, 192000.0 },
                                                            8000);

            for (const auto& row : rows)
                logMessage ("  " + row.describe());

            // A rate requested but not adopted is a FINDING, not a row to drop: a casting that
            // refuses 192 kHz is making a statement about what it supports.
            for (const auto& row : rows)
                expect (row.rateWasAdopted(),
                        "this casting did not adopt a requested rate — that is a finding about what "
                        "it supports, not a row to skip: " + row.describe());

            // **The bar: seconds, not samples.** A decay of N seconds must stay N seconds. The
            // tolerance is generous because the threshold crossing is quantised to a block, and a
            // block is a different duration at each rate.
            double shortest = 1.0e9, longest = 0.0;

            for (const auto& row : rows)
                if (row.measuredSeconds >= 0.0)
                {
                    shortest = juce::jmin (shortest, row.measuredSeconds);
                    longest = juce::jmax (longest, row.measuredSeconds);
                }

            logMessage ("  spread -> " + juce::String (shortest, 4) + " s to "
                            + juce::String (longest, 4) + " s");

            expect (longest <= shortest * 1.25,
                    "the decay's duration in SECONDS changed across sample rates, which is the "
                    "rate-dependence this bar exists to catch: " + juce::String (shortest, 4)
                        + " s to " + juce::String (longest, 4) + " s");
        }

        beginTest ("dampHF across BOTH ends of its range and BOTH rate extremes — the 2x2");
        {
            // **Classified from the CORNER, and the first version could not be.**
            //
            // All three pre-stated readings turn on WHERE the curve moved — both ends together,
            // only the top corner at 44.1, or all four holding. `largestResponseDifferenceDb`
            // returns one number for a whole curve, so it cannot separate a corner that has shifted
            // from a far field whose shape differs, which is the only distinction the readings are
            // built on. It said "both ends move, 1.861 dB and 1.376 dB" — and the 2 kHz corner is
            // correct at BOTH rates, so that reading was wrong in the direction that would have
            // filed a range defect as a rate defect.
            //
            // This is not the fourth rule. That rule covers a PATTERN the readings do not name; an
            // aggregate that cannot produce any of them is a broken instrument, and reporting
            // "unclassified" from one would dress the failure up as a result.
            //
            // Second instance of the same shape, after gradient-per-pixel. Aggregates are where it
            // keeps happening: a single number reads as a finding, survives review because it is
            // precise, and is silent about the axis it collapsed.
            //
            // A correct one-pole reads -3.01 dB AT its own cutoff, whatever the sample rate. That
            // is the number the classification uses. The full curves are reported beside it so the
            // far field is visible and separable rather than folded in.
            const std::vector<double> probes { 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0 };

            struct Cell { const char* name; float cutoffHz; double fs; };
            const Cell cells[] = {
                { " 2k @ 44.1k", 2000.0f,  44100.0 }, { " 2k @  192k", 2000.0f,  192000.0 },
                { "16k @ 44.1k", 16000.0f, 44100.0 }, { "16k @  192k", 16000.0f, 192000.0 }
            };

            const auto responseAt = [] (float cutoffHz, double fs, const std::vector<double>& at)
            {
                ReverbPrimitives::OnePoleLP filter;
                filter.setCutoff (cutoffHz, fs);

                return nf::testing::measureMagnitudeResponse (
                    [&filter] (float x) { return filter.process (x); },
                    [&filter] { filter.reset(); },
                    fs, at);
            };

            // --- the four curves, reported in full -------------------------------------------
            std::vector<std::vector<nf::testing::MagnitudeRow>> curves;
            for (const auto& cell : cells)
                curves.push_back (responseAt (cell.cutoffHz, cell.fs, probes));

            juce::String header ("     probe |");
            for (const auto& cell : cells)
                header += juce::String (cell.name).paddedLeft (' ', 13) + " |";
            logMessage ("  " + header);

            for (size_t i = 0; i < probes.size(); ++i)
            {
                juce::String row = (juce::String (probes[i], 0) + " Hz").paddedLeft (' ', 10) + " |";

                for (const auto& curve : curves)
                    row += (juce::String (curve[i].gainDb, 3) + " dB").paddedLeft (' ', 13) + " |";

                logMessage ("  " + row);
            }

            // --- the corners, which are what classify ----------------------------------------
            constexpr double idealCornerDb = -3.01;
            constexpr double cornerToleranceDb = 0.25;

            std::array<bool, 4> cornerHolds {};

            for (size_t c = 0; c < 4; ++c)
            {
                const auto atCorner = responseAt (cells[c].cutoffHz, cells[c].fs,
                                                  { (double) cells[c].cutoffHz }).front().gainDb;
                const auto error = std::abs (atCorner - idealCornerDb);
                cornerHolds[c] = error <= cornerToleranceDb;

                logMessage ("  corner " + juce::String (cells[c].name) + " -> "
                                + juce::String (atCorner, 3) + " dB, "
                                + juce::String (error, 3) + " dB from -3.01  "
                                + (cornerHolds[c] ? "(holds)" : "(WRONG)"));
            }

            // The control: dampLF's corner is nowhere near Nyquist at either rate, so it cannot
            // produce a Nyquist artefact and a move there would mean something else entirely.
            const auto lfHolds = [&]
            {
                bool holds = true;

                for (double fs : { 44100.0, 192000.0 })
                {
                    const auto atCorner = responseAt (500.0f, fs, { 500.0 }).front().gainDb;
                    holds = holds && std::abs (atCorner - idealCornerDb) <= cornerToleranceDb;
                    logMessage ("  control dampLF 500 Hz @ " + juce::String (fs / 1000.0, 1)
                                    + "k -> " + juce::String (atCorner, 3) + " dB");
                }

                return holds;
            }();

            const bool lo441 = cornerHolds[0], lo192 = cornerHolds[1];
            const bool hi441 = cornerHolds[2], hi192 = cornerHolds[3];

            if (lo441 && lo192 && hi441 && hi192 && lfHolds)
                logMessage ("  => ALL FOUR CORNERS HOLD: rate-correct across the whole range");
            else if (! lo441 && ! lo192 && ! hi441 && ! hi192)
                logMessage ("  => BOTH ENDS MOVE AT BOTH RATES: a rate bug");
            else if (lo441 && lo192 && hi192 && ! hi441 && lfHolds)
                logMessage ("  => ONLY THE TOP CORNER AT 44.1k MOVES: a RANGE finding, not a rate "
                            "finding. dampHF's own maximum is reachable at every supported rate, so "
                            "the range is what is wrong rather than the rate handling.");
            else
                logMessage ("  => UNCLASSIFIED — a pattern the four readings do not name. Figures "
                            "reported, nothing assigned, per the rule.");

            expect (lo441 && lo192 && hi441 && hi192 && lfHolds,
                    "a dampHF corner is not where its cutoff says it is — read the four corner "
                    "lines above against this test's table rather than assigning it");
        }

        beginTest ("Offline against real-time");
        {
            Reflect84AudioProcessor processor;
            warm (processor);                    // see the pre-delay finding above

            const auto r = nf::testing::offlineAgainstRealtime (processor, {});

            logMessage ("  " + r.describe());

            expect (r.nonRealtimeWasHonoured,
                    "setNonRealtime(true) did not take effect, so this compared two real-time "
                    "renders: " + r.describe());

            // A difference is not automatically a defect — setNonRealtime is a hint a processor may
            // legitimately act on. Reported; the casting decides.
            if (! r.sampleExact)
                logMessage ("  NOTE: offline differs from real-time. Not a defect on its face — "
                            "this casting would have to intend it.");
        }
    }

private:
    /** One discarded render, so the pre-delay smoother is past its constructed condition. Named
        rather than inlined so every call site points at the finding that makes it necessary. */
    static void warm (Reflect84AudioProcessor& p)
    {
        nf::testing::RenderSpec spec;
        spec.blockSize = 512;
        spec.numBlocks = 4;
        nf::testing::render (p, spec);
    }

    static void set (Reflect84AudioProcessor& p, const char* id, float normalised)
    {
        if (auto* param = p.apvts.getParameter (id))
            param->setValueNotifyingHost (normalised);
    }
};

static InvarianceTests invarianceTests;
