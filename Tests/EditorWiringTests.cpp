#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/GUI/ReflectTheme.h"

#include <nf/HeaderPart.h>
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
        beginTest ("This panel READS the shared header part, rather than agreeing with it");
        {
            /*  **Core owns the mechanism; this proves the wiring** — the same split as
                `nf::connectUserEdit`, where core's own tests prove the gate works and each casting's
                test proves that casting actually calls it.

                It matters here more than most places. A panel can compile against
                `nf::HeaderGeometry` and still hold its own literals: that is precisely what six
                panels did before this round, and §10 records three figures that reached some copies
                and not others. So the assertions below compare this panel's theme against core, and
                a theme that stopped aliasing and went back to literals would fail them the moment
                the part's figure moved — which is the only failure mode worth guarding.

                **This is NOT the same test as core's own.** Core asserts its figures against
                HEADER-PART's published literals; this asserts that this panel is downstream of
                those figures. Neither implies the other, and the pair is what makes the extraction
                a guarantee rather than a convention. */
            namespace L = ReflectTheme::Layout;

            expectEquals ((int) L::programWellX, nf::HeaderGeometry::lcdX);
            expectEquals ((int) L::programWellY, nf::HeaderGeometry::bandY);
            expectEquals ((int) L::programWellW, nf::HeaderGeometry::lcdW);
            expectEquals ((int) L::programWellH, nf::HeaderGeometry::bandH);

            expectEquals ((int) L::saveButtonX, nf::HeaderGeometry::saveX);
            expectEquals ((int) L::saveButtonW, nf::HeaderGeometry::saveW);
            expectEquals ((int) L::deleteButtonX, nf::HeaderGeometry::deleteX);
            expectEquals ((int) L::deleteButtonW, nf::HeaderGeometry::deleteW);

            expectEquals ((int) L::meterInX, nf::HeaderGeometry::inWellX);
            expectEquals ((int) L::meterOutX, nf::HeaderGeometry::outWellX);
            expectEquals ((int) L::meterWellW, nf::HeaderGeometry::meterWellW);

            expectEquals ((int) L::programLabelH, nf::HeaderGeometry::captionH);
            expectEquals ((int) L::meterLabelH, nf::HeaderGeometry::captionH);

            // **The two the round actually moved**, named so the change is findable later: the
            // meters sat at 1162 / 1236 on a 16 px DELETE gap measured off the 3x render, and the
            // part states 18. The render was out of date rather than the measurement wrong.
            expectEquals ((int) L::meterInX, 1164, "the IN well moved 2 px right this round");
            expectEquals ((int) L::meterOutX, 1238, "the OUT well moved with it");

            // The canvas: 648 pinned, closing the 645.13 / 648.63 font-load split.
            expectEquals ((int) L::canvasWidth, nf::HeaderGeometry::canvasWidth);
            expectEquals ((int) L::canvasHeight, 648);
        }

        beginTest ("The LCD budget has ADOPTED the shared figure, now that the face has landed");
        {
            /*  **This arm is inverted, not deleted, and the distinction is the point.**

                It used to assert the two figures DIFFER — a guard holding this panel at its own
                measured 41 while §11's type-adoption gate stood: 49 and 47 are measured on Share
                Tech Mono, and adopting them against a face not in `fonts/` would have overflowed the
                cell irreversibly, because a cap may never shrink.

                The face landed with design bundle 2, so the gate is **satisfied rather than
                waived** and the guard states the property it was always protecting. Deleting it
                would have lost the guard entirely; relaxing it is forbidden; inverting it keeps the
                thing asserted and moves which side of the gate it asserts. That is this suite's
                recorded answer for an assertion that encodes the defect as the property.

                What replaces the old vacuity check is `DisplayBudgetTests`, which measures the
                budget off the font the paint path actually draws rather than from a constant — so
                the two sides still come from different places. */
            expectEquals (ReflectTheme::Layout::lcdCharacterBudget, nf::LcdCell::characterBudget(),
                          "the panel must now carry the shared budget; if these differ, either the "
                          "face was removed from fonts/ or the cell's terms moved");

            expectEquals (ReflectTheme::Layout::lcdCharacterBudget, 49);
            expectEquals (ReflectTheme::Layout::maxUserNameLength, 47);

            // **The cap may never shrink, so the floor is asserted rather than assumed.** 39 was
            // this casting's previous cap; anything below it orphans names already saved — they
            // load, then fail to save back under their own name.
            expectGreaterOrEqual (ReflectTheme::Layout::maxUserNameLength, 39,
                                  "the cap fell below what this casting has already shipped, which "
                                  "is not a re-export but a data migration");

            // The two copies of the cap live in different targets and cannot see each other;
            // DisplayBudgetTests owns that binding and asserts it against the drawn font.
        }

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
