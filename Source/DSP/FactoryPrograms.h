#pragma once

#include <juce_core/juce_core.h>

#include <array>

/**
    REFLECT-84's factory bank.

    A flat POD of plain ints and floats rather than the DSP layer's own types, so this header stays
    decoupled from the tank classes and can be compiled into the test console app on its own.
    ProgramManager maps `algorithm` onto the APVTS choice parameter.

    Every field is 0-1 normalised except `algorithm`, matching Parameters.h's state model - so a
    program is literally a snapshot of the eleven knob positions plus the switch.

    STATUS: names and values are both authored, transcribed from the approved bank table. The
    values are still a by-design pass rather than a by-ear one - the four tank topologies do not
    exist yet, so nothing has been listened to - but they are deliberate per-Program choices, not
    placeholders.

    The table was authored in panel units (2.4 s, 20 ms, 9 kHz, 150 Hz, 75%); what is stored here
    is the 0-1 normalised form, because that is the state model Parameters.h uses. Every mapping is
    affine, so the conversion is exact and reversible - the numbers below are what the panel will
    read back. Two conversions worth spelling out:

      - Size is stored as knob travel, not as its readout: the readout runs 0.20-1.00, so an
        authored "55%" is a readout of 0.55 and a stored 0.4375.
      - Width's readout runs 0-200%, so an authored "100%" is unity and a stored 0.5.

    ONE VALUE WAS CLAMPED: Heaven asks for a 9.0 s decay, but DECAY's range is 0.4-8.0 s
    (design/README.md section 5), so it is stored at 1.0 = 8.0 s - the longest tail the control
    can express. Everything else is transcribed exactly.
*/
/** Which list a Program belongs to. INIT is its own bank rather than a magic index; `unresolved`
    is a stored identifier that no longer names anything. */
enum class ProgramBank
{
    init,
    factory,
    user,
    unresolved
};

/** **How a Program is identified everywhere except the host adapter.** Not a position - positions
    change when the bank is reordered or extended, so a stored position is a name that stops meaning
    the same thing.

    `displayName` is carried because a factory slug is not presentable: "rain-all-day?" in the LCD
    would read as a rendering fault. It is display only and never resolves anything. */
struct ProgramId
{
    ProgramBank bank = ProgramBank::factory;
    juce::String id;
    juce::String displayName;

    bool operator== (const ProgramId& o) const noexcept { return bank == o.bank && id == o.id; }
    bool operator!= (const ProgramId& o) const noexcept { return ! operator== (o); }
};

struct FactoryProgram
{
    /** **The permanent identity, fixed at creation and never changed again.** `name` is a label the
        designers may revise; `slug` may not be, because it is what a saved session stores. */
    const char* slug;

    const char* name;

    int algorithm;          // 0 = Plate, 1 = Digital Room, 2 = Chamber, 3 = Hall

    // REVERB TANK
    float size;
    float decay;
    float preDelay;
    float density;

    // DAMPING
    float dampHF;
    float dampLF;

    // CHARACTER
    float modulation;
    float grain;

    // OUTPUT
    float width;
    float mix;
    float trim;
};

