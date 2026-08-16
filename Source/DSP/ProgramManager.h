#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <nf/ParameterSnapshot.h>
#include <nf/UserProgramStore.h>

#include "FactoryPrograms.h"

#include <functional>
#include <vector>

/**
    Program storage: a read-only factory bank plus a user bank on disk.

    The architecture is Gatecrasher's, reused directly rather than redesigned - it is the settled
    suite pattern and CHORUS-60 already ported it verbatim. Terminology is Programs, never Presets,
    per BRAND.md: in the UI label, the class names, the file extension, and the docs alike.

    The invariants that matter:

    - Factory Programs occupy indices [0, kNumFactoryPrograms) and are never written to disk.
      User Programs occupy [kNumFactoryPrograms, getNumPrograms()), one XML file each, sorted
      alphabetically by filename so the order is stable across relaunches.

    - SAVE ALWAYS CREATES A NEW PROGRAM. There is no overwrite verb and no "New Program" action:
      starting fresh is loading any Program, moving something, and saving. Unlike both siblings,
      a name collision here creates a distinct file rather than silently replacing the existing
      Program's contents - neither of them checks, so "never overwrites" held only in the sense
      that the API had no store-to-slot method.

    - DELETE is only ever valid for a User Program. Gated twice: the header disables the button,
      and deleteUserProgram() no-ops on a factory index regardless of what called it.

    - setCurrentProgram can arrive on the AUDIO thread, because VST3 delivers a program change as
      an ordinary automatable parameter, while applying one calls setValueNotifyingHost, which is
      message-thread-only. Hence the AsyncUpdater: requestProgramChange() is safe from any thread.
*/
class ProgramManager final : private juce::AsyncUpdater
{
public:
    /** @param state              the APVTS this manages
        @param userDirectoryOverride  where User Programs live. Defaults to the real per-OS
                                      location; the tests pass a temporary directory so they
                                      never write into the user's own Programs folder. */
    explicit ProgramManager (juce::AudioProcessorValueTreeState& state,
                             juce::File userDirectoryOverride = {});
    ~ProgramManager() override;

    /** Applies the default Program and captures the clean baseline. Call once, from the
        processor's constructor. */
    void initialise();

    //==============================================================================
    /** The Factory bank's size - what the host is told, and it never changes. */
    int getNumPrograms() const noexcept { return kNumFactoryPrograms; }

    /** Raw, unnumbered - what the HOST's list wants, since a host renders its own numbering. */
    juce::String getProgramName (int factoryPosition) const;

    ProgramId getCurrentProgramId() const;
    static ProgramId factoryIdAt (int factoryPosition);
    static ProgramId initId();
    static int factoryPositionOf (const juce::String& slug);

    /** The Factory position of the current Program, or 0 when it is INIT, a User Program or
        unresolved - none of which the host's list contains. */
    int getCurrentFactoryPosition() const;

    ProgramId resolve (ProgramBank bank, const juce::String& id, const juce::String& displayName) const;
    std::vector<ProgramId> listPrograms() const;

    /** **What the LCD and the dropdown print - a label, not a key.** Only Factory Programs get the
        two-digit number, computed from their bank position at paint time. */
    juce::String displayLabelFor (const ProgramId& id) const;





    /** Thread-safe from anywhere: stores the index and defers the actual apply to the message
        thread. */
    void requestProgramChange (const ProgramId& id);

    /*  **The pending-program handshake, and it is public so a test can reach it.**

        These two functions ARE the critical section: everything between taking `pendingLock` and
        releasing it happens inside them, and nothing else touches `pendingProgram`. An allocation
        sentinel is not lock-aware, so a probe around `requestProgramChange` cannot distinguish heap
        work under the lock from heap work beside it — the totals are identical either way. Arming
        it around a function that is exactly the locked region is the only honest way to assert the
        property, and that is worth the two names on this class.

        See their definitions for what moved out of the lock and why 0.12 us was never the argument. */
    ProgramId exchangePendingProgram (ProgramId incoming);
    bool takePendingProgram (ProgramId& out);


    /** Drops any deferred change. setStateInformation MUST call this before restoring, or a
        request that arrived just beforehand lands afterwards and clobbers the restored session. */
    void cancelPendingChange();

    /** Applies a deferred change right now instead of waiting for the message loop. Only the
        tests need this: the console app they run in has no message loop to deliver the async
        callback, so without it every requestProgramChange would silently never arrive. */
    void flushPendingChange() { handleUpdateNowIfNeeded(); }

    /** Restores the "which Program was I on" display state without touching parameter values, and
        treats the current values as the new clean baseline. */
    void setCurrentProgramWithoutApplying (const ProgramId& id);

    //==============================================================================
    void saveNewUserProgram (const juce::String& requestedName);
    void deleteUserProgram (const ProgramId& id);

    /** True once any parameter differs from the loaded Program. Drives SAVE's enablement - there
        is nothing to save until something has actually moved. */
    bool isModifiedFromLoadedProgram() const;

    /** Fired when the list or the current index changes, so the processor can call
        updateHostDisplay. A std::function rather than a listener interface so this class needs to
        know nothing about juce::AudioProcessor. */
    std::function<void()> onProgramListChanged;

    /** Where this instance stores User Programs. */
    juce::File getUserProgramDirectory() const;

    /** The real per-OS location, independent of any override. */
    static juce::File getDefaultUserProgramDirectory();

    static juce::String getProgramFileExtension() { return ".reflect84program"; }

    /** **39, and it is derived rather than asserted.**

        The name cell holds 41 characters at the font actually drawn (see ReflectTheme's
        lcdCharacterBudget, measured in Tests/DisplayBudgetTests.cpp). The dirty marker " *" takes
        2 and the naming cursor takes 1, so the cap is the budget less the larger of the two.

        **It grew from 35 with the 34px header row**, because the LCD widened 586 -> 641: the
        second legend cost no button width (each was already sized by its longest word) and the
        row gaps either side came in from 10px to 8px. The cell took all of it.

        **A budget may grow; a cap may never shrink.** If a future change to the header height,
        font size, tracking or cell width would reduce this, take the room from padding or widen
        the cell instead. Lowering it orphans names already saved to disk - they would load, then
        fail to save back under their own name.

        It was 22 before that, with no stated derivation, computed against a font size the panel
        does not use and against a display that still carried a two-digit index prefix on user
        names. Both are gone: only Factory Programs are numbered now. */
    static constexpr int maxProgramNameLength = 39;

private:
    void handleAsyncUpdate() override;

    void applyProgram (const ProgramId& id);
    void setCurrentId (const ProgramId& id);
    void applyFactoryProgram (const FactoryProgram& program);
    void captureCleanSnapshot();

    juce::AudioProcessorValueTreeState& apvts;

    /** The User bank on disk. Scanning, sorting, naming, the collision check, save and delete are
        core's; WHAT a Program contains - the whole APVTS state - stays here. */
    nf::UserProgramStore store;

    mutable juce::SpinLock currentIdLock;
    ProgramId currentId;
    juce::SpinLock pendingLock;

    bool hasPendingProgram = false;
    ProgramId pendingProgram;

    /** The baseline the dirty flag compares against. Message-thread only - every writer runs
        there, so it needs no synchronisation. Keyed by parameter ID inside; see
        nf/ParameterSnapshot.h for why that is not an index. */
    nf::ParameterSnapshot cleanSnapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgramManager)
};
