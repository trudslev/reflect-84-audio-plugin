#include <juce_events/juce_events.h>

int main()
{
    // ProgramManager defers every apply through juce::AsyncUpdater, and triggerAsyncUpdate()
    // needs a MessageManager: without one, activeMessage->post() fails, the pending flag is
    // cleared, and handleUpdateNowIfNeeded() then has nothing to deliver. The failure is silent -
    // no assertion in a release build, just a program change that never arrives - so any test of
    // that path would pass while proving nothing. This initialiser is what makes those tests real.
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;

    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (runner.getResult (i)->failures > 0)
            return 1;

    return 0;
}
