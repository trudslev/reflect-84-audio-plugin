#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <nf/BlockChunking.h>

#include <cmath>

Reflect84AudioProcessor::Reflect84AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createReflect84ParameterLayout())
{
    bypassParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ParamIDs::bypass));
    jassert (bypassParam != nullptr);

    sizeParam       = apvts.getRawParameterValue (ParamIDs::size);
    decayParam      = apvts.getRawParameterValue (ParamIDs::decay);
    preDelayParam   = apvts.getRawParameterValue (ParamIDs::preDelay);
    densityParam    = apvts.getRawParameterValue (ParamIDs::density);
    dampHFParam     = apvts.getRawParameterValue (ParamIDs::dampHF);
    dampLFParam     = apvts.getRawParameterValue (ParamIDs::dampLF);
    modulationParam = apvts.getRawParameterValue (ParamIDs::modulation);
    grainParam      = apvts.getRawParameterValue (ParamIDs::grain);
    widthParam      = apvts.getRawParameterValue (ParamIDs::width);
    mixParam        = apvts.getRawParameterValue (ParamIDs::mix);
    trimParam       = apvts.getRawParameterValue (ParamIDs::trim);
    algorithmParam  = apvts.getRawParameterValue (ParamIDs::algorithm);

    programManager.onProgramListChanged = [this]
    {
        updateHostDisplay (juce::AudioProcessorListener::ChangeDetails().withProgramChanged (true));
    };

    programManager.initialise();
}

//==============================================================================
void Reflect84AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    displaySampleRate = sampleRate;

    const juce::dsp::ProcessSpec spec {
        sampleRate,
        (juce::uint32) juce::jmax (1, samplesPerBlock),
        (juce::uint32) juce::jmax (1, getTotalNumOutputChannels())
    };

    // The pre-delay the first block should already be at, read off the live parameter: a session
    // restore writes the APVTS before the host prepares.
    reverbEngine.prepare (spec, ParamFormat::preDelayMs (preDelayParam->load()),
                          juce::roundToInt (algorithmParam->load()));

    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock, false, false, true);

    mixSmoothed.reset (sampleRate, 0.02);
    trimSmoothed.reset (sampleRate, 0.02);
    widthSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (mixParam->load());
    trimSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (ParamFormat::trimDb (trimParam->load())));
    widthSmoothed.setCurrentAndTargetValue (ParamFormat::widthPercent (widthParam->load()) * 0.01f);

    inputMeterDb.store  (-100.0f, std::memory_order_relaxed);
    outputMeterDb.store (-100.0f, std::memory_order_relaxed);
    tankEnergy.store    (0.0f,    std::memory_order_relaxed);
}

//==============================================================================
/** A host's reset - a transport locate, a buffer clear - propagated to the DSP.

    **JUCE's base implementation is a no-op, and none of the six castings overrode it**, so until
    stage 1c a host asking every plugin in the session to clear itself was answered by nothing
    anywhere. Measured tails surviving a reset: Gatecrasher 0.679, Chorus-60 0.429, Reflect-84 0.111.

    Routed to the same per-stage `reset()` calls `prepareToPlay` already makes, and deliberately NOT
    to `prepareToPlay` itself: re-preparing would also re-run whatever a prepare re-arms, and this
    suite has a measured example of that being audible.
*/
void Reflect84AudioProcessor::reset()
{
    reverbEngine.reset();
}

