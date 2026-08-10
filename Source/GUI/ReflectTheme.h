#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <BinaryData.h>

#include <array>

/**
    REFLECT-84's design tokens: every colour, size, position and typographic constant the panel
    uses. Components pull from here rather than carrying their own numbers.

    Values come from design/README.md's token tables and layout section. That document expresses
    layout as CSS flexbox, which cannot be transcribed directly - the house model (TapeRot's
    TapeRotTheme.h) is absolute-coordinate painting against a fixed reference canvas, with no
    resized() anywhere. So the flexbox was resolved to absolute pixels and then verified against
    design/screenshots/01-panel.png, which is the approved artwork: the section dividers, the scope
    screen rect, and every knob centre below were read off that image and agree with the CSS to
    within a pixel.

    Two things worth knowing before changing anything here:

    - The canvas is 1200 x 615, not the "1200 x ~530" design/README.md's prose estimates. The
      screenshot is 2400 x 1230 at 2x, and the panel's height is content-driven rather than
      declared, so the rendered artwork is authoritative and the prose figure is prefixed "~".
      BRAND.md's 960x400 reference explicitly allows a denser control set to scale.

    - The design's inner elements are CSS content-box, so a declared `width: 336px` plus
      `padding-right: 22px` occupies 358px. Getting that wrong moves every column.
*/
namespace ReflectTheme
{

//==============================================================================
namespace Colour
{
    // --- Fascia / body -------------------------------------------------------
    inline const juce::Colour fasciaTop        { 0xFFEFE6D0 };
    inline const juce::Colour fasciaMid        { 0xFFE2D8BD };   // at 60%
    inline const juce::Colour fasciaBottom     { 0xFFD8CDB0 };

    inline const juce::Colour engravedLine     { juce::Colour::fromRGBA (120,  98,  55, 102) }; // .40
    inline const juce::Colour engravedLineSoft { juce::Colour::fromRGBA (120,  98,  55,  89) }; // .35
    inline const juce::Colour highlightEdge    { juce::Colour::fromRGBA (255, 255, 255, 166) }; // .65
    inline const juce::Colour panelInnerShade  { juce::Colour::fromRGBA ( 90,  70,  40,  46) }; // .18
    inline const juce::Colour scanline         { juce::Colour::fromRGBA ( 80,  60,  25,   9) }; // .035

    // GUI-SPEC.md section 4's corrected palette. Ratios are WCAG relative-luminance contrast against
    // the DARKEST point of the fascia gradient, #d8cdb0 - not the #efe6d0 top. Measuring against the
    // light end is what let the old values look compliant: #4a4132 reads 8.07:1 there and 6.34:1
    // where the text actually has to survive.
    //
    // Retired, and not to be reinstated: #4a4132 (6.34:1) and #5c5241 (4.85:1) both failed the
    // functional floor, and #9a8e74 measured 2.04:1 - the worst on the panel, used for the
    // unselected algorithm labels, i.e. text carrying real state.
    //
    // Verified independently rather than taken from the table: every value below measures within
    // 0.01 of its stated ratio except textPrimary, which the spec calls 9.07 and measures 8.82.
    // Still comfortably functional, so it stands; noted because deltas get measured, not read.
    inline const juce::Colour textPrimary      { 0xFF332B1E };   // 8.82:1 functional
    inline const juce::Colour textSecondary    { 0xFF3E3527 };   // 7.62:1 functional
    inline const juce::Colour scaleNumeral     { 0xFF3E3527 };   // 7.62:1 functional - printed scales
    inline const juce::Colour textTertiary     { 0xFF5E5440 };   // 4.71:1 flavour
    inline const juce::Colour textMuted        { 0xFF5E5440 };   // 4.71:1 flavour
    // textFaint (#9a8e74, 2.04:1) is RETIRED. It carried the unselected algorithm labels - text
    // reporting real state at barely twice the contrast of the fascia itself - and the version
    // stamp. Both now use textTertiary. Anything reaching for a fainter tone should ask whether the
    // text is needed at all: BRAND.md's Legibility says below the flavour floor it is "decoration
    // pretending to be information".
    inline const juce::Colour labelSelected    { 0xFF332B1E };   // 8.82:1 functional

    // --- Bezel / dark plate --------------------------------------------------
    inline const juce::Colour bezelTop         { 0xFF22304C };
    inline const juce::Colour bezelMid         { 0xFF1A2740 };   // at 55%
    inline const juce::Colour bezelBottom      { 0xFF142036 };
    inline const juce::Colour pillTop          { 0xFF22304C };
    inline const juce::Colour pillBottom       { 0xFF16223A };
    inline const juce::Colour bezelLabel       { 0xFFA9B6CD };
    inline const juce::Colour bezelGold        { 0xFFC8B177 };
    inline const juce::Colour bezelGoldBright  { 0xFFD8C18A };
    inline const juce::Colour wordmark         { 0xFFF0E2BA };

    // --- LCD -----------------------------------------------------------------
    inline const juce::Colour lcdTop           { 0xFF0A0F18 };
    inline const juce::Colour lcdBottom        { 0xFF060A11 };
    inline const juce::Colour lcdHoverTop      { 0xFF0E1522 };
    inline const juce::Colour lcdHoverBottom   { 0xFF080D16 };
    inline const juce::Colour phosphor         { 0xFFF2E6C2 };
    inline const juce::Colour meterText        { 0xFFE8DCBA };

