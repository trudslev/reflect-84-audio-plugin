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
    // The identity must be valid before initialise() runs, as the atomic index it replaced was
    // valid from its in-class initialiser: a host may query the moment the processor exists.
    setCurrentId (factoryIdAt (defaultFactoryProgramIndex));

    refreshUserProgramList();
}

ProgramManager::~ProgramManager()
{
    cancelPendingUpdate();
}

void ProgramManager::initialise()
{
    applyProgram (factoryIdAt (defaultFactoryProgramIndex));
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
                   // The STEM, not getFileName(): comparing with the extension attached sorts
                   // "AB C" before "AB", because a space (0x20) precedes the dot (0x2E).
                   return a.getFileNameWithoutExtension()
                           .compareIgnoreCase (b.getFileNameWithoutExtension()) < 0;
               });
}

//==============================================================================
juce::String ProgramManager::getProgramName (int factoryPosition) const
{
    if (juce::isPositiveAndBelow (factoryPosition, kNumFactoryPrograms))
        return kFactoryPrograms[(size_t) factoryPosition].name;

    return {};
}

//==============================================================================
//==============================================================================
// Identity. Nothing below addresses a Program by position except the deliberate crossings.

ProgramId ProgramManager::factoryIdAt (int factoryPosition)
{
    const auto& p = kFactoryPrograms[(size_t) factoryPosition];
    return { ProgramBank::factory, p.slug, p.name };
}

ProgramId ProgramManager::initId()
{
    return { ProgramBank::init, kInitProgram.slug, kInitProgram.name };
}

int ProgramManager::factoryPositionOf (const juce::String& slug)
{
    for (size_t i = 0; i < kFactoryPrograms.size(); ++i)
        if (slug == kFactoryPrograms[i].slug)
            return (int) i;

    return -1;
}

ProgramId ProgramManager::getCurrentProgramId() const
{
    const juce::SpinLock::ScopedLockType lock (currentIdLock);
    return currentId;
}

void ProgramManager::setCurrentId (const ProgramId& id)
{
    const juce::SpinLock::ScopedLockType lock (currentIdLock);
    currentId = id;
}

int ProgramManager::getCurrentFactoryPosition() const
{
    const auto id = getCurrentProgramId();

    if (id.bank == ProgramBank::factory)
        if (const int pos = factoryPositionOf (id.id); pos >= 0)
            return pos;

    return 0;
}

ProgramId ProgramManager::resolve (ProgramBank bank, const juce::String& id,
                                    const juce::String& displayName) const
{
    if (bank == ProgramBank::init && id == kInitProgram.slug)
        return initId();

    if (bank == ProgramBank::factory)
        if (const int pos = factoryPositionOf (id); pos >= 0)
            return factoryIdAt (pos);

    if (bank == ProgramBank::user)
        for (const auto& f : userProgramFiles)
            if (f.getFileNameWithoutExtension() == id)
                return { ProgramBank::user, id, id };

    // **Degrade honestly.** The restored values are correct and stay put; only the name is unknown.
    return { ProgramBank::unresolved, id, displayName.isNotEmpty() ? displayName : id };
}

std::vector<ProgramId> ProgramManager::listPrograms() const
{
    std::vector<ProgramId> out;
    out.reserve (1 + kFactoryPrograms.size() + (size_t) userProgramFiles.size());

    out.push_back (initId());

    for (size_t i = 0; i < kFactoryPrograms.size(); ++i)
        out.push_back (factoryIdAt ((int) i));

    for (const auto& f : userProgramFiles)
    {
        const auto stem = f.getFileNameWithoutExtension();
        out.push_back ({ ProgramBank::user, stem, stem });
    }

    return out;
}

juce::String ProgramManager::displayLabelFor (const ProgramId& id) const
{
    if (id.bank == ProgramBank::factory)
        if (const int pos = factoryPositionOf (id.id); pos >= 0)
            return juce::String (pos + 1).paddedLeft ('0', 2) + " " + id.displayName;

    return id.displayName;
}

juce::File ProgramManager::userProgramFile (const juce::String& stem) const
{
    for (const auto& f : userProgramFiles)
        if (f.getFileNameWithoutExtension() == stem)
            return f;

    return {};
}

void ProgramManager::requestProgramChange (const ProgramId& id)
{

    {
        const juce::SpinLock::ScopedLockType lock (pendingLock);
        pendingProgram = id;
        hasPendingProgram = true;
    }

    triggerAsyncUpdate();
}

void ProgramManager::cancelPendingChange()
{
    {
        const juce::SpinLock::ScopedLockType lock (pendingLock);
        hasPendingProgram = false;
    }

    cancelPendingUpdate();
}

void ProgramManager::handleAsyncUpdate()
{
    ProgramId id;

    {
        const juce::SpinLock::ScopedLockType lock (pendingLock);

        if (! hasPendingProgram)
            return;

        id = pendingProgram;
        hasPendingProgram = false;
    }

    applyProgram (id);
}

void ProgramManager::setCurrentProgramWithoutApplying (const ProgramId& id)
{
    setCurrentId (id);

    // The restored session IS its own baseline. The accepted consequence is that SAVE starts
    // disabled after reopening a session that had unsaved edits.
    captureCleanSnapshot();

    if (onProgramListChanged)
        onProgramListChanged();
}

//==============================================================================
void ProgramManager::applyProgram (const ProgramId& id)
{
    if (id.bank == ProgramBank::init)
    {
        // The slug is checked, not just the bank. An id claiming to be INIT with some other
        // identifier names nothing, and applying INIT anyway would be the same "land on whatever
        // is nearby" failure the whole model exists to prevent.
        if (id.id != kInitProgram.slug)
            return;

        applyFactoryProgram (kInitProgram);
    }
    else if (id.bank == ProgramBank::factory)
    {
        const int pos = factoryPositionOf (id.id);

        if (pos < 0)
            return;

        applyFactoryProgram (kFactoryPrograms[(size_t) pos]);
    }
    else if (id.bank == ProgramBank::user)
    {
        const auto file = userProgramFile (id.id);

        if (file == juce::File())
            return;

        std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));

        if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
            return;

        apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
    else
    {
        // Unresolved: the values are whatever the session restored and stay exactly as they are.
        setCurrentId (id);
        captureCleanSnapshot();

        if (onProgramListChanged)
            onProgramListChanged();

        return;
    }

    setCurrentId (id);
    captureCleanSnapshot();

    if (onProgramListChanged)
        onProgramListChanged();
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

    const auto stem = file.getFileNameWithoutExtension();
    setCurrentId ({ ProgramBank::user, stem, stem });

    // The just-saved Program IS the current parameter state, so it becomes the new clean baseline
    // and SAVE goes straight back to disabled until something moves again.
    captureCleanSnapshot();

    if (onProgramListChanged)
        onProgramListChanged();
}

void ProgramManager::deleteUserProgram (const ProgramId& id)
{
    // Gated here as well as at the button, so no code path can delete a factory Program.
    if (id.bank != ProgramBank::user)
        return;

    const auto file = userProgramFile (id.id);

    if (file == juce::File())
        return;

    const bool wasCurrent = getCurrentProgramId() == id;

    file.deleteFile();
    refreshUserProgramList();

    // Deliberately NOT the unresolved state: deleting from the panel is unambiguous intent.
    if (wasCurrent)
        requestProgramChange (factoryIdAt (defaultFactoryProgramIndex));   // fires the callback
    else if (onProgramListChanged)
        onProgramListChanged();
}
