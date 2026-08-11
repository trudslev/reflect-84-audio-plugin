#include "ProgramManager.h"

#include "../Parameters.h"

#include <algorithm>
#include <cmath>

#if ! defined (NF_COMPANY_NAME) || ! defined (NF_PRODUCT_NAME)
 #error "NF_COMPANY_NAME and NF_PRODUCT_NAME must come from CMake. They cannot be read from \
JucePlugin_* here: those macros only exist in the plugin target's generated header, and this file \
is also compiled into the Tests console app. CHORUS-60's CMakeLists records what a hand-synced \
copy costs - it drifted from the real company name and quietly pointed saved Programs at a \
directory nothing was writing to."
#endif

namespace
{
    /** A parameter is "moved" once it differs by more than this from the loaded Program. Loose
        enough to absorb the float round-trip through a user Program's XML, so a freshly loaded
        Program never reads as dirty; far tighter than the smallest movement any control can make. */
    constexpr float modifiedEpsilon = 1.0e-4f;
}

//==============================================================================
ProgramManager::ProgramManager (juce::AudioProcessorValueTreeState& state,
                                juce::File userDirectoryOverride)
    : apvts (state),
      userDirectory (userDirectoryOverride == juce::File() ? getDefaultUserProgramDirectory()
                                                           : userDirectoryOverride)
{
    refreshUserProgramList();
}

ProgramManager::~ProgramManager()
{
    cancelPendingUpdate();
}

void ProgramManager::initialise()
{
    applyProgramByIndex (defaultFactoryProgramIndex);
}

//==============================================================================
juce::File ProgramManager::getUserProgramDirectory() const
{
    return userDirectory;
}

juce::File ProgramManager::getDefaultUserProgramDirectory()
{
    // **Application data on every platform - no macOS special case.** This used to branch, putting
    // macOS Programs under ~/Library/Audio/Presets. That is Apple's location for the AU PRESET
    // FORMAT: .aupreset files the AU system itself scans, reads and writes. Our user Programs are
    // not those - they are application-owned data in our own XML format - so they belong where an
    // application keeps its data, and the AU folder should hold only what AU understands.
    //
    // **macOS needs the "Application Support" segment added by hand, and only macOS.** JUCE's
    // userApplicationDataDirectory is `~/Library` there - NOT `~/Library/Application Support` -
    // while it is `%APPDATA%` on Windows and `~/.config` on Linux, both of which are already the
    // right root. JUCE's own PropertiesFile appends the segment the same way, for the same reason.
    //
    // This was got wrong once in exactly the plausible direction: the note here used to claim JUCE
    // resolved the segment for us, and that hard-coding it would be wrong on two platforms out of
    // three. The first half was false, and the second half only argues for the `#if` - it is one
    // platform's extra segment, not a shared literal path. Programs landed directly in
    // `~/Library/<Company>/` for a while, which is not where application data goes on macOS and is
    // not a folder anything else writes into.
    //
    // No migration from the old location: nothing has shipped at a released version, so no
    // installed build has ever written a Program there for anyone but us. See Elmer's
    // ProgramManager for the full reasoning - it is a decision, not an oversight.
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);

   #if JUCE_MAC
    dir = dir.getChildFile ("Application Support");
   #endif

    return dir
               .getChildFile (NF_COMPANY_NAME)
               .getChildFile (NF_PRODUCT_NAME)
               .getChildFile ("Programs");
}

void ProgramManager::refreshUserProgramList()
{
    userProgramFiles.clear();

    const auto dir = getUserProgramDirectory();

    if (dir.isDirectory())
        for (const auto& entry : juce::RangedDirectoryIterator (dir, false, "*" + getProgramFileExtension()))
            userProgramFiles.add (entry.getFile());

    // Alphabetical by filename, deliberately not by modification time: the menu's order has to be
    // the same every launch.
    std::sort (userProgramFiles.begin(), userProgramFiles.end(),
               [] (const juce::File& a, const juce::File& b)
               {
                   return a.getFileName().compareIgnoreCase (b.getFileName()) < 0;
               });
}