    // --- Buttons -------------------------------------------------------------
    inline const juce::Colour brassTop         { 0xFFDED0A6 };
    inline const juce::Colour brassBottom      { 0xFFBDA979 };
    inline const juce::Colour brassTopHover    { 0xFFEADCB4 };
    inline const juce::Colour brassBottomHover { 0xFFCBB787 };
    inline const juce::Colour brassText        { 0xFF2A3550 };
    inline const juce::Colour buttonOffTop     { 0xFF232F49 };
    inline const juce::Colour buttonOffBottom  { 0xFF1B2640 };
    inline const juce::Colour buttonOffText    { 0xFF4A5670 };

    // --- Scope ---------------------------------------------------------------
    inline const juce::Colour screenTop        { 0xFF080D16 };
    inline const juce::Colour screenBottom     { 0xFF050810 };
    inline const juce::Colour scopeBezelTop    { 0xFFC9BD9C };
    inline const juce::Colour scopeBezelBottom { 0xFFB8AA87 };
    inline const juce::Colour scopeGrid        { juce::Colour::fromRGBA (120, 160, 200,  26) }; // .10
    inline const juce::Colour scopeLegend      { juce::Colour::fromRGBA (190, 205, 225, 128) }; // .50
    inline const juce::Colour scopeLegendDim   { juce::Colour::fromRGBA (190, 205, 225, 102) }; // .40
    inline const juce::Colour scopeNoiseTail   { juce::Colour::fromRGBA (190, 205, 225,  41) }; // .16

    // --- Knobs ---------------------------------------------------------------
    inline const juce::Colour tick             { juce::Colour::fromRGBA ( 80,  64,  30, 179) }; // .70
    inline const juce::Colour pointerDark      { 0xFF33291A };
    inline const juce::Colour pointerDarkLarge { 0xFF2F2617 };
    inline const juce::Colour knobShadow       { juce::Colour::fromRGBA ( 45,  33,  12, 115) }; // .45
    inline const juce::Colour knobInnerShade   { juce::Colour::fromRGBA ( 90,  70,  30,  89) }; // .35

    // Algorithm rotary - dark, matching the bezel, so it reads as the one "system" control
    // among brass knobs (design/README.md section 2).
    inline const juce::Colour algoFace0        { 0xFF1F2B44 };
    inline const juce::Colour algoFace1        { 0xFF16223A };   // 52%
    inline const juce::Colour algoFace2        { 0xFF0D1526 };   // 80%
    inline const juce::Colour algoFace3        { 0xFF070C15 };
    inline const juce::Colour algoPointerTop   { 0xFFF4E8C4 };
    inline const juce::Colour algoPointerBottom{ 0xFFCDB989 };

    /** The one accent colour, per BRAND.md. Reserved exclusively for the TANK LIVE LED, its glow,
        and the live decay trace + its fill - never on knobs, labels, or decoration. */
    inline const juce::Colour accent           { 0xFF5CE07A };
}

//==============================================================================
namespace Font
{
    // Function-local statics so each face is created once, lazily, and thread-safely.
    // NOTE: JUCE's binary-data name mangling STRIPS non-alphanumeric characters rather than
    // converting them to underscores, so Jost-500-Medium.ttf becomes Jost500Medium_ttf.
    inline juce::Typeface::Ptr wordmarkTypeface()
    {
        static const juce::Typeface::Ptr t = juce::Typeface::createSystemTypefaceFor (
            BinaryData::Jost500Medium_ttf, (size_t) BinaryData::Jost500Medium_ttfSize);
        return t;
    }

    inline juce::Typeface::Ptr monoTypeface()
    {
        static const juce::Typeface::Ptr t = juce::Typeface::createSystemTypefaceFor (
            BinaryData::IBMPlexMonoRegular_ttf, (size_t) BinaryData::IBMPlexMonoRegular_ttfSize);
        return t;
    }

    inline juce::Typeface::Ptr monoMediumTypeface()
    {
        static const juce::Typeface::Ptr t = juce::Typeface::createSystemTypefaceFor (
            BinaryData::IBMPlexMonoMedium_ttf, (size_t) BinaryData::IBMPlexMonoMedium_ttfSize);
        return t;
    }

    /** Builds a font whose em size equals the design doc's CSS px value.

        This is what `font-size: 9px` means, and it is NOT juce::Font::withHeight(), which sets
        ascent+descent - a typeface-specific multiple of the em size, so passing a CSS px straight
        to withHeight() renders visibly small. Gatecrasher and CHORUS-60 both worked around this
        with a ratio calibrated off measured artwork; JUCE 8's withPointHeight() expresses it
        directly, so no calibration constant is needed here. */
    inline juce::Font mono (float cssPx)
    {
        return juce::Font (juce::FontOptions (monoTypeface()).withPointHeight (cssPx));
    }

    inline juce::Font monoMedium (float cssPx)
    {
        return juce::Font (juce::FontOptions (monoMediumTypeface()).withPointHeight (cssPx));
    }

    inline juce::Font wordmark (float cssPx)
    {
        return juce::Font (juce::FontOptions (wordmarkTypeface()).withPointHeight (cssPx));
    }