bool Reflect84AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
void Reflect84AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (getTotalNumInputChannels(), getTotalNumOutputChannels());

    for (int ch = numChannels; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numChannels == 0 || numSamples == 0)
        return;

    // Bypassed: the dry signal passes untouched. Returning a bypass parameter from
    // getBypassParameter() makes this our job - JUCE routes the host's button to the parameter but
    // does not insert a bypassed path on our behalf.
    //
    // The tank is deliberately NOT reset here. Coming out of bypass should not start from silence
    // any more than re-patching a hardware unit empties its plates; leaving the delay lines alone
    // means the tail is already there when the signal returns.
    if (bypassParam != nullptr && bypassParam->get())
    {
        updateDisplayState (buffer, buffer, numSamples);
        return;
    }

    // Every parameter is read exactly once, here, into plain locals - no DSP stage below ever
    // touches the APVTS.
    const float mix01 = mixParam->load();
    const float trimGain = juce::Decibels::decibelsToGain (ParamFormat::trimDb (trimParam->load()));
    const float width = ParamFormat::widthPercent (widthParam->load()) * 0.01f;
    const int algorithm = juce::roundToInt (algorithmParam->load());

    TankParameters tank;
    tank.sizeScale    = ParamFormat::sizeScale (sizeParam->load());
    tank.decaySeconds = ParamFormat::decaySeconds (decayParam->load());
    tank.density01    = densityParam->load();
    tank.dampHFHz     = ParamFormat::dampHFHz (dampHFParam->load());
    tank.dampLFHz     = ParamFormat::dampLFHz (dampLFParam->load());
    tank.mod01        = modulationParam->load();
    tank.grain01      = grainParam->load();

    // **The over-delivery policy.** dryBuffer.setSize grows when a host sends more samples than it
    // declared. Chunking removes it: no span is longer than the prepared size, so the growth path
    // is never reached.
    //
    // **THE BUS QUESTION, ASKED HERE.** Gatecrasher had to move its getBusBuffer calls inside the
    // loop; Fifth Member had none to move because it has no second bus. **Reflect-84's answer is
    // Fifth Member's ANSWER for Fifth Member's REASON, and both halves were checked rather than
    // predicted from the layout:** there is no getBusBuffer call anywhere in this processBlock, and
    // its channel count comes from getTotalNumInput/OutputChannels — the processor, not the buffer —
    // which is span-invariant by construction. Plain stereo in and out, no sidechain, so the
    // extraction that would have needed moving does not exist. A casting can call getBusBuffer for
    // its main bus without having a second one, which is why the call site was looked for rather
    // than inferred from the bus count.
    //
    // ScopedNoDenormals, the unused-channel clear, the bypass path and the parameter reads all stay
    // OUTSIDE: the guard is scoped, the clear operates on the whole buffer, and bypass returns
    // before any of this is reached.
    nf::processInChunks (buffer, getBlockSize(), [&] (juce::AudioBuffer<float>& span)
    {
    const int numSamples = span.getNumSamples();

    dryBuffer.setSize (numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, span, ch, 0, numSamples);

    mixSmoothed.setTargetValue (mix01);
    trimSmoothed.setTargetValue (trimGain);
    widthSmoothed.setTargetValue (width);

    reverbEngine.process (span, algorithm, tank, ParamFormat::preDelayMs (preDelayParam->load()));

    StereoWidthStage::process (span, widthSmoothed);

    std::array<float*, 2> out {};
    std::array<const float*, 2> dry {};

    for (int ch = 0; ch < numChannels; ++ch)
    {
        out[(size_t) ch] = span.getWritePointer (ch);
        dry[(size_t) ch] = dryBuffer.getReadPointer (ch);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        // getNextValue() advances the smoother, so it is called once per sample and shared across
        // channels - not once per channel, which would advance it N times per sample.
        const float wetAmount = mixSmoothed.getNextValue();
        const float gain = trimSmoothed.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float wet = out[(size_t) ch][i];
            out[(size_t) ch][i] = (dry[(size_t) ch][i] * (1.0f - wetAmount) + wet * wetAmount) * gain;
        }
    }

    updateDisplayState (dryBuffer, span, numSamples);
    });
}

void Reflect84AudioProcessor::updateDisplayState (const juce::AudioBuffer<float>& dry,
                                                  const juce::AudioBuffer<float>& out,
                                                  int numSamples)
{
    // Ballistics derived from a real time constant and the actual block length, so the meters
    // behave identically whatever buffer size the host hands us. Gatecrasher's per-block 0.5/0.12
    // constants silently change meaning when the host's buffer size changes; TapeRot's
    // block-rate-corrected form is the one worth carrying forward.
    constexpr float releaseTimeConstantSeconds = 0.15f;

    const float blockSeconds = (float) numSamples / (float) displaySampleRate;
    const float releaseCoeff = 1.0f - std::exp (-blockSeconds / releaseTimeConstantSeconds);

    const auto peakDb = [numSamples] (const juce::AudioBuffer<float>& b)
    {
        float peak = 0.0f;

        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const auto* data = b.getReadPointer (ch);

            for (int i = 0; i < numSamples; ++i)
                peak = juce::jmax (peak, std::abs (data[i]));
        }

        return juce::Decibels::gainToDecibels (peak, -100.0f);
    };

    const auto smoothDb = [releaseCoeff] (std::atomic<float>& target, float measured)
    {
        const float current = target.load (std::memory_order_relaxed);

        // Instant attack, time-constant release - a peak meter should catch the transient it is
        // there to report and then fall back at a readable rate.
        target.store (measured > current ? measured
                                         : current + releaseCoeff * (measured - current),
                      std::memory_order_relaxed);
    };

    smoothDb (inputMeterDb,  peakDb (dry));
    smoothDb (outputMeterDb, peakDb (out));

    // Read from the tank itself rather than the output, so the lamp keeps reporting through the
    // tail after the input stops - and at Mix 0, where the tank is still running but the output
    // is dry by definition.
    const float current = tankEnergy.load (std::memory_order_relaxed);
    const float measured = reverbEngine.getEnergy();

    tankEnergy.store (measured > current ? measured
                                         : current + releaseCoeff * (measured - current),
                      std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor* Reflect84AudioProcessor::createEditor()
{
    return new Reflect84AudioProcessorEditor (*this);
}

//==============================================================================
void Reflect84AudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, programManager.getNumPrograms()))
        return;

    // The stale-replay guard, disarmed by this call whether or not it is honoured.
    if (userEdits.consumeRestore() && index == getCurrentProgram())
        return;

    programManager.requestProgramChange (ProgramManager::factoryIdAt (index));
}

