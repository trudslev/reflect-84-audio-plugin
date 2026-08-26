#include <juce_events/juce_events.h>
#include <nf/UserProgramDirectory.h>
#include <iostream>
#include <vector>
#include <nf/testing/ExpectedFailure.h>

/*  **JUCE's default logger writes to OutputDebugString on Windows, so every line this suite logs
    was invisible in Windows CI.**

    `Logger::outputDebugString` is `std::cerr << text` on POSIX and `OutputDebugString (...)` on
    Windows (`juce_win32_Misc.cpp`), which goes to an attached debugger and nowhere else. The target
    is a console app, so this is not a subsystem problem — it is which sink JUCE picked.

    Measured on Elmer: the Windows step printed **8 lines against macOS's 94**, and the job was
    green. It passes silently and fails loudly — `TestMain` writes its summary to `std::cerr` and
    returns non-zero — so a real failure still goes red. But "green with no evidence" is
    indistinguishable, to a reader, from a step that never ran the binary at all.

    Installing this settled which: all three platforms print 94 suites now, so they were running.

    **RAII rather than an install/clear pair**, because several of these `main`s return early from
    inside a loop, and a trailing clear would be skipped on exactly the path that matters. */
struct ConsoleLogger final : juce::Logger
{
    ConsoleLogger()           { juce::Logger::setCurrentLogger (this); }
    ~ConsoleLogger() override { juce::Logger::setCurrentLogger (nullptr); }

    void logMessage (const juce::String& message) override
    {
        std::cout << message << std::endl;
    }
};

int main()
{
    ConsoleLogger consoleLogger;

    // ProgramManager defers every apply through juce::AsyncUpdater, and triggerAsyncUpdate()
    // needs a MessageManager: without one, activeMessage->post() fails, the pending flag is
    // cleared, and handleUpdateNowIfNeeded() then has nothing to deliver. The failure is silent -
    // no assertion in a release build, just a program change that never arrives - so any test of
    // that path would pass while proving nothing. This initialiser is what makes those tests real.
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;

    // **Every suite in this process resolves User Programs under a scratch directory, not the
    // user's real one.** The test harness compiles the shipping AudioProcessor, which builds its
    // ProgramManager from the real per-OS path because that is its job — so without this, any test
    // constructing the processor can reach
    // ~/Library/Application Support/<Company>/<Product>/Programs.
    //
    // A comment saying "do not write there" is a convention, and a convention gets broken silently.
    // It is also the one most likely to be broken by someone doing the right thing: verifying the
    // Program list needs several saved Programs, and building that state by hand is the obvious way
    // to get it. A cleanup glob has already destroyed a Program a user had just saved.
    //
    // Installed before the runner for the same reason ScopedJuceInitialiser_GUI is: it has to be in
    // force before the first line of the first test. See nf/UserProgramDirectory.h.
    const nf::ScopedUserProgramDirectoryOverride programRedirect {
        juce::File::getSpecialLocation (juce::File::tempDirectory)
            .getChildFile ("NeonFoundryTestPrograms")
    };

    /*  **The arms this casting is KNOWN to fail, declared rather than left red.**

        A permanently red suite cannot report a NEW failure — the count simply moves, in a job that
        was already failing — and it blocks publishing outright, because `publish` needs the build
        jobs. Each row names what its resolution waits on. See nf/testing/ExpectedFailure.h.

        **This list is the vacuity guard.** An expected failure that never executes is
        indistinguishable from one that executed and failed as expected: nothing is reported either
        way. So every id here must be REACHED, and one that is not fails the run.
    */
    static const std::vector<nf::testing::ExpectedFailure> declaredExpectedFailures {
        { "reflect84.dampHF-2x2",
          "an unclassified dampHF corner - four pre-stated readings, none matched; figures reported, nothing assigned" },
    };

    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    // Declared-but-not-executed is a FAILURE, not a pass. A skipped beginTest, a suite dropped from
    // target_sources, or a condition that stopped being reached would each satisfy the expectation
    // silently otherwise.
    for (const auto& id : nf::testing::expectedFailuresNotExecuted (declaredExpectedFailures))
    {
        std::cerr << "*** expected failure '" << id
                  << "' was declared but never reached — it cannot have been satisfied\n";
        ++failures;
    }

    return failures > 0 ? 1 : 0;
}