    /** CSS `letter-spacing` is expressed in em, so its pixel value scales with the font size.
        Every tracking value in design/README.md is quoted in em - always convert through here
        rather than hard-coding a pixel gap. */
    inline constexpr float trackingPx (float em, float cssPx) noexcept { return em * cssPx; }
}

//==============================================================================
namespace Text
{
    /** U+00B7 MIDDLE DOT, used as the separator throughout the panel ("MODEL RF-84 . STEREO",
        "GRAIN 46 . 18 STEP"). Built from its codepoint rather than written as a literal or a
        \x escape sequence: both depend on the source file's encoding surviving every toolchain
        the suite builds on, and a mis-decoded one renders as a stray "Â". */
    inline juce::String middleDot()
    {
        return juce::String::charToString ((juce::juce_wchar) 0x00B7);
    }

    /** juce::Font has no absolute-pixel letter-spacing, so tracked text is measured and drawn
        glyph by glyph. Ported from TapeRot's TapeRotTheme.h, which needed the same thing to
        reproduce its SVG's letter-spacing attribute. */
    inline float trackedWidth (const juce::String& text, const juce::Font& font, float tracking)
    {
        float width = 0.0f;

        for (int i = 0; i < text.length(); ++i)
        {
            width += juce::GlyphArrangement::getStringWidth (font, juce::String::charToString (text[i]));

            if (i < text.length() - 1)
                width += tracking;
        }

        return width;
    }

    /** Draws `text` with absolute-pixel tracking, justified within `area`. Vertical placement is
        area-centred, so callers position by the line box rather than the baseline. */
    inline void drawTracked (juce::Graphics& g,
                             const juce::String& text,
                             const juce::Font& font,
                             float tracking,
                             juce::Rectangle<float> area,
                             juce::Justification justification,
                             juce::Colour colour)
    {
        g.setFont (font);
        g.setColour (colour);

        const float total = trackedWidth (text, font, tracking);

        float x = area.getX();

        if (justification.testFlags (juce::Justification::horizontallyCentred))
            x = area.getCentreX() - total * 0.5f;
        else if (justification.testFlags (juce::Justification::right))
            x = area.getRight() - total;

        for (int i = 0; i < text.length(); ++i)
        {
            const auto ch = juce::String::charToString (text[i]);
            const float w = juce::GlyphArrangement::getStringWidth (font, ch);

            g.drawText (ch,
                        juce::Rectangle<float> (x, area.getY(), w + 1.0f, area.getHeight()),
                        juce::Justification::centredLeft,
                        false);

            x += w + tracking;
        }
    }
}

//==============================================================================
namespace Layout
{
    // --- Canvas --------------------------------------------------------------
    // GUI-SPEC.md section 1. Confirmed against all three v1.1 screenshots, which are 2680 x 1290 -
    // the artwork's own figure, not the doc's prose. v1.0's 1200 x 615 is retired along with its
    // three-column body; v1.1 is four columns.
    inline constexpr float canvasWidth  = 1340.0f;
    inline constexpr float canvasHeight = 645.0f;
    inline constexpr float panelRadius  = 10.0f;
    inline constexpr float panelPadding = 14.0f;

    /** The overlay texture: 1px scanlines every 3px, plus a top-left radial sheen. */
    inline constexpr float scanlinePitch = 3.0f;

    // --- Header bezel (measured: fill x 15..1185, y 15..118, 1px border outside) -------------
    inline constexpr float headerX = 14.0f;
    inline constexpr float headerY = 14.0f;
    inline constexpr float headerW = 1172.0f;
    inline constexpr float headerH = 105.0f;
    inline constexpr float headerRadius = 6.0f;

    // Wordmark block: min-width 300, starting at the header's content box (x + 1px border + 22px
    // padding). Ink measured at x 40..297, cap top y 32, baseline y ~62.5.
    inline constexpr float wordmarkX = 37.0f;
    inline constexpr float wordmarkTop = 29.0f;
    inline constexpr float wordmarkSize = 42.0f;
    inline constexpr float wordmarkLineHeight = 38.6f;      // 42 * .92
    inline constexpr float taglineSize = 10.0f;
    inline constexpr float taglineTop = 76.0f;              // first of two lines, 3px apart
    inline constexpr float taglineLineHeight = 13.0f;
    inline constexpr float taglineGap = 3.0f;

    // PROGRAM block
    inline constexpr float programLabelX = 363.0f;
    inline constexpr float programLabelY = 36.0f;
    inline constexpr float programLabelH = 12.0f;
    inline constexpr float programWellX = 363.0f;
    inline constexpr float programWellY = 55.0f;
    inline constexpr float programWellW = 432.0f;
    inline constexpr float programWellH = 42.0f;
    inline constexpr float saveButtonX = 805.0f;
    inline constexpr float saveButtonW = 64.0f;
    inline constexpr float deleteButtonX = 879.0f;
    inline constexpr float deleteButtonW = 78.0f;
    inline constexpr float headerButtonY = programWellY;
    inline constexpr float headerButtonH = programWellH;
    inline constexpr float lcdRadius = 3.0f;

    // FACT/USER badge sits 14px in from the well's left edge; the chevron 11px from its right.
    inline constexpr float badgeInsetX = 14.0f;
    inline constexpr float badgeW = 40.0f;
    inline constexpr float badgeH = 18.0f;
    inline constexpr float chevronInsetX = 11.0f;
    inline constexpr float chevronSize = 9.0f;

    // IN / OUT meters (measured: wells x 985 and 1079, both 84 wide, y 69..104)
    inline constexpr float meterLabelY = 51.0f;
    inline constexpr float meterLabelH = 12.0f;
    inline constexpr float meterWellY = 69.0f;
    inline constexpr float meterWellW = 84.0f;
    inline constexpr float meterWellH = 35.0f;
    inline constexpr float meterInX = 985.0f;
    inline constexpr float meterOutX = 1079.0f;

