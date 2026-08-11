#include "TestUtils.h"

#include "../Source/DSP/ProgramManager.h"

#include <juce_core/juce_core.h>

/**
    The Program contract, in tests: Save always creates new, Delete never touches a factory
    Program, and Cancel never touches the APVTS.

    Every case runs against a temporary directory, so nothing here can write into - or delete
    from - the user's real Programs folder.
*/
class ProgramManagerTests final : public juce::UnitTest
{
public:
    ProgramManagerTests() : juce::UnitTest ("ProgramManager", "programs") {}

    void runTest() override
    {
        beginTest ("an empty user bank is just the factory bank");
        {
            ScopedTestDirectory dir { "empty" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            // getNumPrograms is the HOST's list and is the Factory bank, always - it does not
            // grow with user files any more, which is the juce_AudioProcessor.h contract.
            expectEquals (manager.getNumPrograms(), kNumFactoryPrograms);
            expectEquals (manager.getProgramName (0), juce::String ("RAIN ALL DAY"));
            expect (userPrograms (manager).empty());

            // INIT + the factory bank, and nothing else.
            expectEquals ((int) manager.listPrograms().size(), kNumFactoryPrograms + 1);
            expect (manager.listPrograms().front().bank == ProgramBank::init);
        }

        beginTest ("loading a factory Program writes every parameter");
        {
            ScopedTestDirectory dir { "apply" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            // Push everything somewhere it certainly is not, then load.
            for (auto* p : host.getParameters())
                p->setValueNotifyingHost (0.123f);

            manager.initialise();

            const auto& program = kFactoryPrograms[(size_t) defaultFactoryProgramIndex];

            expectWithinAbsoluteError (valueOf (host, ParamIDs::size), program.size, 1.0e-4f);
            expectWithinAbsoluteError (valueOf (host, ParamIDs::grain), program.grain, 1.0e-4f);
            expectWithinAbsoluteError (valueOf (host, ParamIDs::trim), program.trim, 1.0e-4f);

            if (auto* algo = dynamic_cast<juce::AudioParameterChoice*> (host.apvts.getParameter (ParamIDs::algorithm)))
                expectEquals (algo->getIndex(), program.algorithm);
            else
                expect (false, "algorithm is not a choice parameter");
        }

        beginTest ("SAVE is gated on something actually having moved");
        {
            ScopedTestDirectory dir { "modified" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            expect (! manager.isModifiedFromLoadedProgram(), "a freshly loaded Program is clean");

            setValueOf (host, ParamIDs::mix, 0.9f);
            expect (manager.isModifiedFromLoadedProgram());

            manager.initialise();
            expect (! manager.isModifiedFromLoadedProgram(), "reloading restores the clean baseline");
        }

        beginTest ("Save creates a new Program and makes it current");
        {
            ScopedTestDirectory dir { "save" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            setValueOf (host, ParamIDs::decay, 0.77f);

            manager.saveNewUserProgram ("my program");

            // The host list must NOT have grown - that is the conformance point.
            expectEquals (manager.getNumPrograms(), kNumFactoryPrograms);

            const auto users = userPrograms (manager);
            expectEquals ((int) users.size(), 1);
            expect (manager.getCurrentProgramId() == users.front());
            expectEquals (users.front().displayName, juce::String ("MY PROGRAM"));

            // A User Program carries no number - it sorts alphabetically, so one would change
            // whenever another was saved.
            expectEquals (manager.displayLabelFor (users.front()), juce::String ("MY PROGRAM"));

            // The just-saved Program is the new clean baseline, so SAVE goes straight back to
            // disabled.
            expect (! manager.isModifiedFromLoadedProgram());
        }

        beginTest ("Save with an existing name creates a DISTINCT Program, never an overwrite");
        {
            ScopedTestDirectory dir { "collision" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();

            setValueOf (host, ParamIDs::decay, 0.20f);
            manager.saveNewUserProgram ("SAME NAME");
            const auto firstId = manager.getCurrentProgramId();

            setValueOf (host, ParamIDs::decay, 0.80f);
            manager.saveNewUserProgram ("SAME NAME");
            const auto secondId = manager.getCurrentProgramId();

            expectEquals ((int) userPrograms (manager).size(), 2);

            // Compared by IDENTITY, which is now the only thing they could be compared by - and
            // which is the point: the list re-sorts after every save, so a position proves nothing.
            expect (firstId != secondId, "the second save reused the first Program's identity");

            // The original Program's values survived untouched - that is the actual guarantee.
            bool foundOriginal = false;

            for (const auto& id : userPrograms (manager))
            {
                if (id != firstId)
                    continue;

                manager.requestProgramChange (id);
                manager.flushPendingChange();
                expectWithinAbsoluteError (valueOf (host, ParamIDs::decay), 0.20f, 1.0e-3f);
                foundOriginal = true;
            }

            expect (foundOriginal, "the first Program is gone - it was overwritten");
        }

        beginTest ("Delete no-ops on anything that is not a User Program");
        {
            ScopedTestDirectory dir { "deleteFactory" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            // Every Factory Program, plus INIT. The gate is on the BANK now, which is stronger than
            // the old index range: an id from any other bank simply cannot address a file.
            for (const auto& id : manager.listPrograms())
                manager.deleteUserProgram (id);

            expectEquals ((int) manager.listPrograms().size(), kNumFactoryPrograms + 1);
        }

        beginTest ("Delete removes a user Program and falls back if it was loaded");
        {
            ScopedTestDirectory dir { "deleteUser" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            setValueOf (host, ParamIDs::width, 0.9f);
            manager.saveNewUserProgram ("DOOMED");

            const auto doomed = manager.getCurrentProgramId();
            expect (doomed.bank == ProgramBank::user);

            manager.deleteUserProgram (doomed);
            manager.flushPendingChange();

            expect (userPrograms (manager).empty());

            // Deleting from the panel is unambiguous intent, so it falls back to the default
            // Program rather than to the unresolved state - that is reserved for a session naming
            // something that is gone.
            expect (manager.getCurrentProgramId()
                        == ProgramManager::factoryIdAt (defaultFactoryProgramIndex));
        }

        beginTest ("a user Program round-trips its values through disk");
        {
            ScopedTestDirectory dir { "roundtrip" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            setValueOf (host, ParamIDs::size, 0.31f);
            setValueOf (host, ParamIDs::grain, 0.66f);
            manager.saveNewUserProgram ("ROUND TRIP");

            const auto saved = manager.getCurrentProgramId();

            manager.requestProgramChange (ProgramManager::factoryIdAt (defaultFactoryProgramIndex));
            manager.flushPendingChange();

            manager.requestProgramChange (saved);
            manager.flushPendingChange();

            expectWithinAbsoluteError (valueOf (host, ParamIDs::size), 0.31f, 1.0e-3f);
            expectWithinAbsoluteError (valueOf (host, ParamIDs::grain), 0.66f, 1.0e-3f);
        }

        beginTest ("a cancelled pending change never lands");
        {
            ScopedTestDirectory dir { "cancel" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            setValueOf (host, ParamIDs::mix, 0.42f);

            // This is the setStateInformation sequence: a request arrives, then the restore
            // cancels it before replacing the state.
            manager.requestProgramChange (ProgramManager::factoryIdAt (5));
            manager.cancelPendingChange();
            manager.flushPendingChange();

            expectWithinAbsoluteError (valueOf (host, ParamIDs::mix), 0.42f, 1.0e-4f);
            expect (manager.getCurrentProgramId()
                        == ProgramManager::factoryIdAt (defaultFactoryProgramIndex));
        }

        beginTest ("An unresolvable identifier is ignored, not clamped into something else");
        {
            ScopedTestDirectory dir { "bounds" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            const auto before = manager.getCurrentProgramId();

            // **There is no out-of-range any more** - that was the whole point. What replaces it is
            // an identifier naming nothing, and the answer must be to leave the current Program
            // alone rather than land on whatever now occupies some position.
            for (const auto& bogus : { ProgramId { ProgramBank::factory, "no-such-slug", "X" },
                                       ProgramId { ProgramBank::user, "no-such-file", "X" },
                                       ProgramId { ProgramBank::init, "not-init", "X" } })
            {
                manager.requestProgramChange (bogus);
                manager.flushPendingChange();
                expect (manager.getCurrentProgramId() == before,
                        "an unresolvable id must not move the current Program");
            }
        }

        beginTest ("INIT is its own bank, reachable, and never the default");
        {
            ScopedTestDirectory dir { "init" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            expect (manager.getCurrentProgramId()
                        == ProgramManager::factoryIdAt (defaultFactoryProgramIndex),
                    "INIT must never be the instantiation default");

            manager.requestProgramChange (ProgramManager::initId());
            manager.flushPendingChange();
            expect (manager.getCurrentProgramId() == ProgramManager::initId());

            // INIT is in neither bank, so it carries no number and reports position 0 to the host.
            expectEquals (manager.displayLabelFor (ProgramManager::initId()), juce::String ("INIT"));
            expectEquals (manager.getCurrentFactoryPosition(), 0);

            // The three rules, each spot-checked on the parameter it governs: character at zero,
            // structure at a usable middle, and "not acting" at whatever value that is.
            expectWithinAbsoluteError (valueOf (host, ParamIDs::modulation), 0.0f, 1.0e-4f);
            expectWithinAbsoluteError (valueOf (host, ParamIDs::grain),      0.0f, 1.0e-4f);
            expectWithinAbsoluteError (valueOf (host, ParamIDs::decay),      0.5f, 1.0e-4f);
            expectWithinAbsoluteError (valueOf (host, ParamIDs::size),       0.5f, 1.0e-4f);
            expectWithinAbsoluteError (valueOf (host, ParamIDs::preDelay),   0.0f, 1.0e-4f);

            // Damping opens in OPPOSITE directions - HF wide open is 1.0 and LF wide open is 0.0 -
            // which is the single easiest thing here to invert.
            expectWithinAbsoluteError (ParamFormat::dampHFHz (valueOf (host, ParamIDs::dampHF)),
                                       16000.0f, 1.0f);
            expectWithinAbsoluteError (ParamFormat::dampLFHz (valueOf (host, ParamIDs::dampLF)),
                                       40.0f, 0.1f);

            // Mix is 50 % because REFLECT-84 is a wet/dry effect; the serial castings are at 100 %.
            expectWithinAbsoluteError (ParamFormat::mixPercent (valueOf (host, ParamIDs::mix)),
                                       50.0f, 0.01f);
            expectWithinAbsoluteError (ParamFormat::widthPercent (valueOf (host, ParamIDs::width)),
                                       100.0f, 0.01f);
            expectWithinAbsoluteError (ParamFormat::trimDb (valueOf (host, ParamIDs::trim)),
                                       0.0f, 0.01f);

            expect (ProgramManager::initId().bank == ProgramBank::init,
                    "INIT must be in neither of the other banks");
            expect (ProgramManager::factoryPositionOf (ProgramManager::initId().id) < 0,
                    "INIT's slug must not name a Factory entry");
        }
    }

private:
    /** The user Programs in display order, as ProgramIds. The tests used to walk indices from
        kNumFactoryPrograms upward; there is no such range any more. */
    static std::vector<ProgramId> userPrograms (const ProgramManager& manager)
    {
        std::vector<ProgramId> out;

        for (const auto& id : manager.listPrograms())
            if (id.bank == ProgramBank::user)
                out.push_back (id);

        return out;
    }

    static float valueOf (TestHostProcessor& host, const char* id)
    {
        return host.apvts.getRawParameterValue (id)->load();
    }

    static void setValueOf (TestHostProcessor& host, const char* id, float value)
    {
        if (auto* p = host.apvts.getParameter (id))
            p->setValueNotifyingHost (value);
    }
};

static ProgramManagerTests programManagerTests;
