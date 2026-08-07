#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>
#include <vector>

/**
    Single source of truth for REFLECT-84's parameter IDs, APVTS layout, and value formatting.

    Every continuous parameter is stored 0-1 normalised, matching design/README.md's own state
    model ("vals: 11 normalized floats ... all 0-1"). That is not the shape TapeRot and Gatecrasher
    use - both store physical values behind a NormalisableRange - but here every one of the design
    doc's parameter mappings is affine, so the two are mathematically interchangeable and the
    normalised form keeps the mappings in exactly one place. The cost, that a host's automation
    lane would otherwise read "0.22" rather than "40 ms", is paid back by giving each parameter a
    stringFromValueFunction built from the same ParamFormat helpers the panel readouts use.
*/
namespace ParamIDs
{
    // REVERB TANK
    inline constexpr auto size      = "size";
    inline constexpr auto decay     = "decay";
    inline constexpr auto preDelay  = "preDelay";
    inline constexpr auto density   = "density";

    // DAMPING
    inline constexpr auto dampHF    = "dampHF";
    inline constexpr auto dampLF    = "dampLF";

    // CHARACTER
    inline constexpr auto modulation = "modulation";
    inline constexpr auto grain      = "grain";

    // OUTPUT
    inline constexpr auto width     = "width";
    inline constexpr auto mix       = "mix";
    inline constexpr auto trim      = "trim";

    // ALGORITHM
    inline constexpr auto algorithm = "algorithm";
}

/**
    The four reverb algorithms, in DSP-canonical order.

    design/README.md section 2 warns that the panel's corner placement is NOT clockwise-sequential -
    HALL sits bottom-left and is index 3, CHAMBER bottom-right and index 2. The visual arrangement
    is fixed by the design; this enum is fixed by the DSP. AlgorithmSwitch owns the corner-to-index
    table that joins them, and neither is ever derived from the other.
*/
enum class Algorithm
{
    plate = 0,
    digitalRoom = 1,
    chamber = 2,
    hall = 3
};

inline constexpr int numAlgorithms = 4;

namespace ParamFormat
{
    // Every mapping below is transcribed from design/README.md section 5's table, which is
    // authoritative for both the knob readouts and the host-facing parameter text. They live here
    // rather than in the GUI so the two can never disagree.

    inline float sizeScale     (float v) noexcept { return 0.2f + v * 0.8f; }        // 0.20 .. 1.00
    inline float decaySeconds  (float v) noexcept { return 0.4f + v * 7.6f; }        // 0.4 .. 8.0 s
    inline float preDelayMs    (float v) noexcept { return v * 180.0f; }             // 0 .. 180 ms
    inline float densityPercent(float v) noexcept { return v * 100.0f; }             // 0 .. 100 %
    inline float dampHFHz      (float v) noexcept { return (2.0f + v * 14.0f) * 1000.0f; }  // 2 .. 16 kHz
    inline float dampLFHz      (float v) noexcept { return 40.0f + v * 460.0f; }     // 40 .. 500 Hz
    inline float modPercent    (float v) noexcept { return v * 100.0f; }             // 0 .. 100 %
    inline float widthPercent  (float v) noexcept { return v * 200.0f; }             // 0 .. 200 %
    inline float mixPercent    (float v) noexcept { return v * 100.0f; }             // 0 .. 100 %
    inline float trimDb        (float v) noexcept { return (v - 0.5f) * 24.0f; }     // -12 .. +12 dB

    inline juce::String sizeText    (float v) { return juce::String (sizeScale (v), 2); }
    inline juce::String decayText   (float v) { return juce::String (decaySeconds (v), 1) + " s"; }
    inline juce::String preDelayText(float v) { return juce::String (juce::roundToInt (preDelayMs (v))) + " ms"; }
    inline juce::String densityText (float v) { return juce::String (juce::roundToInt (densityPercent (v))) + "%"; }
    inline juce::String dampHFText  (float v) { return juce::String (dampHFHz (v) * 0.001f, 1) + " k"; }
    inline juce::String dampLFText  (float v) { return juce::String (juce::roundToInt (dampLFHz (v))) + " Hz"; }
    inline juce::String modText     (float v) { return juce::String (juce::roundToInt (modPercent (v))) + "%"; }
    inline juce::String grainText   (float v) { return juce::String (juce::roundToInt (v * 100.0f)); }
    inline juce::String widthText   (float v) { return juce::String (juce::roundToInt (widthPercent (v))) + "%"; }
    inline juce::String mixText     (float v) { return juce::String (juce::roundToInt (mixPercent (v))) + "%"; }
    inline juce::String trimText    (float v) { return juce::String (trimDb (v), 1); }
}