    // --- Body row ------------------------------------------------------------
    // Panel padding box minus the body row's own `padding: 20px 4px 6px`.
    inline constexpr float bodyTop = 139.0f;
    inline constexpr float bodyLeft = 18.0f;
    inline constexpr float bodyRight = 1182.0f;

    // Columns. Widths include each column's own padding, because the design's inner elements are
    // content-box: `width: 336px; padding-right: 22px` occupies 358px.
    inline constexpr float leftColumnX = 18.0f;
    inline constexpr float leftColumnW = 336.0f;            // content, excluding the 22px padding
    inline constexpr float leftColumnCentre = leftColumnX + leftColumnW * 0.5f;   // 186

    inline constexpr float divider1X = 376.0f;
    inline constexpr float divider2X = 983.0f;
    inline constexpr float dividerTop = 139.0f;
    inline constexpr float dividerBottom = 601.0f;

    inline constexpr float centreColumnX = 401.0f;          // content box
    inline constexpr float centreColumnW = 558.0f;
    inline constexpr float centreColumnCentre = centreColumnX + centreColumnW * 0.5f;   // 680

    inline constexpr float rightColumnX = 1006.0f;
    inline constexpr float rightColumnW = 176.0f;
    inline constexpr float rightColumnCentre = rightColumnX + rightColumnW * 0.5f;      // 1094

    // --- ALGORITHM rotary (measured: body x 134..238, y 161..265) -------------
    inline constexpr float algoCentreX = 186.0f;
    inline constexpr float algoCentreY = 213.0f;
    inline constexpr float algoRadius = 52.0f;
    inline constexpr float algoTickInset = -15.0f;          // ring outer radius = 67

    /** Detent angles, and the ticks are drawn CENTRED on them.

        GUI-SPEC.md section 6 states them this way now, which ends a 0.55 degree error: the old
        `224.45` was the LEADING EDGE of the design's 1.1 degree-wide wedge, and reading it as a
        centre drew every tick just counter-clockwise of the position it marks. The pointer detents
        sit at -45 / +45 / +135 / +225, so the wedge centre is 225.0.

        Not derived from a start angle plus a step, deliberately: the four positions are a property
        of the switch, and writing them out means the next person reads angles rather than
        reconstructing them. */
    inline constexpr std::array<float, 4> algoDetentDegrees { { -45.0f, 45.0f, 135.0f, 225.0f } };
    inline constexpr float algoPointerTopInset = 9.0f;
    inline constexpr float algoPointerWidth = 3.0f;
    inline constexpr float algoPointerLengthFraction = 0.36f;   // of the radius
    inline constexpr float algoCaptionY = 290.0f;
    inline constexpr float algoLabelSize = 10.0f;

    // Printed scales, GUI-SPEC.md section 7. 10px is the floor for functional text (BRAND.md's
    // Legibility) and these are functional now that the standing readouts are gone - they are the
    // only at-rest value reference on the panel. The unit is the same size for the same reason:
    // "kHz" tells you what the numerals mean, so it is not decoration.
    inline constexpr float scaleNumeralSize = 10.0f;
    inline constexpr float scaleUnitSize = 10.0f;
    inline constexpr float algoCaptionSize = 9.0f;

    /** Corner label placement. design/README.md section 2 is explicit that the visual arrangement
        is NOT clockwise-sequential - HALL is index 3 at bottom-left, CHAMBER index 2 at
        bottom-right - so the DSP enum order and the panel order are joined by this table and
        neither is derived from the other. */
    enum class Corner { topLeft, topRight, bottomLeft, bottomRight };

    struct AlgorithmCorner
    {
        int index;                  // Algorithm enum value
        const char* label;
        const char* secondLine;     // "DIGITAL ROOM" wraps to two lines
        Corner corner;
    };

    inline constexpr std::array<AlgorithmCorner, 4> algorithmCorners { {
        { 0, "PLATE",   nullptr, Corner::topLeft     },
        { 1, "DIGITAL", "ROOM",  Corner::topRight    },
        { 3, "HALL",    nullptr, Corner::bottomLeft  },
        { 2, "CHAMBER", nullptr, Corner::bottomRight },
    } };

    // --- Knob grammar --------------------------------------------------------
    // Travel arc 270 degrees, -135 to +135, measured clockwise from 12 o'clock. This is not
    // JUCE's own convention, so the trig helpers below convert.
    inline constexpr float knobArcStartDegrees = -135.0f;
    inline constexpr float knobArcEndDegrees   =  135.0f;

    // Three variants, not four. GUI-SPEC.md section 2 retires the 44px tiny knob from this panel:
    // five 10px numerals will not clear a 29px tick radius, and shrinking type below the 10px floor
    // is not available (BRAND.md's Legibility floor). The damping pair is promoted to small, which
    // also lets both keep their full mark set rather than dropping to three marks.
    enum class KnobSize { large, medium, small };

    struct KnobVariant
    {
        float radius;               // body radius; spec states diameter
        float tickArcRadius;        // r - where the printed ticks sit, measured from the dial centre
        float tickLength;           // drawn from tickArcRadius INWARD
        float numeralRadius;        // R - centre of each scale numeral
        float tickWidth;
        float pointerWidth;
        float pointerLengthFraction;// of the body diameter, from near the top edge inward
        float pointerTopInset;
        float labelSize;
        float labelTracking;        // em
        float unitDrop;             // unit string sits at cy + this, inside the arc's bottom gap
        float innerCapInset;        // 0 = no inner cap
    };