//==============================================================================
int ProgramManager::getNumPrograms() const
{
    return kNumFactoryPrograms + userProgramFiles.size();
}

juce::String ProgramManager::getProgramName (int index) const
{
    if (isInitProgram (index))
        return kInitProgram.name;

    if (isFactoryProgram (index))
        return kFactoryPrograms[(size_t) index].name;

    const int userIndex = index - kNumFactoryPrograms;

    if (juce::isPositiveAndBelow (userIndex, userProgramFiles.size()))
        return userProgramFiles.getReference (userIndex).getFileNameWithoutExtension();

    return {};
}

//==============================================================================
void ProgramManager::requestProgramChange (int index)
{
    // INIT is a legal target and is NOT isPositiveAndBelow, so it is admitted explicitly rather
    // than by widening the check - which would also admit every other negative index.
    if (! isInitProgram (index) && ! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;

    pendingProgramIndex.store (index, std::memory_order_relaxed);
    triggerAsyncUpdate();
}

void ProgramManager::cancelPendingChange()
{
    pendingProgramIndex.store (noPendingProgram, std::memory_order_relaxed);
    cancelPendingUpdate();
}

void ProgramManager::handleAsyncUpdate()
{
    const int index = pendingProgramIndex.exchange (noPendingProgram, std::memory_order_relaxed);

    if (index != noPendingProgram)
        applyProgramByIndex (index);
}

void ProgramManager::setCurrentProgramIndexWithoutApplying (int index)
{
    // INIT is a valid remembered Program and is NOT isPositiveAndBelow, so it is admitted
    // explicitly rather than by widening the check - which would also admit every other negative
    // index. **No migration is needed**: INIT was ADDED at -1 rather than inserted at 0, so not one
    // existing Factory index moved and every session saved before today still names the sound it
    // was saved with.
    const bool valid = isInitProgram (index) || juce::isPositiveAndBelow (index, getNumPrograms());

    currentProgramIndex.store (valid ? index : defaultFactoryProgramIndex,
                               std::memory_order_relaxed);

    // The restored session IS its own baseline. The accepted consequence is that SAVE starts
    // disabled after reopening a session that had unsaved edits.
    captureCleanSnapshot();

    if (onProgramListChanged)
        onProgramListChanged();
}

//==============================================================================
void ProgramManager::applyProgramByIndex (int index)
{
    if (isInitProgram (index))
    {
        applyFactoryProgram (kInitProgram);
        currentProgramIndex.store (index, std::memory_order_relaxed);
        captureCleanSnapshot();

        if (onProgramListChanged)
            onProgramListChanged();

        return;
    }

    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;

    if (isFactoryProgram (index))
    {
        applyFactoryProgram (kFactoryPrograms[(size_t) index]);
    }
    else
    {
        const int userIndex = index - kNumFactoryPrograms;
        std::unique_ptr<juce::XmlElement> xml (
            juce::XmlDocument::parse (userProgramFiles.getReference (userIndex)));

        if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
            return;

        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }

    currentProgramIndex.store (index, std::memory_order_relaxed);
    captureCleanSnapshot();

    if (onProgramListChanged)
        onProgramListChanged();
}

juce::String ProgramManager::getProgramDisplayName (int index) const
{
    const auto name = getProgramName (index);

    if (isInitProgram (index) || name.isEmpty())
        return name;

    return juce::String (index + 1).paddedLeft ('0', 2) + " " + name;
}

void ProgramManager::applyFactoryProgram (const FactoryProgram& program)
{
    // Assigning through JUCE's typed parameter operator= runs setValueNotifyingHost internally,
    // which is why applying has to happen on the message thread.
    const auto setNormalised = [this] (const char* id, float value)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (id)))
            *p = juce::jlimit (0.0f, 1.0f, value);
        else
            jassertfalse;   // id is not a float parameter, or does not exist
    };

    const auto setChoice = [this] (const char* id, int value)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
            *p = juce::jlimit (0, p->choices.size() - 1, value);
        else
            jassertfalse;
    };

    setChoice     (ParamIDs::algorithm,  program.algorithm);
    setNormalised (ParamIDs::size,       program.size);
    setNormalised (ParamIDs::decay,      program.decay);
    setNormalised (ParamIDs::preDelay,   program.preDelay);
    setNormalised (ParamIDs::density,    program.density);
    setNormalised (ParamIDs::dampHF,     program.dampHF);
    setNormalised (ParamIDs::dampLF,     program.dampLF);
    setNormalised (ParamIDs::modulation, program.modulation);
    setNormalised (ParamIDs::grain,      program.grain);
    setNormalised (ParamIDs::width,      program.width);
    setNormalised (ParamIDs::mix,        program.mix);
    setNormalised (ParamIDs::trim,       program.trim);

    // Every APVTS parameter is listed above. A new parameter that is NOT added here would keep
    // whatever the previously-loaded Program left behind - silently, and differently depending on
    // which Program you came from. Tests/FactoryProgramsTests.cpp asserts the counts match.
}

