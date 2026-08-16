#include "ProgramManager.h"

#include "../Parameters.h"

#include <nf/UserProgramDirectory.h>

#include <algorithm>
#include <cmath>

#if ! defined (NF_COMPANY_NAME) || ! defined (NF_PRODUCT_NAME)
 #error "NF_COMPANY_NAME and NF_PRODUCT_NAME must come from CMake. They cannot be read from \
JucePlugin_* here: those macros only exist in the plugin target's generated header, and this file \
is also compiled into the Tests console app. CHORUS-60's CMakeLists records what a hand-synced \
copy costs - it drifted from the real company name and quietly pointed saved Programs at a \
directory nothing was writing to."
#endif

//==============================================================================
ProgramManager::ProgramManager (juce::AudioProcessorValueTreeState& state,
                                juce::File userDirectoryOverride)
    : apvts (state),
      store (nf::userProgramDirectory (NF_COMPANY_NAME, NF_PRODUCT_NAME, userDirectoryOverride),
             getProgramFileExtension(),
             maxProgramNameLength)
{
    // The identity must be valid before initialise() runs, as the atomic index it replaced was
    // valid from its in-class initialiser: a host may query the moment the processor exists.
    setCurrentId (factoryIdAt (defaultFactoryProgramIndex));

    store.refresh();
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
    return store.getDirectory();
}

juce::File ProgramManager::getDefaultUserProgramDirectory()
{
    // **Moved to core**, where the reasoning lives with it: the AU-preset-folder mistake five
    // castings made, and the macOS "Application Support" segment that went missing for a day
    // because a comment claimed JUCE resolved it. See nf/UserProgramDirectory.h.
    //
    // Company and product stay HERE. They are this casting's identity, and core takes them as
    // arguments precisely so no shared default can exist to drift - CHORUS-60's hand-synced copy
    // drifted to a dead company name and quietly pointed saved Programs at a directory nothing
    // reads. The #error at the top of this file is what guarantees they arrive.
    return nf::userProgramDirectory (NF_COMPANY_NAME, NF_PRODUCT_NAME);
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
        if (store.fileFor (id) != juce::File())
            return { ProgramBank::user, id, id };

    // **Degrade honestly.** The restored values are correct and stay put; only the name is unknown.
    return { ProgramBank::unresolved, id, displayName.isNotEmpty() ? displayName : id };
}

std::vector<ProgramId> ProgramManager::listPrograms() const
{
    std::vector<ProgramId> out;
    out.reserve (1 + kFactoryPrograms.size() + (size_t) store.getFiles().size());

    out.push_back (initId());

    for (size_t i = 0; i < kFactoryPrograms.size(); ++i)
        out.push_back (factoryIdAt ((int) i));

    for (const auto& f : store.getFiles())
    {
        const auto stem = f.getFileNameWithoutExtension();
        out.push_back ({ ProgramBank::user, stem, stem });
    }

    return out;
}

juce::String ProgramManager::displayLabelFor (const ProgramId& id) const
{
    // The Factory position is resolved here because the Factory bank is this casting's own; core
    // never holds one. The two-digit number is presentation and is computed, never stored.
    return nf::programDisplayLabel (id, id.bank == ProgramBank::factory ? factoryPositionOf (id.id)
                                                                        : -1);
}


/*  **The critical section is a SWAP now, and it used to be two assignments.**

    A `juce::String` copy is a refcount increment and reads as safe. The ASSIGNMENT is the other
    half: it releases whatever the target held first, and a refcount reaching zero calls `free()`.
    So `pendingProgram = id` and `id = pendingProgram` each did heap work, and both were inside the
    lock — on a path VST3 can deliver **on the audio thread**, since a program change is an
    automatable parameter there.

    **Measured at 0.12 us worst case against a 10,667 us block budget**, so this was never a dropout
    risk and is not sold as one. It is negligible because a refcount release happens to be cheap,
    not because anything guarantees the path stays heap-free — and the next person to add a field to
    `ProgramId` has no reason to think about it.

    The copy and the destruction both move OUT of the lock: `exchangePendingProgram` takes its
    argument by value, so the caller's copy is made in the caller's frame, and returns the previous
    program by value, so its release happens in the caller's frame too. What is left between the
    lock and the unlock is a pointer exchange.

    **Named functions rather than inline blocks because that is what makes it testable.** An
    allocation sentinel is not lock-aware, so a probe around `requestProgramChange` sees the same
    total either way — the change is WHERE the work happens, not whether it happens. Arming the
    sentinel around a function that IS the critical section is the only honest way to assert it. */
