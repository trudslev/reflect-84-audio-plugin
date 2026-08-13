#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

#include <nf/UserProgramDirectory.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    The first tests in this casting that run against the REAL editor.

    **Why this file exists.** REFLECT-84 shipped the stale-replay guard with zero call sites for its
    disarm. The guard was correct, the processor was correct, and every suite here passed — because
    the defect was in `ReflectEditorContent.cpp`, which was not compiled into any target a test could
    run. The test harness port is what makes this file possible; the port without a test using it
    would just be a longer build.

    **What can honestly be asserted here, and what cannot.** A knob only takes the LCD over, and only
    disarms the guard, while it is genuinely being dragged - `nf::connectUserEdit` guards on
    `isMouseButtonDown()`. That state lives in the mouse source, not in the component, so a headless
    test cannot fake a drag: there is no windowed peer for a synthetic event to arrive through.
    Asserting "a user edit disarms the guard" from here is therefore not available, and pretending
    otherwise would be a claim of coverage rather than coverage. `tools/check_user_edit_wiring.py`
    covers the call-site question statically, and `nf::UserEditGate`'s own tests cover the mechanism.

    What IS available is the other half, and it is the half that has a rejected design behind it:
    **with a real editor attached and its attachments firing, a host's parameter writes must not
    disarm the guard.** That is exactly what a `ValueTree::Listener` inside core would have broken -
    the extraction plan specified one, and it was rejected because a listener cannot tell a person
    from an automation lane. This file is the regression test for that decision, so re-introducing it
    fails here rather than in a session six months from now.
*/
class EditorWiringTests final : public juce::UnitTest
{
public:
    EditorWiringTests() : juce::UnitTest ("Editor wiring", "GUI") {}

    void runTest() override
    {
        beginTest ("The real editor constructs, lays out and tears down");
        {
            // Worth its own case even though it asserts almost nothing: until the harness port,
            // nothing here executed a line of the editor at all, so a null dereference or a failed
            // assertion in layout would have been found by opening the plugin in a DAW.
            //
            // This constructs the shipping processor, which builds its ProgramManager from the
            // resolved user-Programs path — it has no injectable override, only ProgramManager does.
            // **That path is redirected to a scratch directory for the whole process**, by the
            // nf::ScopedUserProgramDirectoryOverride in TestMain, so nothing here can reach real
            // Programs. ProgramDirectoryRedirectTests below asserts that is true in this binary
            // rather than trusting it.
            Reflect84AudioProcessor processor;
            auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());

            expect (editor != nullptr, "createEditor returned nothing");

            if (editor != nullptr)
            {
                expectGreaterThan (editor->getWidth(), 0);
                expectGreaterThan (editor->getHeight(), 0);
            }
        }

        beginTest ("A host's parameter writes do not disarm the stale-replay guard");
        {
            Reflect84AudioProcessor processor;
            auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());
            expect (editor != nullptr);

            processor.userEdits.armRestore();

            // Every attachment in the editor fires for these, exactly as it does when a host
            // replays automation on session load. None of them is a drag.
            int written = 0;

            for (auto* parameter : processor.getParameters())
            {
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                {
                    const auto original = ranged->getValue();
                    ranged->setValueNotifyingHost (original < 0.5f ? 0.9f : 0.1f);
                    ++written;
                }
            }

            expectGreaterThan (written, 0, "no parameters were written, so this asserted nothing");

            expect (processor.userEdits.isRestorePending(),
                    "automation disarmed the stale-replay guard. A ValueTree listener would do "
                    "exactly this — see nf/UserEditGate.h for why the shared model does not use "
                    "one, and do not re-introduce it");
        }

        beginTest ("A restored session survives the host replaying its remembered index");
        {
            // The user-visible behaviour the guard exists for, driven through the real processor
            // with the real editor attached — which is the arrangement a host actually has, and the
            // one no test here could reach before the harness port.
            Reflect84AudioProcessor processor;
            auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());

            auto& programs = processor.getProgramManager();

            programs.requestProgramChange (ProgramManager::factoryIdAt (3));
            programs.flushPendingChange();

            juce::MemoryBlock session;
            processor.getStateInformation (session);
            processor.setStateInformation (session.getData(), (int) session.getSize());

            const auto restored = programs.getCurrentProgramId();

            // The replay: a host echoes back the index it remembered, after the state is already
            // correct. Honouring it would re-apply a Program over the restored session.
            processor.setCurrentProgram (processor.getCurrentProgram());
            programs.flushPendingChange();

            expect (programs.getCurrentProgramId() == restored,
                    "the host's replay was honoured and overwrote the restored Program");
        }
    }
};

static EditorWiringTests editorWiringTests;

/** Proves the process-wide redirect is actually in force in THIS binary, rather than merely
    installed in a file somebody could delete.

    `run_tests.py` refuses a target whose TestMain does not install it, and core's own tests prove
    the mechanism redirects. Neither of those establishes that *this* process is redirected, which is
    the thing that keeps a suite off the user's disk — so it is asserted where it matters, against
    the real processor's real ProgramManager.
*/
class ProgramDirectoryRedirectTests final : public juce::UnitTest
{
public:
    ProgramDirectoryRedirectTests() : juce::UnitTest ("Program directory redirect", "programs") {}

    void runTest() override
    {
        beginTest ("The shipping processor cannot reach the user's real Programs directory");
        {
            expect (nf::userProgramDirectoryOverrideRoot() != juce::File(),
                    "no redirect is installed in this process — TestMain must install "
                    "nf::ScopedUserProgramDirectoryOverride before the runner");

            Reflect84AudioProcessor processor;
            const auto used = processor.getProgramManager().getUserProgramDirectory();

            expect (used.isAChildOf (nf::userProgramDirectoryOverrideRoot()),
                    "the processor resolved " + used.getFullPathName()
                        + ", which is outside the redirect root");

            // Named explicitly rather than compared against a rebuilt "real" path: the point is that
            // the application-data root is not on this path at all.
            const auto appData =
                juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);

            expect (! used.isAChildOf (appData),
                    "the processor is pointing inside the user's application data");
        }
    }
};

static ProgramDirectoryRedirectTests programDirectoryRedirectTests;
