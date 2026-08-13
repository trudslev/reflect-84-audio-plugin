#include <juce_events/juce_events.h>
#include <nf/UserProgramDirectory.h>

int main()
{
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

    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (runner.getResult (i)->failures > 0)
            return 1;

    return 0;
}