namespace ParamDefaults
{
    // design/README.md section 5's "Default (0-1)" column, verbatim.
    inline constexpr float size       = 0.64f;
    inline constexpr float decay      = 0.58f;
    inline constexpr float preDelay   = 0.22f;
    inline constexpr float density    = 0.72f;
    inline constexpr float dampHF     = 0.42f;
    inline constexpr float dampLF     = 0.28f;
    inline constexpr float modulation = 0.34f;
    inline constexpr float grain      = 0.46f;
    inline constexpr float width      = 0.70f;
    inline constexpr float mix        = 0.55f;
    inline constexpr float trim       = 0.50f;
}

namespace LegacyMigration
{
    /** Written into the state XML root by getStateInformation and checked on restore.
        Deliberately separate from juce::ParameterID's versionHint, which only affects the
        host's numeric automation-lane ID - APVTS's own XML is keyed by plain ID string
        regardless, so a schema change needs its own marker. */
    inline constexpr auto stateSchemaVersionAttribute = "reflect84StateSchemaVersion";
    inline constexpr int currentStateSchemaVersion = 1;

    /** Sticky display metadata: which Program the session was last on. */
    inline constexpr auto currentProgramIndexAttribute = "reflect84CurrentProgramIndex";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createReflect84ParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // All eleven continuous parameters share the same plain 0-1 range; only their display text
    // differs. The lambda-per-parameter keeps the host's readout in the panel's own units.
    const auto addNormalised = [&params] (const char* id,
                                          const juce::String& name,
                                          float defaultValue,
                                          juce::String (*format) (float))
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 },
            name,
            juce::NormalisableRange<float> (0.0f, 1.0f),
            defaultValue,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([format] (float v, int) { return format (v); })));
    };

    addNormalised (ParamIDs::size,       "Size",          ParamDefaults::size,       ParamFormat::sizeText);
    addNormalised (ParamIDs::decay,      "Decay",         ParamDefaults::decay,      ParamFormat::decayText);
    addNormalised (ParamIDs::preDelay,   "Pre-Delay",     ParamDefaults::preDelay,   ParamFormat::preDelayText);
    addNormalised (ParamIDs::density,    "Density",       ParamDefaults::density,    ParamFormat::densityText);
    addNormalised (ParamIDs::dampHF,     "Damping HF",    ParamDefaults::dampHF,     ParamFormat::dampHFText);
    addNormalised (ParamIDs::dampLF,     "Damping LF",    ParamDefaults::dampLF,     ParamFormat::dampLFText);
    addNormalised (ParamIDs::modulation, "Modulation",    ParamDefaults::modulation, ParamFormat::modText);
    addNormalised (ParamIDs::grain,      "Digital Grain", ParamDefaults::grain,      ParamFormat::grainText);
    addNormalised (ParamIDs::width,      "Stereo Width",  ParamDefaults::width,      ParamFormat::widthText);
    addNormalised (ParamIDs::mix,        "Mix",           ParamDefaults::mix,        ParamFormat::mixText);
    addNormalised (ParamIDs::trim,       "Output Trim",   ParamDefaults::trim,       ParamFormat::trimText);

    // A detented 4-position switch, not a continuous control - see the Algorithm enum's comment
    // on why the panel's corner order and this order deliberately differ.
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::algorithm, 1 },
        "Algorithm",
        juce::StringArray { "Plate", "Digital Room", "Chamber", "Hall" },
        (int) Algorithm::plate));

    // New parameters are APPENDED below this line, never inserted above it - saved Programs and
    // host automation lanes are keyed by position as well as ID.

    return { params.begin(), params.end() };
}
