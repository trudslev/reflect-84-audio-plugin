#include "../Source/DSP/ReverbEngine.h"

#include <juce_core/juce_core.h>

#include <chrono>

namespace
{
    constexpr double testSampleRate = 48000.0;
    constexpr int testBlockSize = 256;
    constexpr int numAlgorithms = 4;

    const char* algorithmName (int i)
    {
        switch (i)
        {
            case 0: return "Plate";
            case 1: return "Digital Room";
            case 2: return "Chamber";
            default: return "Hall";
        }
    }

    TankParameters defaultParams()
    {
        TankParameters p;
        p.sizeScale = 0.7f;
        p.decaySeconds = 2.0f;
        p.density01 = 0.7f;
        p.dampHFHz = 9000.0f;
        p.dampLFHz = 120.0f;
        p.mod01 = 0.3f;
        p.grain01 = 0.0f;
        return p;
    }

    ReverbEngine makeEngine()
    {
        ReverbEngine engine;
        engine.prepare ({ testSampleRate, (juce::uint32) testBlockSize, 2 }, 0.0f, 0);
        return engine;
    }

    /** Feeds one impulse and captures the wet envelope, block-peak per block. */
    std::vector<float> impulseEnvelope (ReverbEngine& engine, int algorithm,
                                        const TankParameters& p, double seconds,
                                        float preDelayMs = 0.0f)
    {
        juce::AudioBuffer<float> buffer (2, testBlockSize);
        std::vector<float> envelope;

        const int totalBlocks = (int) std::ceil (seconds * testSampleRate / testBlockSize);
        envelope.reserve ((size_t) totalBlocks);

        for (int block = 0; block < totalBlocks; ++block)
        {
            buffer.clear();

            if (block == 0)
                for (int ch = 0; ch < 2; ++ch)
                    buffer.setSample (ch, 0, 1.0f);

            engine.process (buffer, algorithm, p, preDelayMs);

            float peak = 0.0f;

            for (int ch = 0; ch < 2; ++ch)
                for (int n = 0; n < testBlockSize; ++n)
                    peak = juce::jmax (peak, std::abs (buffer.getSample (ch, n)));

            envelope.push_back (peak);
        }

        return envelope;
    }

    /** Seconds from the envelope's peak until it falls 60 dB below it, or -1 if it never does. */
    float measureRT60 (const std::vector<float>& envelope)
    {
        const auto peakIt = std::max_element (envelope.begin(), envelope.end());

        if (peakIt == envelope.end() || *peakIt <= 0.0f)
            return -1.0f;

        const float threshold = *peakIt * 0.001f;   // -60 dB

        for (auto it = peakIt; it != envelope.end(); ++it)
            if (*it < threshold)
                return (float) std::distance (envelope.begin(), it)
                     * (float) testBlockSize / (float) testSampleRate;

        return -1.0f;
    }
}

//==============================================================================
/**
    The DSP contract. The load-bearing case is "four algorithms are measurably distinct" - the
    single thing this plugin could most plausibly get wrong is shipping one tank wearing four
    names, and that is not something a listener would necessarily catch quickly.
*/
class ReverbEngineTests final : public juce::UnitTest
{
public:
    ReverbEngineTests() : juce::UnitTest ("ReverbEngine", "dsp") {}