// **The two damping columns were re-derived when the taper went log, and no longer match the
// pattern the other columns follow.** Everything here is stored normalised, but this bank was
// authored in PANEL UNITS - the comment above each row is the authored form and is the source of
// truth. Under the old affine mapping a normalised 0.5 meant 9 kHz; under `2000 * 8^n` it means
// 5.66 kHz. Re-pointing the curve without re-deriving would have moved every Program: damping LF
// roughly halved across the whole bank (150 Hz -> 73, 400 Hz -> 289) and HF fell 30-40%.
//
// So each row's dampHF/dampLF now carry `log (hz / min) / log (max / min)` for the authored hertz,
// which keeps all twelve sounding as authored. FactoryProgramsTests asserts that round-trip, so a
// future taper change cannot silently move the bank again.
//
// ParamDefaults deliberately did NOT get the same treatment - see the note there. The defaults were
// authored as normalised positions, this bank was authored in hertz, and each keeps its own
// authoring intent.
inline constexpr std::array<FactoryProgram, 12> kFactoryPrograms { {
    //  Authored as:      algo           size  decay  preDly densty dampHF dampLF  mod  grain width  mix   trim
    //  01 Rain All Day   Plate           55%  2.4 s  20 ms   75%   9 kHz  150 Hz  20%   0%   100%   35%  0 dB
    { "rain-all-day", "RAIN ALL DAY",      0, 0.4375f, 0.2632f, 0.1111f, 0.7500f, 0.7233f, 0.5233f, 0.2000f, 0.0000f, 0.5000f, 0.3500f, 0.5000f },
    //  02 So Long        Digital Room    80%  6.5 s  35 ms   65%   6 kHz  200 Hz  30%  15%   100%   40%  0 dB
    { "so-long", "SO LONG",           1, 0.7500f, 0.8026f, 0.1944f, 0.6500f, 0.5283f, 0.6372f, 0.3000f, 0.1500f, 0.5000f, 0.4000f, 0.5000f },
    //  03 Quiet Violence Chamber         65%  3.2 s  15 ms   80%   5 kHz  300 Hz  15%  35%    90%   30%  0 dB
    { "quiet-violence", "QUIET VIOLENCE",    2, 0.5625f, 0.3684f, 0.0833f, 0.8000f, 0.4406f, 0.7978f, 0.1500f, 0.3500f, 0.4500f, 0.3000f, 0.5000f },
    //  04 Sail Away      Hall            95%  8.0 s  40 ms   60%  12 kHz  100 Hz  40%   0%   100%   45%  0 dB
    { "sail-away", "SAIL AWAY",         3, 0.9375f, 1.0000f, 0.2222f, 0.6000f, 0.8617f, 0.3628f, 0.4000f, 0.0000f, 0.5000f, 0.4500f, 0.5000f },
    //  05 On the Moon    Hall           100%  5.5 s  60 ms   40%  10 kHz   80 Hz  25%  10%   100%   35%  0 dB
    { "on-the-moon", "ON THE MOON",       3, 1.0000f, 0.6711f, 0.3333f, 0.4000f, 0.7740f, 0.2744f, 0.2500f, 0.1000f, 0.5000f, 0.3500f, 0.5000f },
    //  06 A Prayer       Chamber         85%  4.5 s  25 ms   85%   8 kHz  120 Hz  20%   0%    95%   40%  0 dB
    { "a-prayer", "A PRAYER",          2, 0.8125f, 0.5395f, 0.1389f, 0.8500f, 0.6667f, 0.4350f, 0.2000f, 0.0000f, 0.4750f, 0.4000f, 0.5000f },
    //  07 Brothers       Digital Room    60%  3.0 s  20 ms   70%   7 kHz  180 Hz  25%  25%    90%   35%  0 dB
    { "brothers", "BROTHERS",          1, 0.5000f, 0.3421f, 0.1111f, 0.7000f, 0.6025f, 0.5955f, 0.2500f, 0.2500f, 0.4500f, 0.3500f, 0.5000f },
    //  08 Heaven         Hall           100%  9.0 s* 30 ms   55%  11 kHz   90 Hz  50%   0%   100%   50%  0 dB   (*clamped to 8.0 s)
    { "heaven", "HEAVEN",            3, 1.0000f, 1.0000f, 0.1667f, 0.5500f, 0.8198f, 0.3211f, 0.5000f, 0.0000f, 0.5000f, 0.5000f, 0.5000f },
    //  09 Cold Atmosph.  Digital Room    50%  2.8 s  10 ms   60%   4 kHz  400 Hz  10%  45%    70%   30%  0 dB
    { "cold-atmosphere", "COLD ATMOSPHERE",   1, 0.3750f, 0.3158f, 0.0556f, 0.6000f, 0.3333f, 0.9117f, 0.1000f, 0.4500f, 0.3500f, 0.3000f, 0.5000f },
    //  10 The Moors      Hall            90%  6.0 s  45 ms   45%   9 kHz  110 Hz  35%   5%   100%   40%  0 dB
    { "the-moors", "THE MOORS",         3, 0.8750f, 0.7368f, 0.2500f, 0.4500f, 0.7233f, 0.4005f, 0.3500f, 0.0500f, 0.5000f, 0.4000f, 0.5000f },
    //  11 Second Nature  Chamber         70%  3.8 s  18 ms   75%   8 kHz  160 Hz  30%   8%    95%   38%  0 dB
    { "second-nature", "SECOND NATURE",     2, 0.6250f, 0.4474f, 0.1000f, 0.7500f, 0.6667f, 0.5489f, 0.3000f, 0.0800f, 0.4750f, 0.3800f, 0.5000f },
    //  12 World Gone Mad Plate           75%  3.5 s  22 ms   80%  10 kHz  140 Hz  20%  20%   100%   42%  0 dB
    { "world-gone-mad", "WORLD GONE MAD",    0, 0.6875f, 0.4079f, 0.1222f, 0.8000f, 0.7740f, 0.4960f, 0.2000f, 0.2000f, 0.5000f, 0.4200f, 0.5000f },
} };