ProgramId ProgramManager::exchangePendingProgram (ProgramId incoming)
{
    const juce::SpinLock::ScopedLockType lock (pendingLock);

    std::swap (pendingProgram, incoming);
    hasPendingProgram = true;

    return incoming;   // the PREVIOUS pending program; it is released in the caller's frame
}

bool ProgramManager::takePendingProgram (ProgramId& out)
{
    const juce::SpinLock::ScopedLockType lock (pendingLock);

    if (! hasPendingProgram)
        return false;

    // `out` is empty on entry, so this is a pointer exchange and nothing is released here.
    std::swap (out, pendingProgram);
    hasPendingProgram = false;

    return true;
}

void ProgramManager::requestProgramChange (const ProgramId& id)
{
    // The copy is made HERE, in this frame: copying a ProgramId is two refcount increments, and an
    // increment never frees. The previous pending program comes back and is released here too.
    const ProgramId previous = exchangePendingProgram (id);
    juce::ignoreUnused (previous);

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

    if (! takePendingProgram (id))
        return;

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
        const auto file = store.fileFor (id.id);

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
    // The four call sites are unchanged - apply, restore, save, and the constructor. What moved is
    // the storage: nf::ParameterSnapshot keys by parameter ID rather than by getParameters() order,
    // so the "did the count change" guard this used to need is gone. That guard silently reported
    // "not modified" whenever it fired, which is the wrong direction to fail in.
    cleanSnapshot.capture (apvts.processor);
}

bool ProgramManager::isModifiedFromLoadedProgram() const
{
    // No exclusion predicate here: every parameter on this casting counts as an edit. Chorus-60
    // passes one for its pager latches; the distinction is documented on ParameterSnapshot.
    return cleanSnapshot.differsFrom (apvts.processor);
}

//==============================================================================
void ProgramManager::saveNewUserProgram (const juce::String& requestedName)
{
    // **What a Program CONTAINS stays here** - the whole APVTS state plus the schema version. Core
    // owns naming, the collision check and the write, and takes finished XML.
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute (LegacyMigration::stateSchemaVersionAttribute,
                       LegacyMigration::currentStateSchemaVersion);

    // **The empty-name fallback is `TAKE n` now, not `NEW PROGRAM`.** The suite had five different
    // ones across six castings; TAKE n is the one that is better rather than merely different,
    // since consecutive empty saves give TAKE 3, TAKE 4 instead of leaning on getNonexistentSibling
    // for "NEW PROGRAM (2)". Trimming, upper-casing and the 39-character cap are core's now too -
    // this casting already applied all three here, so only the fallback string changes.
    //
    // The collision check that made "never overwrites" true rather than merely unimplemented
    // originated in this casting; it is core's guarantee for all six now.
    const auto file = store.save (requestedName, *xml);

    if (file == juce::File())
        return;   // the write failed; the panel keeps naming the Program it was already on

    // **The stem comes off the file core returned, not off the requested name.** A collision takes
    // the next free sibling, so taking it from the request would point the panel at the first file
    // while the values came from the second.
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

    const bool wasCurrent = getCurrentProgramId() == id;

    if (! store.remove (id.id))
        return;

    // Deliberately NOT the unresolved state: deleting from the panel is unambiguous intent.
    if (wasCurrent)
        requestProgramChange (factoryIdAt (defaultFactoryProgramIndex));   // fires the callback
    else if (onProgramListChanged)
        onProgramListChanged();
}