    void runTest() override
    {
        beginTest ("every algorithm produces a decaying tail from an impulse");
        {
            for (int algo = 0; algo < numAlgorithms; ++algo)
            {
                auto engine = makeEngine();
                const auto env = impulseEnvelope (engine, algo, defaultParams(), 4.0);

                const float peak = *std::max_element (env.begin(), env.end());
                expect (peak > 1.0e-4f, juce::String (algorithmName (algo)) + ": produced no output");

                const float tail = env.back();
                expect (tail < peak * 0.1f,
                        juce::String (algorithmName (algo)) + ": still at "
                            + juce::String (tail / peak, 3) + " of peak after 4 s");
            }
        }

        beginTest ("RT60 tracks the DECAY parameter");
        {
            for (int algo = 0; algo < numAlgorithms; ++algo)
            {
                float previous = 0.0f;

                for (const float requested : { 0.8f, 2.0f, 5.0f })
                {
                    auto engine = makeEngine();
                    auto p = defaultParams();
                    p.decaySeconds = requested;

                    const float measured = measureRT60 (impulseEnvelope (engine, algo, p, requested * 4.0 + 2.0));

                    expect (measured > 0.0f,
                            juce::String (algorithmName (algo)) + ": tail never reached -60 dB at "
                                + juce::String (requested) + "s");

                    // A generous band: RT60 from a block-peak envelope on a modulated network is
                    // an estimate, and the point here is that the control does what it says, not
                    // that the number is laboratory-accurate.
                    expect (measured > requested * 0.35f && measured < requested * 2.5f,
                            juce::String (algorithmName (algo)) + ": asked for "
                                + juce::String (requested) + "s, measured " + juce::String (measured, 2) + "s");

                    expect (measured > previous,
                            juce::String (algorithmName (algo)) + ": RT60 did not increase with DECAY");
                    previous = measured;
                }
            }
        }

        beginTest ("nothing diverges at maximum settings");
        {
            for (int algo = 0; algo < numAlgorithms; ++algo)
            {
                auto engine = makeEngine();

                TankParameters p;
                p.sizeScale = 1.0f;
                p.decaySeconds = 8.0f;
                p.density01 = 1.0f;
                p.dampHFHz = 16000.0f;
                p.dampLFHz = 40.0f;
                p.mod01 = 1.0f;
                p.grain01 = 1.0f;

                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::Random random { 1234 };

                float worstPeak = 0.0f;

                // 20 seconds of full-scale noise into the longest, densest, grainiest setting.
                for (int block = 0; block < (int) (20.0 * testSampleRate / testBlockSize); ++block)
                {
                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < testBlockSize; ++n)
                            buffer.setSample (ch, n, random.nextFloat() * 2.0f - 1.0f);

                    engine.process (buffer, algo, p, ReverbEngine::maxPreDelayMs);

                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < testBlockSize; ++n)
                        {
                            const float v = buffer.getSample (ch, n);
                            expect (std::isfinite (v),
                                    juce::String (algorithmName (algo)) + ": produced a non-finite sample");
                            worstPeak = juce::jmax (worstPeak, std::abs (v));
                        }
                }

                expect (worstPeak < 8.0f,
                        juce::String (algorithmName (algo)) + ": ran away to "
                            + juce::String (worstPeak, 2) + " full scale");
            }
        }

        beginTest ("the four algorithms are measurably distinct, not one tank renamed");
        {
            // This is the guard the whole design rests on. Three independent measures, because any
            // one of them could coincide between two genuinely different networks.
            std::array<Fingerprint, numAlgorithms> prints {};

            for (int algo = 0; algo < numAlgorithms; ++algo)
                prints[(size_t) algo] = fingerprint (algo);

            for (int a = 0; a < numAlgorithms; ++a)
            {
                for (int b = a + 1; b < numAlgorithms; ++b)
                {
                    const auto& x = prints[(size_t) a];
                    const auto& y = prints[(size_t) b];

                    const bool densityDiffers  = relativeDifference (x.earlyDensity, y.earlyDensity) > 0.15f;
                    const bool buildDiffers    = relativeDifference (x.buildTime, y.buildTime) > 0.15f;
                    const bool centroidDiffers = relativeDifference (x.centroid, y.centroid) > 0.15f;

                    expect (densityDiffers || buildDiffers || centroidDiffers,
                            juce::String (algorithmName (a)) + " and " + algorithmName (b)
                                + " are indistinguishable: density "
                                + juce::String (x.earlyDensity, 3) + "/" + juce::String (y.earlyDensity, 3)
                                + ", build " + juce::String (x.buildTime, 4) + "/" + juce::String (y.buildTime, 4)
                                + ", centroid " + juce::String (x.centroid, 0) + "/" + juce::String (y.centroid, 0));
                }
            }
        }

        beginTest ("Plate builds faster and denser than Hall");
        {
            // The one qualitative relationship worth asserting rather than just measuring: a plate
            // has no early reflections and is dense immediately; a hall has a deliberate gap.
            const auto plate = fingerprint (0);
            const auto hall = fingerprint (3);

            expect (plate.earlyDensity > hall.earlyDensity,
                    "Plate early density " + juce::String (plate.earlyDensity, 3)
                        + " should exceed Hall's " + juce::String (hall.earlyDensity, 3));
        }

        beginTest ("switching algorithm mid-stream does not produce a discontinuity");
        {
            auto engine = makeEngine();
            const auto p = defaultParams();

            juce::AudioBuffer<float> buffer (2, testBlockSize);
            juce::Random random { 99 };

            float previousSample = 0.0f;
            float worstJump = 0.0f;

            for (int block = 0; block < 400; ++block)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < testBlockSize; ++n)
                        buffer.setSample (ch, n, (random.nextFloat() * 2.0f - 1.0f) * 0.25f);

                // Switch every 50 blocks, cycling through all four.
                engine.process (buffer, (block / 50) % numAlgorithms, p, 10.0f);

                for (int n = 0; n < testBlockSize; ++n)
                {
                    const float s = buffer.getSample (0, n);
                    worstJump = juce::jmax (worstJump, std::abs (s - previousSample));
                    previousSample = s;
                }
            }

            // The input itself is noise, so sample-to-sample motion is expected; what a bad switch
            // looks like is a step far larger than anything the signal can produce on its own.
            expect (worstJump < 1.5f,
                    "worst sample-to-sample jump across switches was " + juce::String (worstJump, 3));
        }

        beginTest ("CPU stays inside the real-time budget");
        {
            // 256 samples at 48 kHz is a 5.33 ms budget. Anything approaching that on one core is
            // unusable in a session with other plugins in it.
            constexpr double budgetMs = 1000.0 * testBlockSize / testSampleRate;

            for (int algo = 0; algo < numAlgorithms; ++algo)
            {
                auto engine = makeEngine();
                auto p = defaultParams();
                p.grain01 = 0.5f;

                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::Random random { 7 };

                constexpr int blocks = 400;
                const auto start = std::chrono::steady_clock::now();

                for (int block = 0; block < blocks; ++block)
                {
                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < testBlockSize; ++n)
                            buffer.setSample (ch, n, random.nextFloat() * 0.5f - 0.25f);

                    engine.process (buffer, algo, p, 20.0f);
                }

                const auto elapsed = std::chrono::steady_clock::now() - start;
                const double perBlockMs =
                    std::chrono::duration<double, std::milli> (elapsed).count() / blocks;

                logMessage (juce::String (algorithmName (algo)) + ": "
                                + juce::String (perBlockMs, 3) + " ms/block ("
                                + juce::String (100.0 * perBlockMs / budgetMs, 1) + "% of budget)");

                expect (perBlockMs < budgetMs * 0.5,
                        juce::String (algorithmName (algo)) + " used "
                            + juce::String (perBlockMs, 3) + " ms of a " + juce::String (budgetMs, 2) + " ms budget");
            }
        }

        /*  **QUIET VIOLENCE — reported from outside as "Chamber goes into feedback no matter
            what I do".** Driven at that Program's own stored values rather than at anything
            convenient, because the report named it and a named case is the cheapest known case
            there is.

            The arms are a sweep, not one run: the Program, then each of its distinguishing values
            moved back toward the defaults one at a time. A tail that grows is a tail that grows
            whatever the cause, so the arm that reports it is `peak after` against `peak during`.  */
        beginTest ("QUIET VIOLENCE — the reported Chamber case, driven at its own values");
        {
            /*  The stored 0-1 values put through `Parameters.h`'s own laws. Written out rather
                than included, because including that header makes `numAlgorithms` ambiguous
                against the one already in scope here — and the four conversions are one line
                each:
                    sizeScale     0.2 + v * 0.8                 0.5625 -> 0.65
                    decaySeconds  0.4 + v * 7.6                 0.3684 -> 3.20 s
                    dampHFHz      2000 * 8   ^ v                0.4406 -> 5000 Hz
                    dampLFHz      40   * 12.5 ^ v               0.7978 ->  300 Hz
                which is exactly what `FactoryPrograms.h` annotates the row with. */
            TankParameters p;
            p.sizeScale    = 0.65f;
            p.decaySeconds = 3.20f;
            p.density01    = 0.8000f;
            p.dampHFHz     = 5000.0f;
            p.dampLFHz     = 300.0f;
            p.mod01        = 0.1500f;
            p.grain01      = 0.3500f;

            logMessage ("  size " + juce::String (p.sizeScale, 3)
                        + "  decay " + juce::String (p.decaySeconds, 2) + " s"
                        + "  density " + juce::String (p.density01, 2)
                        + "  dampHF " + juce::String (juce::roundToInt (p.dampHFHz)) + " Hz"
                        + "  dampLF " + juce::String (juce::roundToInt (p.dampLFHz)) + " Hz"
                        + "  mod " + juce::String (p.mod01, 2)
                        + "  grain " + juce::String (p.grain01, 2));

            const auto run = [this] (int algo, TankParameters params, const char* label)
            {
                auto engine = makeEngine();
                juce::AudioBuffer<float> buffer (2, testBlockSize);
                float during = 0.0f, after = 0.0f;

                // 0.5 s of impulse-fed tail, then 12 s of silence: an RT60 of 3.2 s should be
                // 60 dB down well inside that, so anything still growing is not a tail.
                const int fed  = (int) (0.5 * testSampleRate / testBlockSize);
                const int rest = (int) (12.0 * testSampleRate / testBlockSize);

                for (int b = 0; b < fed; ++b)
                {
                    buffer.clear();
                    if (b == 0) for (int ch = 0; ch < 2; ++ch) buffer.setSample (ch, 0, 1.0f);
                    engine.process (buffer, algo, params, 0.0f);
                    during = juce::jmax (during, buffer.getMagnitude (0, testBlockSize));
                }
                for (int b = 0; b < rest; ++b)
                {
                    buffer.clear();
                    engine.process (buffer, algo, params, 0.0f);
                    after = juce::jmax (after, buffer.getMagnitude (0, testBlockSize));
                }
                logMessage (juce::String ("  ") + label
                            + "  during " + juce::String (during, 6)
                            + "  after "  + juce::String (after, 6)
                            + "  ratio "  + juce::String (during > 0.0f ? after / during : 0.0f, 4));
                return during > 0.0f ? after / during : 0.0f;
            };

            const float chamber = run (2, p, "CHAMBER, as stored     ");

            // Each distinguishing value moved back alone, to localise by parameter rather than
            // by construction — a stage bisection partitions, a hypothesis only ever refutes.
            auto q = p; q.density01 = 0.7f;      run (2, q, "  density 0.80 -> 0.70 ");
            q = p; q.dampLFHz = 120.0f;          run (2, q, "  dampLF  -> 120 Hz    ");
            q = p; q.dampHFHz = 9000.0f;         run (2, q, "  dampHF  -> 9000 Hz   ");
            q = p; q.grain01 = 0.0f;             run (2, q, "  grain   -> 0         ");
            q = p; q.mod01 = 0.3f;               run (2, q, "  mod     -> 0.30      ");
            q = p; q.sizeScale = 0.7f;           run (2, q, "  size    -> 0.70      ");
            q = p; q.decaySeconds = 2.0f;        run (2, q, "  decay   -> 2.0 s     ");

            // Controls: the same values on the other three networks.
            run (0, p, "PLATE, same values     ");
            run (1, p, "DIGITAL ROOM, same     ");
            run (3, p, "HALL, same values      ");

            // **Pre-stated**: if GRAIN is the source — a sample-and-hold quantiser closing each
            // line's loop — then at dampLF 120 grain 0 is stable and grain > 0 diverges. If it
            // diverges either way, grain is exonerated and the loop gain itself is the subject.
            logMessage ("  --- grain at dampLF 120 Hz, CHAMBER ---");
            float worstGrain = 0.0f, worstGrainAt = 0.0f;
            for (float g : { 0.0f, 0.05f, 0.15f, 0.35f, 0.60f, 1.0f })
            {
                auto v = p; v.dampLFHz = 120.0f; v.grain01 = g;
                const float r = run (2, v, juce::String ("  grain " + juce::String (g, 2)
                                                         + "        ").toRawUTF8());
                if (r > worstGrain) { worstGrain = r; worstGrainAt = g; }
            }

            // Where is the edge? dampLF sweeps 40..500 Hz across its whole range, on all four.
            logMessage ("  --- dampLF sweep, ratio after/during ---");
            float worstSweep = 0.0f, worstSweepHz = 0.0f;
            int   worstSweepAlgo = -1;
            for (int algo = 0; algo < 4; ++algo)
            {
                juce::String row ("  algo " + juce::String (algo) + " ");
                for (float hz : { 40.0f, 60.0f, 90.0f, 120.0f, 160.0f, 200.0f, 250.0f, 300.0f, 400.0f, 500.0f })
                {
                    auto v = p; v.dampLFHz = hz;
                    auto engine = makeEngine();
                    juce::AudioBuffer<float> buffer (2, testBlockSize);
                    float during = 0.0f, after = 0.0f;
                    for (int b = 0; b < (int) (0.5 * testSampleRate / testBlockSize); ++b)
                    {
                        buffer.clear();
                        if (b == 0) for (int ch = 0; ch < 2; ++ch) buffer.setSample (ch, 0, 1.0f);
                        engine.process (buffer, algo, v, 0.0f);
                        during = juce::jmax (during, buffer.getMagnitude (0, testBlockSize));
                    }
                    for (int b = 0; b < (int) (8.0 * testSampleRate / testBlockSize); ++b)
                    {
                        buffer.clear();
                        engine.process (buffer, algo, v, 0.0f);
                        after = juce::jmax (after, buffer.getMagnitude (0, testBlockSize));
                    }
                    const float r = during > 0.0f ? after / during : 0.0f;
                    if (r > worstSweep) { worstSweep = r; worstSweepAlgo = algo; worstSweepHz = hz; }
                    row << juce::String (juce::roundToInt (hz)) << ":"
                        << (r > 1.0f ? juce::String ("DIVERGES") : juce::String (r, 3)) << "  ";
                }
                logMessage (row);
            }

            /*  **CONTINUOUSLY DRIVEN, at the Program's own values and nothing else's.** The
                impulse arm above is how a tail is normally measured and it is the wrong instrument
                for this report: it excites the network once and then asks whether the ring-down
                decays, where the reported symptom is a plugin with music going through it. A
                collapsed-rank feedback network that decays from one impulse can still accumulate
                under continuous excitation, so the two arms are asking different questions. */
            {
                auto engine = makeEngine();
                juce::AudioBuffer<float> buffer (2, testBlockSize);
                juce::Random rng (20260823);
                float early = 0.0f, late = 0.0f;

                const int blocks = (int) (20.0 * testSampleRate / testBlockSize);
                for (int b = 0; b < blocks; ++b)
                {
                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < testBlockSize; ++i)
                            buffer.setSample (ch, i, rng.nextFloat() * 0.5f - 0.25f);

                    engine.process (buffer, 2, p, 0.0f);
                    const float m = buffer.getMagnitude (0, testBlockSize);

                    if (b < blocks / 10) early = juce::jmax (early, m);
                    if (b > blocks - blocks / 10) late = juce::jmax (late, m);
                }

                logMessage ("  CHAMBER, 20 s of noise    early " + juce::String (early, 6)
                            + "  late " + juce::String (late, 6)
                            + "  ratio " + juce::String (early > 0.0f ? late / early : 0.0f, 4));

                expect (late < early * 4.0f,
                        "CHAMBER at QUIET VIOLENCE's values grows under continuous input — "
                        "last tenth peaks at " + juce::String (late, 6)
                        + " against " + juce::String (early, 6) + " in the first");
            }

            /*  **Three assertions, because the Program was the REPORT and not the subject.**
                QUIET VIOLENCE's own values sat just inside the stable side of the edge — it was
                the *default* 120 Hz that diverged — so an arm pinning only the reported Program
                would have gone green on a plugin that still ran away everywhere else. */

            expect (chamber < 0.5f,
                    "CHAMBER at QUIET VIOLENCE's values is louder after 12 s of silence than "
                    "it ever was while being fed — ratio "
                    + juce::String (chamber, 4) + ", which is a network that is not decaying");

            /*  The mechanism, pinned at the value that showed it. Before the fix this read
                0.00 / 0.00 / DIVERGES / DIVERGES / DIVERGES / 0.00 across the six grain settings:
                `GrainStage` held two slots while `FdnTank` drove it with a LINE index, so during a
                hold lines 0 and 2 returned one value and 1 and 3 another, and the FDN's four
                independent states collapsed onto two identical pairs that the orthogonal mixing
                matrix then summed coherently. Both stable ends are the two cases where no sharing
                is possible — a passthrough, and holdPeriod 1. */
            expect (worstGrain < 0.5f,
                    "CHAMBER runs away at GRAIN " + juce::String (worstGrainAt, 2)
                    + " with dampLF at its default 120 Hz — ratio "
                    + juce::String (worstGrain, 4));

            /*  And the whole low-cut range on all four, because "no matter what I do" is a claim
                about the range rather than about a point. Chamber failed 40..200 Hz inclusive and
                Hall failed at 40. */
            expect (worstSweep < 0.5f,
                    "algorithm " + juce::String (worstSweepAlgo) + " runs away at dampLF "
                    + juce::String (juce::roundToInt (worstSweepHz)) + " Hz — ratio "
                    + juce::String (worstSweep, 4));
        }

        beginTest ("the tank reports energy for the lamp, and it decays");
        {
            auto engine = makeEngine();
            auto p = defaultParams();
            p.decaySeconds = 3.0f;

            juce::AudioBuffer<float> buffer (2, testBlockSize);

            // The tail has to be given time to arrive before asking about it. One 256-sample
            // block at 48 kHz is 5.3 ms, and the Plate's earliest output tap is around 9 ms, so
            // checking immediately after the impulse measures nothing but the fill delay.
            float peakEnergy = 0.0f;

            for (int block = 0; block < 100; ++block)      // ~0.5 s
            {
                buffer.clear();

                if (block == 0)
                    for (int ch = 0; ch < 2; ++ch)
                        buffer.setSample (ch, 0, 1.0f);

                engine.process (buffer, 0, p, 0.0f);
                peakEnergy = juce::jmax (peakEnergy, engine.getEnergy());
            }

            expect (peakEnergy > 0.0f, "no energy reported while the tail was audible");

            for (int block = 0; block < 1200; ++block)     // ~6.4 s, twice the RT60
            {
                buffer.clear();
                engine.process (buffer, 0, p, 0.0f);
            }

            expect (engine.getEnergy() < peakEnergy * 0.25f,
                    "energy was still at " + juce::String (engine.getEnergy() / peakEnergy, 3)
                        + " of peak long after the tail should have gone");
        }
    }

