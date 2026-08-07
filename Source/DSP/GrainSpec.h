#pragma once

#include <juce_core/juce_core.h>

#include <cmath>

/**
    The single interpretation of the DIGITAL GRAIN parameter, shared by the audio path and the
    TANK LIVE scope.

    design/README.md is explicit that the scope's stair-stepping is meant to *demonstrate what the
    knob is doing to the audio*, not merely resemble it - so there must not be two separate
    readings of the grain value that can drift apart. This header is the only place the parameter
    is interpreted; GrainStage (audio) and TankScope (GUI) both call fromNormalized() and derive
    everything from the same struct.

    Two quantities come straight out of the design doc's drawing model and are reproduced verbatim,
    because the scope's rendering is specified against them exactly:

      levels = max(3, round(30 - g*26))     amplitude steps
      stepPx = 3 + g*24                     x-axis step, in scope pixels

    The audio side derives from those same two numbers:

      - `levels` scaled by kHeadroom is the audio quantizer's step count over unit amplitude.
        The scaling exists because `levels` alone spans 29 down to 4 steps of FULL SCALE, which
        as an audio word length is roughly 5.9 down to 3 bits - unusable as anything but an
        effect. kHeadroom shifts the whole range up to something musical without changing its
        shape; the SPAN is fixed at log2(29/4) = 2.86 bits no matter what kHeadroom is, and
        kHeadroom only chooses where that span sits. At 48 it sits at ~11.4 bits when grain first
        engages and ~8.6 bits at full travel.

      - `stepPx` divided by its own floor (3 px) gives holdRatio, a dimensionless 1.0-9.0. It
        means the same thing in both domains: pixels-per-step on the scope, samples-per-hold in
        the tank. At full grain that decimates the tank's internal update rate 9:1 - 48 kHz down
        to ~5.3 kHz - with no anti-alias filter, so the fold-back is part of the sound.

    GrainStage applies this INSIDE the tank's feedback path rather than on the output, so the
    truncation compounds once per recirculation and the decay envelope itself breaks into steps.
    That is what makes the scope's picture something the audio path actually produces.
*/
struct GrainSpec
{
    /** Below this the stage is bypassed entirely and the scope draws a smooth curve.
        design/README.md section 6: "when grain > 0.03". */
    static constexpr float kThreshold = 0.03f;

    /** The scope's x-axis step when grain is off, and the divisor that turns stepPx into the
        dimensionless holdRatio. design/README.md section 6: "At grain 0 the curve is smooth with
        a 3px step." */
    static constexpr float kSmoothStepPx = 3.0f;

    /** Chooses where the fixed 2.86-bit span of `levels` sits as an audio word length. See the
        class comment - this is the one free constant in the mapping, and it is named rather than
        buried in GrainStage precisely so it stays reviewable. */
    static constexpr float kHeadroom = 48.0f;

    /** The parameter value this was built from, kept so the readout can print it. */
    float normalized = 0.0f;

    /** False below kThreshold: no quantization, no decimation, smooth scope trace. */
    bool active = false;

    /** Amplitude steps. Zero when inactive - callers treat that as "do not quantize". */
    int levels = 0;

    /** Scope x-axis step in the design doc's 600 x 168 viewBox. Never below kSmoothStepPx. */
    float stepPx = kSmoothStepPx;

    /** stepPx / kSmoothStepPx, so 1.0 (no decimation) up to 9.0 at full travel. */
    float holdRatio = 1.0f;

    static GrainSpec fromNormalized (float g) noexcept
    {
        GrainSpec spec;
        spec.normalized = juce::jlimit (0.0f, 1.0f, g);
        spec.active     = spec.normalized > kThreshold;
        spec.levels     = spec.active ? juce::jmax (3, juce::roundToInt (30.0f - spec.normalized * 26.0f)) : 0;
        spec.stepPx     = spec.active ? kSmoothStepPx + spec.normalized * 24.0f : kSmoothStepPx;
        spec.holdRatio  = spec.stepPx / kSmoothStepPx;
        return spec;
    }

    /** Audio-domain quantizer step over unit amplitude. Zero when inactive. */
    float quantStep() const noexcept
    {
        return active ? 1.0f / ((float) levels * kHeadroom) : 0.0f;
    }

    /** Equivalent signed word length, for tests and documentation. An N-bit signed quantizer
        spans [-1, 1) in 2^N steps, so step = 2^(1-N) and N = 1 + log2(levels * kHeadroom). */
    float bits() const noexcept
    {
        return active ? 1.0f + std::log2 ((float) levels * kHeadroom) : 32.0f;
    }

    /** Sample-and-hold period for the tank's internal update rate: 1 (no decimation) to 9. */
    int holdSamples() const noexcept
    {
        return juce::jmax (1, juce::roundToInt (holdRatio));
    }

    /** Truncating quantizer - deliberately trunc(), not round(), and with no dither. Rounding
        and dithering are both later inventions; the artefact being modelled is a fixed-point
        machine dropping low bits on every pass through its tank. */
    float quantize (float x) const noexcept
    {
        if (! active)
            return x;

        const float step = quantStep();
        return std::trunc (x / step) * step;
    }

    /** The scope's amplitude quantization, on a 0-1 envelope rather than a bipolar sample.
        design/README.md section 6: "quantize env to levels steps". Rounding here, not truncation:
        the doc's own drawing model rounds, and an envelope is a magnitude, not a waveform. */
    float quantizeEnvelope (float env) const noexcept
    {
        if (levels <= 0)
            return env;

        return (float) juce::roundToInt (env * (float) levels) / (float) levels;
    }

    /** The scope's bottom-left legend. design/README.md section 6.

        The separator is U+00B7 MIDDLE DOT, built from its codepoint rather than written as a
        literal or a \x escape: both depend on the source encoding surviving every toolchain, and
        a mis-decoded one renders as a stray "A-circumflex" on the panel. */
    juce::String describe() const
    {
        const auto dot = juce::String::charToString ((juce::juce_wchar) 0x00B7);

        if (! active)
            return "GRAIN OFF " + dot + " SMOOTH";

        return "GRAIN " + juce::String (juce::roundToInt (normalized * 100.0f))
             + " " + dot + " " + juce::String (levels) + " STEP";
    }
};
