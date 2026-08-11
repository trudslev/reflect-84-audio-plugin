#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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
    int getNumPrograms() const;
    juce::String getProgramName (int index) const;

    /** What the LCD and the dropdown show: a two-digit 1-based index, a space, then the name.
        getProgramName stays raw - that is what the HOST's program list wants, since a host renders
        its own numbering and would print "01" twice. INIT is unnumbered in both, because it is
        outside the bank and a number would place it in a running order it is not part of. */
    juce::String getProgramDisplayName (int index) const;

    /** INIT sits outside both banks at index -1, so it is neither factory nor user. */
    static bool isInitProgram (int index) noexcept { return index == initProgramIndex; }

    static bool isFactoryProgram (int index) noexcept
    {
        return index >= 0 && index < kNumFactoryPrograms;
    }

    int getCurrentProgram() const noexcept
    {
        return currentProgramIndex.load (std::memory_order_relaxed);
    }

    /** Thread-safe from anywhere: stores the index and defers the actual apply to the message
        thread. */
    void requestProgramChange (int index);

    /** Drops any deferred change. setStateInformation MUST call this before restoring, or a
        request that arrived just beforehand lands afterwards and clobbers the restored session. */
    void cancelPendingChange();

    /** Applies a deferred change right now instead of waiting for the message loop. Only the
        tests need this: the console app they run in has no message loop to deliver the async
        callback, so without it every requestProgramChange would silently never arrive. */
    void flushPendingChange() { handleUpdateNowIfNeeded(); }

    /** Restores the "which Program was I on" display state without touching parameter values, and
        treats the current values as the new clean baseline. */
    void setCurrentProgramIndexWithoutApplying (int index);

    //==============================================================================
    void saveNewUserProgram (const juce::String& requestedName);
    void deleteUserProgram (int index);

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

    /** The LCD's usable width at 17px with .16em tracking. */
    static constexpr int maxProgramNameLength = 22;

private:
    void handleAsyncUpdate() override;

    void refreshUserProgramList();
    void applyProgramByIndex (int index);
    void applyFactoryProgram (const FactoryProgram& program);
    void captureCleanSnapshot();

    juce::AudioProcessorValueTreeState& apvts;

    const juce::File userDirectory;
    juce::Array<juce::File> userProgramFiles;

    std::atomic<int> currentProgramIndex { defaultFactoryProgramIndex };
    // **-2, not -1.** -1 is INIT's index now, so it can no longer double as "nothing pending" -
    // using it would make selecting INIT indistinguishable from having nothing queued.
    static constexpr int noPendingProgram = -2;
    std::atomic<int> pendingProgramIndex { noPendingProgram };

    /** Normalised values in getParameters() order. Message-thread only - every writer runs there,
        so it needs no synchronisation. */
    std::vector<float> cleanSnapshot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgramManager)
};