inline constexpr int kNumFactoryPrograms = (int) kFactoryPrograms.size();

/** INIT's index. **-1, deliberately outside the bank rather than position 0 within it.**

    INIT is the blank canvas you start from, not an authored sound competing with the twelve, so
    numbering it would push RAIN ALL DAY to 02 and imply a running order it is not part of. Keeping
    it outside also means it never renumbers anything, so no saved session needs migrating.

    -1 is therefore a meaningful index here, and every "no index" sentinel in this casting has to be
    something else - see ProgramManager's pending-apply sentinel, which is -2. */
inline constexpr int initProgramIndex = -1;

/** The blank canvas: the tank present and audible in its plainest form, with everything that gives
    REFLECT-84 its character at zero.

    Three rules decide every value, and they are not the same rule:
      - **Character and amount go to zero** - Modulation and Digital Grain. Raise either and you
        immediately hear what it does.
      - **Structure goes to a usable middle, never zero** - Size, Decay and Density at 0.5. A reverb
        at zero decay is not neutral, it is broken: there would be nothing to hear at all, and the
        first knob raised would appear to do nothing.
      - **Anything meaning "not acting" takes whatever value that is** - Pre-Delay 0 ms, Width 100 %
        (the neutral point of a 0-200 % range), Trim 0 dB, and both damping filters wide open.

    **Every value here is a normalised 0-1 position**, as everywhere in this casting, so the ones
    that are not obvious are worked below rather than left to be re-derived:
      - Decay 0.5 -> `0.4 + 0.5 x 7.6` = **4.2 s**, the middle of the range.
      - Damping HF 1.0 -> `2000 x 8^1` = **16 kHz**, the TOP of the range and therefore the least
        damping. It is 1.0 and not 0.0 precisely because "wide open" is the high end here.
      - Damping LF 0.0 -> `40 x 12.5^0` = **40 Hz**, the bottom of its range and likewise the least.
        The two open in opposite directions, which is exactly the sort of thing that gets inverted.
      - Width 0.5 -> `0.5 x 200` = **100 %**, unmodified stereo rather than half of anything.
      - Trim 0.5 -> `(0.5 - 0.5) x 24` = **0 dB**.

    **Mix is 50 %.** REFLECT-84 is a wet/dry effect, and the midpoint reads as "nothing decided yet"
    where a value like 35 % would look like a judgement someone made. The two serial castings,
    TapeRot and Elmer, sit at 100 % for the opposite reason. */
inline constexpr FactoryProgram kInitProgram
    //                 alg  size     decay    preDly   dens     dampHF   dampLF   mod      grain    width    mix      trim
    { "init", "INIT",          0,   0.5000f, 0.5000f, 0.0000f, 0.5000f, 1.0000f, 0.0000f, 0.0000f, 0.0000f, 0.5000f, 0.5000f, 0.5000f };

/** Loaded on first launch and whenever no saved session state exists. design/screenshots/01-panel.png
    shows "01 RAIN ALL DAY" in the display, so that is what the plugin opens on. */
inline constexpr int defaultFactoryProgramIndex = 0;