private:
    static float relativeDifference (float a, float b)
    {
        const float scale = juce::jmax (1.0e-6f, (std::abs (a) + std::abs (b)) * 0.5f);
        return std::abs (a - b) / scale;
    }

    struct Fingerprint
    {
        float earlyDensity;
        float buildTime;
        float centroid;
    };

    static Fingerprint fingerprint (int algorithm)
    {
        auto engine = makeEngine();
        auto p = defaultParams();
        p.decaySeconds = 2.5f;

        const int totalSamples = (int) testSampleRate;   // one second
        std::vector<float> tail;
        tail.reserve ((size_t) totalSamples);

        juce::AudioBuffer<float> buffer (2, testBlockSize);

        for (int block = 0; block * testBlockSize < totalSamples; ++block)
        {
            buffer.clear();

            if (block == 0)
                for (int ch = 0; ch < 2; ++ch)
                    buffer.setSample (ch, 0, 1.0f);

            engine.process (buffer, algorithm, p, 0.0f);

            for (int n = 0; n < testBlockSize; ++n)
                tail.push_back (buffer.getSample (0, n));
        }

        const float peak = *std::max_element (tail.begin(), tail.end(),
                                              [] (float a, float b) { return std::abs (a) < std::abs (b); });
        const float absPeak = juce::jmax (1.0e-9f, std::abs (peak));

        Fingerprint fp {};

        // Echo density in the first 100 ms: what fraction of samples carry meaningful energy. A
        // plate is near-continuous immediately; a hall is a handful of discrete arrivals.
        const int earlyCount = juce::jmin ((int) tail.size(), (int) (0.1 * testSampleRate));
        int active = 0;

        for (int i = 0; i < earlyCount; ++i)
            if (std::abs (tail[(size_t) i]) > absPeak * 0.02f)
                ++active;

        fp.earlyDensity = (float) active / (float) juce::jmax (1, earlyCount);

        // Build time: how long until the tail reaches its loudest point.
        int peakIndex = 0;

        for (size_t i = 0; i < tail.size(); ++i)
            if (std::abs (tail[i]) >= absPeak * 0.999f)
            {
                peakIndex = (int) i;
                break;
            }

        fp.buildTime = (float) peakIndex / (float) testSampleRate;

        // Spectral centroid via zero-crossing rate - a cheap proxy, but a monotonic one, and it
        // separates a bright plate from a dark hall without needing an FFT.
        int crossings = 0;

        for (size_t i = 1; i < tail.size(); ++i)
            if ((tail[i] >= 0.0f) != (tail[i - 1] >= 0.0f))
                ++crossings;

        fp.centroid = (float) crossings * 0.5f / ((float) tail.size() / (float) testSampleRate);

        return fp;
    }
};

static ReverbEngineTests reverbEngineTests;