void Reflect84AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    xml->setAttribute (LegacyMigration::stateSchemaVersionAttribute,
                       LegacyMigration::currentStateSchemaVersion);

    // Sticky display metadata only, restored clamped below and never re-validated against the
    // session's actual knob values: a session saved after tweaking a loaded Program still
    // remembers which Program it was tweaked from, even though it was never itself saved.
    // **The bank, the identifier, and the full parameter state.** The values make the session
    // sound right; the identity only decides what the panel CALLS them.
    const auto id = programManager.getCurrentProgramId();
    xml->setAttribute (LegacyMigration::programBankAttribute, LegacyMigration::bankAttributeValue (id.bank));
    xml->setAttribute (LegacyMigration::programIdAttribute, id.id);
    xml->setAttribute (LegacyMigration::programNameAttribute, id.displayName);

    copyXmlToBinary (*xml, destData);
}

void Reflect84AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    // The schema version is read, not merely written. CHORUS-60 records what happens otherwise:
    // restoring an older layout leaves surviving parameter IDs at their saved values while new
    // ones fall back to defaults, producing a silent hybrid nothing reports as a problem.
    const int savedSchema = xml->getIntAttribute (LegacyMigration::stateSchemaVersionAttribute, 1);

    // **Two branches, both pinned to literals, because they are different situations.** Too old:
    // the values genuinely cannot be interpreted. Too new: written by a later build, and reading it
    // with today's assumptions would give plausible wrong values rather than an obvious fallback.
    //
    // This replaced `savedSchema != currentStateSchemaVersion`, which was correct exactly once -
    // this very bump would otherwise have discarded every existing session over a change that
    // alters no parameter's meaning.
    if (LegacyMigration::classifySchema (savedSchema) != LegacyMigration::SchemaVerdict::readable)
    {
        programManager.cancelPendingChange();
        programManager.requestProgramChange (ProgramManager::factoryIdAt (defaultFactoryProgramIndex));
        return;
    }

    // Essential, and easy to miss: a program change requested just before the restore would
    // otherwise be applied just after it and overwrite everything that was restored.
    programManager.cancelPendingChange();

    apvts.replaceState (juce::ValueTree::fromXml (*xml));

    ProgramId restored;

    if (savedSchema >= LegacyMigration::identitySchemaVersion)
    {
        restored = programManager.resolve (
            LegacyMigration::bankFromAttribute (
                xml->getStringAttribute (LegacyMigration::programBankAttribute)),
            xml->getStringAttribute (LegacyMigration::programIdAttribute),
            xml->getStringAttribute (LegacyMigration::programNameAttribute));
    }
    else
    {
        // v1 stored a position. Map it through the CURRENT bank - correct because nothing has
        // shipped and the bank has not moved.
        const int savedIndex = xml->getIntAttribute (LegacyMigration::currentProgramIndexAttribute,
                                                      defaultFactoryProgramIndex);

        if (savedIndex == -1)
            restored = ProgramManager::initId();
        else if (juce::isPositiveAndBelow (savedIndex, kNumFactoryPrograms))
            restored = ProgramManager::factoryIdAt (savedIndex);
        else
            restored = ProgramManager::factoryIdAt (defaultFactoryProgramIndex);
    }

    programManager.setCurrentProgramWithoutApplying (restored);

    // **Armed AFTER replaceState**, or the restore's own writes would disarm it.
    userEdits.armRestore();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Reflect84AudioProcessor();
}