    // GUI-SPEC.md section 5's variant table. Body diameters 98 / 60 / 52 -> radii 49 / 30 / 26.
    // readoutSize is gone with the standing readouts (section 2 of the brief); unitDrop replaces it,
    // because a unit now prints once in the scale area rather than being appended to every value.
    inline constexpr KnobVariant largeKnob  { 49.0f, 62.0f, 8.0f, 80.0f, 2.0f, 3.0f, 0.30f, 8.0f, 11.0f, 0.22f, 74.0f, 15.0f };
    inline constexpr KnobVariant mediumKnob { 30.0f, 39.0f, 6.0f, 55.0f, 2.0f, 2.0f, 0.38f, 6.0f,  9.0f, 0.18f, 52.0f,  0.0f };
    inline constexpr KnobVariant smallKnob  { 26.0f, 35.0f, 6.0f, 50.0f, 2.0f, 2.0f, 0.37f, 5.0f,  9.0f, 0.16f, 44.0f,  0.0f };

    inline constexpr const KnobVariant& variantFor (KnobSize s) noexcept
    {
        return s == KnobSize::large  ? largeKnob
             : s == KnobSize::medium ? mediumKnob
                                     : smallKnob;
    }

    /** One printed numeral on a knob's scale.

        `f` is the ROTATION FRACTION, not the physical value - 0 at -135 degrees, 1 at +135. Storing
        the fraction rather than the value is what makes a log control come out right: the tick angle
        is `-135 + f * 270` with no inverse mapping in the drawing code, so a taper change cannot
        leave the ring pointing somewhere the pointer never reaches. BRAND.md requires the printed
        scale and the actual mapping to agree exactly, and this is the form that cannot drift.

        `printed` is the literal string, so OUTPUT TRIM keeps its explicit "+6" and DECAY prints
        "0.4" rather than a rounded "0". */
    struct ScaleMark
    {
        float f;
        const char* printed;
    };

    struct KnobScale
    {
        const ScaleMark* marks;
        int count;
        const char* unit;       // nullptr = bare numbers (SIZE, DIGITAL GRAIN)
    };

    // GUI-SPEC.md section 7, transcribed. The spec supplies both the fractions and the resulting
    // tick angles, so these are copied rather than derived - and the tick-angle column is what the
    // PrintedScaleTests assert against.
    //
    // Every linear control lands on quarters and so comes out evenly spaced. Two do not, and both
    // are correct:
    //   DECAY is linear in SECONDS over 0.4-8.0, so round numbers give uneven spacing.
    //   DAMPING LF is log, and 500 is not an octave above 320, so its last interval is short.
    // Neither may be evened out.
    inline constexpr ScaleMark sizeMarks[]    { {0.0f,"0.2"},{0.25f,"0.4"},{0.5f,"0.6"},{0.75f,"0.8"},{1.0f,"1.0"} };
    inline constexpr ScaleMark decayMarks[]   { {0.0f,"0.4"},{0.2105f,"2"},{0.4737f,"4"},{0.7368f,"6"},{1.0f,"8"} };
    inline constexpr ScaleMark preDelayMarks[]{ {0.0f,"0"},{0.25f,"45"},{0.5f,"90"},{0.75f,"135"},{1.0f,"180"} };
    inline constexpr ScaleMark percentMarks[] { {0.0f,"0"},{0.25f,"25"},{0.5f,"50"},{0.75f,"75"},{1.0f,"100"} };
    inline constexpr ScaleMark widthMarks[]   { {0.0f,"0"},{0.25f,"50"},{0.5f,"100"},{0.75f,"150"},{1.0f,"200"} };
    inline constexpr ScaleMark trimMarks[]    { {0.0f,"-12"},{0.25f,"-6"},{0.5f,"0"},{0.75f,"+6"},{1.0f,"+12"} };
    inline constexpr ScaleMark dampHFMarks[]  { {0.0f,"2"},{0.3333f,"4"},{0.6667f,"8"},{1.0f,"16"} };
    inline constexpr ScaleMark dampLFMarks[]  { {0.0f,"40"},{0.2744f,"80"},{0.5489f,"160"},{0.8233f,"320"},{1.0f,"500"} };

    struct KnobSpec
    {
        const char* paramID;
        const char* label;
        float centreX;
        float centreY;
        KnobSize size;
        KnobScale scale;
    };