//==============================================================================
void ProgramManager::captureCleanSnapshot()
{
    const auto& params = apvts.processor.getParameters();

    cleanSnapshot.resize ((size_t) params.size());

    for (int i = 0; i < params.size(); ++i)
        cleanSnapshot[(size_t) i] = params[i]->getValue();
}

bool ProgramManager::isModifiedFromLoadedProgram() const
{
    const auto& params = apvts.processor.getParameters();

    if (cleanSnapshot.size() != (size_t) params.size())
        return false;

    for (int i = 0; i < params.size(); ++i)
        if (std::abs (params[i]->getValue() - cleanSnapshot[(size_t) i]) > modifiedEpsilon)
            return true;

    return false;
}

//==============================================================================
void ProgramManager::saveNewUserProgram (const juce::String& requestedName)
{
    juce::String name = requestedName.trim().toUpperCase();

    if (name.isEmpty())
        name = "NEW PROGRAM";

    if (name.length() > maxProgramNameLength)
        name = name.substring (0, maxProgramNameLength);

    const auto dir = getUserProgramDirectory();

    if (! dir.isDirectory())
        dir.createDirectory();

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute (LegacyMigration::stateSchemaVersionAttribute,
                       LegacyMigration::currentStateSchemaVersion);

    juce::File file = dir.getChildFile (juce::File::createLegalFileName (name) + getProgramFileExtension());

    // Save always creates a NEW Program. Both sibling castings write straight to this path, which
    // means reusing an existing name silently replaces that Program's contents - the one way their
    // "never overwrites" guarantee could actually be broken. getNonexistentSibling appends a
    // counter instead, so the older Program survives and the new one is distinct.
    if (file.existsAsFile())
        file = file.getNonexistentSibling();

    xml->writeTo (file);

    refreshUserProgramList();

    const int newIndex = kNumFactoryPrograms + userProgramFiles.indexOf (file);
    currentProgramIndex.store (newIndex, std::memory_order_relaxed);

    // The just-saved Program IS the current parameter state, so it becomes the new clean baseline
    // and SAVE goes straight back to disabled until something moves again.
    captureCleanSnapshot();

    if (onProgramListChanged)
        onProgramListChanged();
}

void ProgramManager::deleteUserProgram (int index)
{
    // Gated here as well as at the button, so no code path can delete a factory Program.
    if (isFactoryProgram (index))
        return;

    const int userIndex = index - kNumFactoryPrograms;

    if (! juce::isPositiveAndBelow (userIndex, userProgramFiles.size()))
        return;

    const bool wasCurrent = getCurrentProgram() == index;

    userProgramFiles.getReference (userIndex).deleteFile();
    refreshUserProgramList();

    if (wasCurrent)
        requestProgramChange (defaultFactoryProgramIndex);   // which fires the callback itself
    else if (onProgramListChanged)
        onProgramListChanged();
}
