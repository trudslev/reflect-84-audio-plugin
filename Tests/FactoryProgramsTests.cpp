#include "TestUtils.h"

#include "../Source/DSP/FactoryPrograms.h"

#include <juce_core/juce_core.h>

#include <set>

/**
    Structural guards on the factory bank. These do not - and cannot yet - say anything about
    whether the Programs sound good; the tank topologies do not exist. What they do is make sure
    the bank stays internally consistent as it is tuned.
*/
class FactoryProgramsTests final : public juce::UnitTest
{
public:
    FactoryProgramsTests() : juce::UnitTest ("FactoryPrograms", "programs") {}

    void runTest() override
    {
        beginTest ("the bank is the twelve settled names, in order");
        {
            expectEquals (kNumFactoryPrograms, 12);

            const juce::StringArray expected {
                "RAIN ALL DAY", "SO LONG", "QUIET VIOLENCE", "SAIL AWAY",
                "ON THE MOON", "A PRAYER", "BROTHERS", "HEAVEN",
                "COLD ATMOSPHERE", "THE MOORS", "SECOND NATURE", "WORLD GONE MAD"
            };

            for (int i = 0; i < kNumFactoryPrograms; ++i)
                expectEquals (juce::String (kFactoryPrograms[(size_t) i].name), expected[i]);
        }

        beginTest ("names are unique and fit the LCD");
        {
            std::set<juce::String> seen;

            for (const auto& program : kFactoryPrograms)
            {
                const juce::String name { program.name };

                expect (name.isNotEmpty());
                expect (name.length() <= ProgramManagerNameLimit);
                expect (name == name.toUpperCase(), "names are upper case, matching the LCD: " + name);
                expect (seen.insert (name).second, "duplicate program name: " + name);
            }
        }

        beginTest ("every normalised field is inside 0-1 and the algorithm is a real index");
        {
            for (const auto& p : kFactoryPrograms)
            {
                const juce::String name { p.name };

                expect (p.algorithm >= 0 && p.algorithm < numAlgorithms, name + ": algorithm index");

                for (const auto& [label, value] : {
                         std::pair { "size",       p.size },
                         std::pair { "decay",      p.decay },
                         std::pair { "preDelay",   p.preDelay },
                         std::pair { "density",    p.density },
                         std::pair { "dampHF",     p.dampHF },
                         std::pair { "dampLF",     p.dampLF },
                         std::pair { "modulation", p.modulation },
                         std::pair { "grain",      p.grain },
                         std::pair { "width",      p.width },
                         std::pair { "mix",        p.mix },
                         std::pair { "trim",       p.trim } })
                {
                    expect (value >= 0.0f && value <= 1.0f,
                            name + ": " + label + " is outside 0-1");
                }
            }
        }

        beginTest ("the struct covers every automatable parameter");
        {
            // If a parameter is added to Parameters.h but not to FactoryProgram, loading a factory
            // Program leaves it at whatever the previous Program set - silently, and differently
            // depending on which Program you came from. This is the guard against that.
            TestHostProcessor host;

            constexpr int fieldsInFactoryProgram = 12;   // 11 normalised floats + algorithm
            expectEquals (host.getParameters().size(), fieldsInFactoryProgram);
        }

        beginTest ("no Program is silent, and the default is the one the artwork shows");
        {
            expectEquals (defaultFactoryProgramIndex, 0);
            expectEquals (juce::String (kFactoryPrograms[(size_t) defaultFactoryProgramIndex].name),
                          juce::String ("RAIN ALL DAY"));

            for (const auto& p : kFactoryPrograms)
            {
                // A reverb Program with no wet signal is indistinguishable from a broken one.
                expect (p.mix > 0.05f, juce::String (p.name) + ": mix is effectively dry");
                expect (p.decay > 0.0f, juce::String (p.name) + ": no tail at all");
            }
        }

        beginTest ("the bank is twelve distinct starting points, not twelve copies");
        {
            std::set<int> algorithmsUsed;

            for (const auto& p : kFactoryPrograms)
                algorithmsUsed.insert (p.algorithm);

            expect (algorithmsUsed.size() >= 3,
                    "the bank should exercise most of the algorithm set");

            for (size_t i = 0; i < kFactoryPrograms.size(); ++i)
                for (size_t j = i + 1; j < kFactoryPrograms.size(); ++j)
                    expect (! identical (kFactoryPrograms[i], kFactoryPrograms[j]),
                            juce::String (kFactoryPrograms[i].name) + " and "
                                + kFactoryPrograms[j].name + " are the same Program");
        }
    }

private:
    static constexpr int ProgramManagerNameLimit = 22;

    static bool identical (const FactoryProgram& a, const FactoryProgram& b)
    {
        const auto same = [] (float x, float y) { return std::abs (x - y) < 1.0e-6f; };

        return a.algorithm == b.algorithm
            && same (a.size, b.size) && same (a.decay, b.decay)
            && same (a.preDelay, b.preDelay) && same (a.density, b.density)
            && same (a.dampHF, b.dampHF) && same (a.dampLF, b.dampLF)
            && same (a.modulation, b.modulation) && same (a.grain, b.grain)
            && same (a.width, b.width) && same (a.mix, b.mix) && same (a.trim, b.trim);
    }
};

static FactoryProgramsTests factoryProgramsTests;