    // GUI-SPEC.md section 5's dial-centre table, transcribed.
    //
    // **These are tick-arc centres, not cell centres.** BRAND.md's "Stating coordinates" is explicit
    // that a rotary's centre is the point the needle pivots about, and the spec states them that
    // way. The control label sits BELOW the arc, so the whole control cell is not centred on the
    // dial - measuring a pivot off the cell is what shipped TapeRot 7.27px out and made its needle
    // run past the printed end mark.
    //
    // The four-column body means every one of these moved; none is a nudge of the v1.0 figure.
    // Display names come from the spec too: PRE-DELAY and STEREO WIDTH are no longer abbreviated,
    // and the damping pair carries its own column rather than sitting under a shared caption.
    inline constexpr std::array<KnobSpec, 11> knobs { {
        { "dampHF",     "HF",              94.0f,  475.6f,   KnobSize::small,  { dampHFMarks,   4, "kHz" } },
        { "dampLF",     "LF",             222.0f,  475.6f,   KnobSize::small,  { dampLFMarks,   5, "Hz"  } },
        { "size",       "SIZE",           398.0f,  310.6f,   KnobSize::medium, { sizeMarks,     5, nullptr } },
        { "decay",      "DECAY",          530.0f,  310.6f,   KnobSize::medium, { decayMarks,    5, "s"   } },
        { "preDelay",   "PRE-DELAY",      398.0f,  461.1f,   KnobSize::medium, { preDelayMarks, 5, "ms"  } },
        { "density",    "DENSITY",        530.0f,  461.1f,   KnobSize::medium, { percentMarks,  5, "%"   } },
        { "modulation", "MODULATION",     773.5f,  480.1f,   KnobSize::large,  { percentMarks,  5, "%"   } },
        { "grain",      "DIGITAL GRAIN",  981.5f,  480.1f,   KnobSize::large,  { percentMarks,  5, nullptr } },
        { "width",      "STEREO WIDTH",  1244.0f,  225.1f,   KnobSize::small,  { widthMarks,    5, "%"   } },
        { "mix",        "MIX",           1244.0f,  373.6f,   KnobSize::small,  { percentMarks,  5, "%"   } },
        { "trim",       "OUTPUT TRIM",   1244.0f,  522.1f,   KnobSize::small,  { trimMarks,     5, "dB"  } },
    } };

    /** Gap from the knob's bottom edge down to its label, and from the label to its readout.
        design/README.md section 4: label below the body, numeric readout below that. */
    inline constexpr float knobLabelGap = 7.0f;
    inline constexpr float knobReadoutGap = 7.0f;
    inline constexpr float knobLabelLineHeight = 12.0f;
    inline constexpr float knobReadoutLineHeight = 13.0f;

    // --- Section pills (measured centres and tops) ---------------------------
    inline constexpr float pillHeight = 20.0f;
    inline constexpr float pillPaddingX = 11.0f;
    inline constexpr float pillRadius = 3.0f;
    inline constexpr float pillTextSize = 9.0f;
    inline constexpr float pillTracking = 0.26f;

    inline constexpr float tankPillY = 362.0f;
    inline constexpr float characterPillY = 370.0f;
    inline constexpr float outputPillY = 139.0f;

    inline constexpr float leftDividerY = 343.0f;           // engraved line + 1px white below

    // DAMPING is set to the LEFT of its knob pair, not above it:
    // `right: calc(50% + 70px); top: 18px` within the damping block.
    inline constexpr float dampingLabelRight = leftColumnCentre - 70.0f;   // 116
    inline constexpr float dampingLabelY = 533.0f;
    inline constexpr float dampingLabelSize = 9.0f;
    inline constexpr float dampingLabelTracking = 0.24f;

    // --- TANK LIVE scope (measured: screen border box x 407..953, y 174..344) -
    inline constexpr float ledX = 401.0f;
    inline constexpr float ledY = 139.0f;
    inline constexpr float ledSize = 15.0f;
    inline constexpr float ledGlowRadius = 26.0f;
    inline constexpr float ledLabelX = 427.0f;              // 11px gap after the lamp
    inline constexpr float ledLabelSize = 11.0f;
    inline constexpr float ledLabelTracking = 0.26f;
    inline constexpr float scopeHeaderY = 139.0f;
    inline constexpr float scopeHeaderH = 15.0f;
    inline constexpr float scopeHeaderTextSize = 10.0f;
    inline constexpr float scopeHeaderTracking = 0.20f;
    inline constexpr float scopeHeaderGap = 18.0f;

    inline constexpr float scopeBezelX = 401.0f;
    inline constexpr float scopeBezelY = 168.0f;
    inline constexpr float scopeBezelW = 558.0f;
    inline constexpr float scopeBezelH = 182.0f;
    inline constexpr float scopeBezelRadius = 4.0f;
    inline constexpr float scopeBezelPadding = 6.0f;

    inline constexpr float scopeScreenX = 639.0f;
    inline constexpr float scopeScreenY = 170.6f;
    inline constexpr float scopeScreenW = 477.0f;
    inline constexpr float scopeScreenH = 168.0f;
    inline constexpr float scopeScreenRadius = 2.0f;

    /** **The screen rectangle and the plot region are two different rectangles, and the trace is
        clamped to the PLOT REGION.** GUI-SPEC.md section 11.

        Everything below is in the 600 x 168 drawing space, which maps onto the 477-wide screen
        content box with preserveAspectRatio:none - so **x scales 477/600 = 0.795 and y scales 1.0**.
        That asymmetry is the trap: deriving the gutter split from any other width puts the leader
        ticks somewhere other than the labels they point at.

            Screen                0,  0, 600, 168
            Title strip (reserved) 0,  0, 600,  20     DCY ENV, grain state
            Plot region            0, 20, 520, 148     the trace lives here and nowhere else
            Level gutter (reserved) 520, 20, 80, 148   0 dB, -60 dB, leader ticks

        Within the plot region 0 dB is y = 26 and -60 dB (the baseline) is y = 156, so full-scale
        height is 130. The trace uses that full vertical extent - it touches 26 at peak and rests on
        156 - and is clipped horizontally at x = 520.

        **A top or bottom margin is not a substitute for the gutter.** 0 dB and -60 dB are the levels
        being annotated, so the trace has to be able to reach them; the separation has to be
        horizontal. The previous arrangement drew the four legends inside the plot area and let the
        trace run underneath them, which only looked safe because the reference render happened to
        show a short decay - at 200 ms per division a long tail reaches the right edge and settles
        near the baseline, exactly where the -60 dB legend sat. */
    inline constexpr float scopePlotX = 0.0f;
    inline constexpr float scopePlotY = 20.0f;
    inline constexpr float scopePlotW = 520.0f;
    inline constexpr float scopePlotH = 148.0f;
    inline constexpr float scopeTitleStripH = 20.0f;
    inline constexpr float scopeGutterX = 520.0f;
    inline constexpr float scopeZeroDbY = 26.0f;
    inline constexpr float scopeMinusSixtyDbY = 156.0f;
    inline constexpr float scopeLeaderTickX0 = 520.0f;
    inline constexpr float scopeLeaderTickX1 = 532.0f;
    inline constexpr float scopeGutterLabelX = 532.0f;
    inline constexpr float scopeLeaderTickWidth = 1.5f;

