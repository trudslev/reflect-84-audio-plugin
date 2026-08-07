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

            expectEquals (manager.getNumPrograms(), kNumFactoryPrograms);
            expect (ProgramManager::isFactoryProgram (0));
            expect (ProgramManager::isFactoryProgram (kNumFactoryPrograms - 1));
            expect (! ProgramManager::isFactoryProgram (kNumFactoryPrograms));
            expect (! ProgramManager::isFactoryProgram (-1));
            expectEquals (manager.getProgramName (0), juce::String ("RAIN ALL DAY"));
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

            expectEquals (manager.getNumPrograms(), kNumFactoryPrograms + 1);
            expectEquals (manager.getCurrentProgram(), kNumFactoryPrograms);
            expectEquals (manager.getProgramName (kNumFactoryPrograms), juce::String ("MY PROGRAM"));

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
            const auto firstName = manager.getProgramName (manager.getCurrentProgram());

            setValueOf (host, ParamIDs::decay, 0.80f);
            manager.saveNewUserProgram ("SAME NAME");
            const auto secondName = manager.getProgramName (manager.getCurrentProgram());

            expectEquals (manager.getNumPrograms(), kNumFactoryPrograms + 2);

            // Deliberately compared by NAME, not by index: the list is re-sorted after every
            // save, so a new Program can take an index an older one previously held. Index
            // equality here would prove nothing.
            expect (firstName != secondName, "the second save reused the first Program's name");

            // The original Program's values survived untouched - that is the actual guarantee.
            bool foundOriginal = false;

            for (int i = kNumFactoryPrograms; i < manager.getNumPrograms(); ++i)
            {
                if (manager.getProgramName (i) != firstName)
                    continue;

                manager.requestProgramChange (i);
                manager.flushPendingChange();
                expectWithinAbsoluteError (valueOf (host, ParamIDs::decay), 0.20f, 1.0e-3f);
                foundOriginal = true;
            }

            expect (foundOriginal, "the first Program is gone - it was overwritten");
        }

        beginTest ("Delete no-ops on a factory index, whatever calls it");
        {
            ScopedTestDirectory dir { "deleteFactory" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            for (int i = 0; i < kNumFactoryPrograms; ++i)
                manager.deleteUserProgram (i);

            expectEquals (manager.getNumPrograms(), kNumFactoryPrograms);
        }

        beginTest ("Delete removes a user Program and falls back if it was loaded");
        {
            ScopedTestDirectory dir { "deleteUser" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();
            setValueOf (host, ParamIDs::width, 0.9f);
            manager.saveNewUserProgram ("DOOMED");

            const int index = manager.getCurrentProgram();
            expectEquals (index, kNumFactoryPrograms);

            manager.deleteUserProgram (index);
            manager.flushPendingChange();

            expectEquals (manager.getNumPrograms(), kNumFactoryPrograms);
            expectEquals (manager.getCurrentProgram(), defaultFactoryProgramIndex);
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

            const int saved = manager.getCurrentProgram();

            manager.requestProgramChange (defaultFactoryProgramIndex);
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
            manager.requestProgramChange (5);
            manager.cancelPendingChange();
            manager.flushPendingChange();

            expectWithinAbsoluteError (valueOf (host, ParamIDs::mix), 0.42f, 1.0e-4f);
            expectEquals (manager.getCurrentProgram(), defaultFactoryProgramIndex);
        }

        beginTest ("out-of-range program changes are ignored, not clamped into something else");
        {
            ScopedTestDirectory dir { "bounds" };
            TestHostProcessor host;
            ProgramManager manager { host.apvts, dir.directory };

            manager.initialise();

            for (const int index : { -1, kNumFactoryPrograms, 9999 })
            {
                manager.requestProgramChange (index);
                manager.flushPendingChange();
                expectEquals (manager.getCurrentProgram(), defaultFactoryProgramIndex);
            }
        }
    }

private:
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
