# REFLECT-84

A merged plate / digital-rack reverb, built with JUCE 8 for macOS (AU, VST3, Standalone), Windows
(VST3, Standalone) and Linux (VST3, Standalone). It is the fourth casting under the
[Neon Foundry](../BRAND.md) umbrella, sibling to [TapeRot](../taperot), [Gatecrasher](../gatecrasher)
and [CHORUS-60](../chorus-60), and shares their DNA without depending on any of them.

The panel is a 1980s cream-fascia digital rack unit with a recessed dark-blue control bezel: an
engraved metal wordmark, brass knobs, an LCD Program display, and one phosphor-green oscilloscope.

Four genuinely different reverb networks sit behind the ALGORITHM switch — a Dattorro figure-of-eight
plate, a coarse whole-sample Digital Room, a Householder FDN chamber, and an eight-line Hadamard FDN
hall. They differ in topology, not just in delay lengths.

The signature control is **DIGITAL GRAIN**, which re-quantizes the signal *inside* the reverb tank's
feedback path rather than on its output. Because the truncation compounds once per recirculation, the
decay envelope itself breaks into audible steps as the knob comes up — and the TANK LIVE scope draws
those same steps, from the same numbers the audio path uses. The display is an instrument, not an
illustration.

## Parameters

| Parameter | Range | Default | Notes |
|---|---|---|---|
| Algorithm | Plate / Digital Room / Chamber / Hall | Plate | 4-position detented switch |
| Size | 0.20 – 1.00 | 0.71 | Scales every delay length in the active topology |
| Decay | 0.4 – 8.0 s | 4.8 s | RT60 |
| Pre-Delay | 0 – 180 ms | 40 ms | Ahead of diffusion |
| Density | 0 – 100 % | 72 % | Diffusion, plus early-reflection tap density |
| Damping HF | 2 – 16 kHz | 7.9 k | One-pole low-pass **inside** the feedback loop |
| Damping LF | 40 – 500 Hz | 169 Hz | One-pole high-pass inside the feedback loop |
| Modulation | 0 – 100 % | 34 % | Tank LFO depth; stops long tails ringing metallically |
| Digital Grain | 0 – 100 | 46 | In-loop requantize + sample-and-hold, ~11.4 → ~8.6 bits and 1× → 9× |
| Width | 0 – 200 % | 140 % | M/S on the wet path; transparent at 100 % |
| Mix | 0 – 100 % | 55 % | Dry/wet |
| Output Trim | ±12 dB | 0.0 | |

All eleven continuous parameters are stored 0-1 normalised, matching the design's own state model;
each carries a display function so hosts still show real units.

## Programs

Twelve factory Programs — Rain All Day, So Long, Quiet Violence, Sail Away, On the Moon, A Prayer,
Brothers, Heaven, Cold Atmosphere, The Moors, Second Nature, World Gone Mad. Click the LCD for the
menu. SAVE is disabled until something actually moves, DELETE is disabled for factory Programs, and
saving always creates a new Program rather than overwriting one — including when you reuse a name.

They are called Programs, never Presets, per [BRAND.md](../BRAND.md).

## Building

See [BUILDING.md](BUILDING.md). In short:

```sh
cmake -B build -G Xcode          # macOS; -A x64 on Windows; -DCMAKE_BUILD_TYPE=Release on Linux
cmake --build build --config Release
./build/Tests/Reflect84Tests_artefacts/Release/Reflect84Tests
```

JUCE is fetched automatically — no local checkout needed.

## Project layout

```
Source/
  Parameters.h            Parameter IDs, APVTS layout, and every value-to-text mapping
  PluginProcessor.*       Signal chain, metering, state
  PluginEditor.*          Fixed-canvas scaling shell
  DSP/
    GrainSpec.h           The single interpretation of DIGITAL GRAIN, shared with the GUI
    GrainStage.h          In-loop requantizer + sample-and-hold
    ReverbPrimitives.h    Delays, allpasses, damped combs, one-poles, LFO bank, early reflections
    ReverbTank.h          The algorithm interface
    PlateTank.*           Dattorro figure-of-eight
    DigitalRoomTank.*     Whole-sample early taps into a Moorer/Schroeder comb bank
    FdnTank.*             Feedback delay network; ChamberTank (4, Householder), HallTank (8, Hadamard)
    ReverbEngine.*        Owns all four tanks, pre-delay, and the switch crossfade
    StereoWidthStage.h    M/S width
    ProgramManager.*      Factory and user banks
    FactoryPrograms.h     The twelve Programs
  GUI/
    ReflectTheme.h        Every colour, size, position and typographic constant
    ReflectLookAndFeel.*  The knob grammar, in four size variants
    PanelBackground.*     Fascia, texture, bezel, wordmark, dividers, pills, static labels
    PanelReadouts.*       Live numeric values and the IN/OUT meters
    AlgorithmSwitch.*     The 4-position detented rotary and its corner labels
    TankScope.*           The TANK LIVE lamp and decay scope
    ProgramHeader.*       LCD, badge, menu, SAVE/DELETE, inline naming
    ReflectEditorContent.*
Tests/                    JUCE UnitTest console app
design/                   The approved GUI spec, artwork, prototype and embedded fonts
  icon/                   Product icon — a hand-tuned optical ramp, three cuts rather than one
                          artwork scaled; JUCE builds the .icns from the 1024 and 256 px versions
```

## Status

See [CLAUDE.md](CLAUDE.md). Everything is implemented; the tank tunings and the Program bank have
not had a by-ear pass yet.