    /** design/README.md section 6's drawing model is specified against a 600 x 168 viewBox with
        preserveAspectRatio="none", so it is stretched onto the screen's real size rather than
        letterboxed. Both numbers stay here because every formula in that section - the 60px grid
        pitch, the 3-27px grain step, the `60 + phase * (W + 260)` sweep - is quoted in viewBox
        units, and rescaling them individually would be a transcription hazard. */
    inline constexpr float scopeViewBoxW = 600.0f;
    inline constexpr float scopeViewBoxH = 168.0f;
    inline constexpr float scopeGridPitchX = 60.0f;
    inline constexpr float scopeGridPitchY = 42.0f;
    inline constexpr float scopeLegendSize = 9.0f;
    inline constexpr float scopeLegendTracking = 0.20f;
    inline constexpr float scopeLegendInsetX = 8.0f;
    inline constexpr float scopeLegendInsetTop = 6.0f;
    inline constexpr float scopeLegendInsetBottom = 5.0f;

    /** Sweep and envelope constants, all from design/README.md section 6. */
    inline constexpr float scopeTimeSpanSeconds = 2.4f;
    inline constexpr float scopeTauDivisor = 6.0f;          // tau = decaySeconds / 6
    inline constexpr float scopeTauScale = 1.6f;            // env = exp(-t / (tau * 1.6))
    inline constexpr float scopePhaseStep = 0.016f;         // per ~40ms tick
    inline constexpr int   scopeTimerHz = 25;
    inline constexpr int   scopeNoiseLines = 240;
    inline constexpr float scopeBaselineInset = 4.0f;       // baseline y = H - 4
    inline constexpr float scopeFullScaleInset = 26.0f;     // trace height = H - 26
    inline constexpr float scopeNoiseScaleInset = 30.0f;    // noise height = H - 30
    inline constexpr float scopeSweepOrigin = 60.0f;
    inline constexpr float scopeSweepOvershoot = 260.0f;
    inline constexpr float scopeTraceFillAlpha = 0.16f;
    inline constexpr float scopeBloomWidth = 2.6f;
    inline constexpr float scopeBloomAlpha = 0.75f;
    inline constexpr float scopeTraceWidth = 1.6f;

    // --- Version stamp -------------------------------------------------------
    inline constexpr float versionRight = 1182.0f;
    inline constexpr float versionY = 583.0f;
    inline constexpr float versionSize = 10.0f;
    inline constexpr float versionTracking = 0.10f;
}

//==============================================================================
// Shared geometry helpers. Angles are degrees clockwise from 12 o'clock, matching the design
// doc's own rotation values - NOT juce::Slider's convention.
namespace Geometry
{
    inline float knobAngleForValue (float value01) noexcept
    {
        return Layout::knobArcStartDegrees
             + value01 * (Layout::knobArcEndDegrees - Layout::knobArcStartDegrees);
    }

    inline juce::Point<float> directionFor (float degrees) noexcept
    {
        const float radians = juce::degreesToRadians (degrees);
        return { std::sin (radians), -std::cos (radians) };
    }

    inline juce::Point<float> pointOnCircle (juce::Point<float> centre, float radius, float degrees) noexcept
    {
        return centre + directionFor (degrees) * radius;
    }
}

//==============================================================================
namespace Paint
{
    /** A CSS `linear-gradient(top, bottom)` over a rectangle. */
    inline juce::ColourGradient verticalGradient (juce::Rectangle<float> r,
                                                  juce::Colour top,
                                                  juce::Colour bottom)
    {
        return { top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false };
    }

    /** A CSS `radial-gradient(circle at fx%, fy%, ...)` over a square.

        CSS's default sizing is farthest-corner, so the gradient's radius is the distance from the
        offset centre to the furthest corner of the box - not the box's own radius. Getting that
        wrong makes every knob read as a flat disc with an off-centre blob rather than a dome. */
    inline juce::ColourGradient radialFace (juce::Rectangle<float> box,
                                            float focusX, float focusY,
                                            juce::Colour c0,
                                            juce::Colour c1, float stop1,
                                            juce::Colour c2, float stop2,
                                            juce::Colour c3)
    {
        const juce::Point<float> centre { box.getX() + box.getWidth()  * focusX,
                                          box.getY() + box.getHeight() * focusY };

        const float dx = juce::jmax (centre.x - box.getX(), box.getRight()  - centre.x);
        const float dy = juce::jmax (centre.y - box.getY(), box.getBottom() - centre.y);
        const float radius = std::sqrt (dx * dx + dy * dy);

        juce::ColourGradient g { c0, centre.x, centre.y, c3, centre.x + radius, centre.y, true };
        g.addColour ((double) stop1, c1);
        g.addColour ((double) stop2, c2);
        return g;
    }

    /** The recessed dark-blue plate used by the header bezel. */
    inline void drawBezelPlate (juce::Graphics& g, juce::Rectangle<float> r, float radius)
    {
        auto grad = verticalGradient (r, Colour::bezelTop, Colour::bezelBottom);
        grad.addColour (0.55, Colour::bezelMid);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, radius);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r, radius, 1.0f);

        // inset 0 1px 0 rgba(255,255,255,.12) - the lit top lip
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (r.getX() + radius, r.getY() + 1.0f, r.getRight() - radius, r.getY() + 1.0f, 1.0f);
    }

    /** The LCD well recipe, shared by the Program display and both meter readouts. */
    inline void drawLcdWell (juce::Graphics& g, juce::Rectangle<float> r, bool hovered = false)
    {
        g.setGradientFill (verticalGradient (r,
                                             hovered ? Colour::lcdHoverTop : Colour::lcdTop,
                                             hovered ? Colour::lcdHoverBottom : Colour::lcdBottom));
        g.fillRoundedRectangle (r, Layout::lcdRadius);

        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (r, Layout::lcdRadius, 1.0f);

        // 0 1px 0 rgba(255,255,255,.12) - the lip of the cut-out, below the well
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (r.getX() + 2.0f, r.getBottom() + 0.5f, r.getRight() - 2.0f, r.getBottom() + 0.5f, 1.0f);
    }

    /** A section header pill: REVERB TANK / CHARACTER / OUTPUT all share it. */
    inline void drawSectionPill (juce::Graphics& g, const juce::String& text, float centreX, float top)
    {
        const auto font = Font::mono (Layout::pillTextSize);
        const float tracking = Font::trackingPx (Layout::pillTracking, Layout::pillTextSize);
        const float textWidth = Text::trackedWidth (text, font, tracking);
        const float width = textWidth + Layout::pillPaddingX * 2.0f;

        const juce::Rectangle<float> r { centreX - width * 0.5f, top, width, Layout::pillHeight };

        g.setGradientFill (verticalGradient (r, Colour::pillTop, Colour::pillBottom));
        g.fillRoundedRectangle (r, Layout::pillRadius);

        Text::drawTracked (g, text, font, tracking, r,
                           juce::Justification::centred, Colour::bezelGoldBright);
    }

    /** The engraved-groove divider: a 1px line with a 1px white highlight on its light side. */
    inline void drawVerticalDivider (juce::Graphics& g, float x, float top, float bottom)
    {
        juce::ColourGradient grad { Colour::engravedLine.withAlpha (0.0f), x, top,
                                    Colour::engravedLine.withAlpha (0.0f), x, bottom, false };
        grad.addColour (0.2, Colour::engravedLine);
        grad.addColour (0.8, Colour::engravedLine);
        g.setGradientFill (grad);
        g.fillRect (x, top, 1.0f, bottom - top);

        g.setColour (Colour::highlightEdge);
        g.fillRect (x + 1.0f, top, 1.0f, bottom - top);
    }

    inline void drawHorizontalDivider (juce::Graphics& g, float x, float y, float width)
    {
        g.setGradientFill ({ Colour::engravedLineSoft, x, y,
                             Colour::engravedLineSoft.withAlpha (0.0f), x + width, y, false });
        g.fillRect (x, y, width, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.fillRect (x, y + 1.0f, width, 1.0f);
    }

    /** Tick ring around a knob's travel arc - one tick per printed numeral, at that numeral's
        angle. The design draws these as a conic gradient masked to a ring; GUI-SPEC.md's assets
        note says to draw them in JUCE as short line segments instead, which is what this does. */
    inline void drawTickRing (juce::Graphics& g,
                              juce::Point<float> centre,
                              float bodyRadius,
                              const Layout::KnobVariant& v,
                              const Layout::KnobScale& scale,
                              juce::Colour colour)
    {
        // GUI-SPEC.md section 5: ticks run from the tick-arc radius INWARD by the tick length, and
        // are centred on the tick's angle. Section 7 puts one at every printed numeral and nowhere
        // else - no minor ticks, no even-angle ring.
        //
        // This replaced a fixed `one tick every N degrees` loop. Even spacing is only ever right by
        // coincidence: it holds for the linear controls because their marks are quarters, and it is
        // wrong for DECAY (linear in seconds, round numbers) and for both damping knobs (log). A
        // numeral sitting visibly off its nearest tick reads as an error even when the numeral is
        // correct - BRAND.md's "Ticks sit at the labelled values".
        const float outer = v.tickArcRadius;
        const float inner = outer - v.tickLength;

        g.setColour (colour);

        for (int i = 0; i < scale.count; ++i)
        {
            const float angle = Layout::knobArcStartDegrees
                              + scale.marks[i].f * (Layout::knobArcEndDegrees - Layout::knobArcStartDegrees);

            g.drawLine ({ Geometry::pointOnCircle (centre, inner, angle),
                          Geometry::pointOnCircle (centre, outer, angle) }, v.tickWidth);
        }

        juce::ignoreUnused (bodyRadius);
    }
}

} // namespace ReflectTheme
