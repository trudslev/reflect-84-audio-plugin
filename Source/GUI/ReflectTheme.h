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
#include <nf/ParameterReadout.h>

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
    // **Reclassified functional, and darkened to reach that floor.** These were annotated
    // "flavour" at 4.71:1, but three of their four uses carry information rather than decorate:
    // the unselected algorithm names report state, the scope header prints RT60 and ms/DIV, and
    // textMuted labels the ALGORITHM section. Flavour is for text that can be missed.
    // contrast: 7.12-9.05:1 vs fasciaTop,fasciaMid,fasciaBottom [functional]
    inline const juce::Colour textTertiary     { 0xFF413A2C };
    // contrast: 7.12-9.05:1 vs fasciaTop,fasciaMid,fasciaBottom [functional]
    inline const juce::Colour textMuted        { 0xFF413A2C };
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
    // Lightened from #A9B6CD, which read 6.43:1 at the top of the bezel gradient. This carries
    // REVERB PROCESSOR and MODEL RF-84 - BRAND.md calls the model line primary identification,
    // so it is functional text and cannot sit under the floor at any point of its own ground.
    // contrast: 7.09-8.77:1 vs bezelTop,bezelMid,bezelBottom [functional]
    inline const juce::Colour bezelLabel       { 0xFFB3BFD3 };
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

    // --- Program list --------------------------------------------------------
    /** **The list is the display continuing downward**, so it is drawn in the LCD's own well
        material rather than the header's navy bezel - GUI-SPEC.md section 9. A bezel-coloured list
        at the display's width reads as a panel part that has appeared from nowhere; glass reads as
        more of the screen. */
    inline const juce::Colour listTop          { 0xFF0A0F18 };
    inline const juce::Colour listMid          { 0xFF070C14 };   // at 45%
    inline const juce::Colour listBottom       { 0xFF05080E };

    // Program names. The ink crosses the whole gradient, so both ends are quoted.
    // contrast: 15.42-16.11:1 vs listTop,listBottom [functional]
    inline const juce::Colour listItem         { 0xFFF2E6C2 };
    // The FACTORY / USER captions.
    // contrast: 10.89-11.37:1 vs listTop,listBottom [functional]
    inline const juce::Colour listCaption      { 0xFFD8C18A };
    /** The `- none saved -` row. **State, not functional**: a deliberately non-interactive row,
        which BRAND.md's Legibility puts outside the functional floor - its job is to convey
        *nothing here* rather than to be read. Still comfortably clear of the 3:1 state floor. */
    // contrast: 7.22-7.55:1 vs listTop,listBottom [state]
    inline const juce::Colour listPlaceholder  { 0xFF8EA0BC };
    /** The rule between groups. **Deliberately carries no contrast annotation**, which is the
        house treatment for a hairline - see scopeLeaderLine below: *"not text ... Alpha on a rule
        is furniture and stays."* BRAND.md's floors are stated for TEXT ("No functional text
        below...", "Flavour text never falls below..."), and judging a 1px divider by a text
        standard is a category error. Annotated [flavour] it measured 2.30:1 and failed, which
        would have been a bar it was never under. */
    inline const juce::Colour listSeparator    { juce::Colour::fromRGBA (242, 230, 194, 77) }; // .30

    /** The current Program's 3px bar, and the row's own lift. A bar rather than a tick: a tick
        costs a character cell on every row to serve one, and JUCE's tick is the most OS-looking
        mark available. The bar reads straight down one edge. */
    inline const juce::Colour listMarker       { 0xFFF2E6C2 };
    inline const juce::Colour listCurrentField { juce::Colour::fromRGBA (120, 160, 200, 23) }; // .09
    inline const juce::Colour listHoverField   { juce::Colour::fromRGBA (120, 160, 200, 26) }; // .10
    // The current row's text, which steps up from listItem.
    // contrast: 17.93-18.73:1 vs listTop,listBottom [functional]
    inline const juce::Colour listCurrentText  { 0xFFFDF7E6 };

    /** **The chevron bands are opaque and their own colour**, which is why the chevrons are
        measured against them rather than against the list surface. Top band is the surface's light
        end, bottom band its dark end, each with a 1px rule along its inner edge. */
    inline const juce::Colour listBandTop      { 0xFF0A0F18 };
    inline const juce::Colour listBandBottom   { 0xFF05080E };
    inline const juce::Colour listBandRule     { juce::Colour::fromRGBA (242, 230, 194, 26) }; // .10

    // The scroll chevron, enabled.
    // contrast: 10.89-11.37:1 vs listBandTop,listBandBottom [functional]
    inline const juce::Colour listChevron      { 0xFFD8C18A };
    /** At the end of its travel the chevron is **not removed and not hidden** - it steps back to
        the Program buttons' unlit ink with no glow, because *nothing on this panel is ever drawn
        inert*. State-dimming, so the 3:1 floor applies rather than the functional one. */
    // contrast: 5.96-6.22:1 vs listBandTop,listBandBottom [state]
    inline const juce::Colour listChevronDim   { 0xFF8090AE };

    // --- Program buttons -----------------------------------------------------
    /** **One face, in every state.** GUI-SPEC.md section 9: each button carries two legends,
        stacked and permanently printed - SAVE above STORE, DELETE above CANCEL - and the face
        never changes. Only the legends' illumination does.

        This replaced a brass cap (#DED0A6 -> #BDA979) with a separate disabled face. Both are
        gone, and neither should come back:

        - **A printed panel legend cannot rewrite itself.** Five castings relabelled SAVE to STORE
          at runtime, which no piece of rack gear can do; the second legend is how hardware says it.
        - **A dark face is what gives a lit legend somewhere brighter to go.** On the brass, lit
          type had no headroom - the cap was already the brightest thing on the button, so
          "illuminated" could not read as illuminated.
        - **There is no disabled face.** Real gear does not grey a button out; its lamp goes out.
          Both legends unlit *is* the "nothing to do here" state, and it still has to be readable. */
    inline const juce::Colour buttonFaceTop    { 0xFF26324D };
    inline const juce::Colour buttonFaceBottom { 0xFF1A2438 };

    /** The legend crosses the face's gradient, so both ratios are quoted against its LIGHT end -
        the worst case. Measuring against the mean would flatter both by about a stop.

        **Both annotations used to sit here, above legendLit.** An annotation binds to the next
        colour constant below it, so the unlit figure bound to the LIT colour and check_contrast.py
        reported legendLit as claiming 3.91 while measuring 11.92. The colour had not drifted - the
        annotation had taken the wrong row, the same one-value-two-meanings shape as a schema
        version or a font size serving two things. GUI-SPEC.md:197-198 gives 11.91 lit and 3.91
        unlit; each now sits above the constant it describes. */
    // contrast: 11.91:1 vs buttonFaceTop [functional]
    inline const juce::Colour legendLit        { 0xFFFDF7E6 };
    // **3.96, not the spec's 3.91** - measured off the shipped colours rather than transcribed.
    // #8090AE on #26324D is 3.9605; GUI-SPEC.md:198 rounds to 3.91. A 0.05 gap, which is the
    // designer's arithmetic against ours and is below any perceptual threshold; both clear the 3:1
    // state floor comfortably, so this is recorded rather than raised. It only surfaced now because
    // the annotation had never bound to this constant - see the note above.
    // contrast: 3.96:1 vs buttonFaceTop [state]
    inline const juce::Colour legendUnlit      { 0xFF8090AE };

    // --- Scope ---------------------------------------------------------------
    inline const juce::Colour screenTop        { 0xFF080D16 };
    inline const juce::Colour screenBottom     { 0xFF050810 };
    inline const juce::Colour scopeBezelTop    { 0xFFC9BD9C };
    inline const juce::Colour scopeBezelBottom { 0xFFB8AA87 };
    inline const juce::Colour scopeGrid        { juce::Colour::fromRGBA (120, 160, 200,  26) }; // .10
    // **Opaque, and that alone was the fix.** These are the scope's 0 dB / -60 dB / DCY ENV
    // labels - printed scales, which BRAND.md names as functional text. At .50 the legend read
    // 3.71:1; the identical colour opaque reads 12.05, so the alpha was the whole defect.
    // contrast: 12.05-12.41:1 vs screenTop,screenBottom [functional]
    inline const juce::Colour scopeLegend      { 0xFFBECDE1 };
    // The -60 dB LEADER LINE, not text. scopeLegendDim used to serve both, so raising the text
    // off the floor would have dragged a hairline with it - split by what each describes, per
    // the root CLAUDE.md's one-constant-one-meaning rule. Alpha on a rule is furniture and stays.
    inline const juce::Colour scopeLeaderLine  { juce::Colour::fromRGBA (190, 205, 225, 102) }; // .40
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
    /** U+2212 MINUS SIGN, which GUI-SPEC.md section 8 requires in both printed scales and readouts
        rather than the ASCII hyphen - a hyphen is visibly shorter and sits lower than the digits it
        precedes, so "-12" reads as smaller type than "+12" beside it.

        Built from a codepoint for the same reason middleDot() is: juce::String's const char*
        constructor decodes as LATIN-1, not UTF-8, so a "\xe2\x88\x92" literal renders as three
        stray glyphs on the panel. */
    /** U+2014 EM DASH, from a codepoint for the same Latin-1 reason as the others. */
    inline juce::String emDash()
    {
        return juce::String::charToString ((juce::juce_wchar) 0x2014);
    }

    inline juce::String minusSign()
    {
        return juce::String::charToString ((juce::juce_wchar) 0x2212);
    }

    /** Printed numerals carry ASCII hyphens in the mark tables, because those are constexpr
        `const char*`. This swaps them for the real minus at draw time. */
    inline juce::String withRealMinus (const char* text)
    {
        return juce::String (text).replaceCharacter ('-', (juce::juce_wchar) 0x2212);
    }

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
    // GUI-SPEC.md section 1. Confirmed against the 2026-08-12 screenshots, which are 2680 x 1298 -
    // the artwork's own figure, not the doc's prose. v1.0's 1200 x 615 is retired along with its
    // three-column body; v1.1 is four columns.
    //
    // **649, and the 645 it replaces was measured off un-fonted renders.** The earlier screenshots
    // were captured before the webfonts applied; three unpinned `line-height` blocks lay out ~3.5px
    // shorter under the fallback face, so the whole panel came out 1290 at 2x instead of 1298.
    //
    // **Every coordinate below was already right and none of them moved.** That is the part worth
    // recording, because the obvious reading of "the canvas grew by 4" is that the layout shifted.
    // It did not - the build's own row-by-row correlation against the old renders had reported
    // "1px in the header, the rest below", and this pass confirms the build was describing the
    // panel while the render was describing a font that had not loaded. Measured landmark by
    // landmark against old and new:
    //
    //     header top        16.0    old 16   new 16    unmoved in both
    //     programWellY      61.0    old 60   new 61    build already matched the NEW render
    //     scope bezel top  164.6    old 164  new 165        "
    //     scopeScreenY     170.6    old 170  new 171        "
    //     scope bezel base 346.6    old 346  new 347        "
    //
    // So this is a one-constant change. If a future render disagrees with the build again, measure
    // landmarks before moving coordinates: the render is not automatically the authority when it
    // may have been shot in a different font state.
    inline constexpr float canvasWidth  = 1340.0f;
    inline constexpr float canvasHeight = 649.0f;
    inline constexpr float panelRadius  = 10.0f;
    inline constexpr float panelPadding = 14.0f;

    /** The overlay texture: 1px scanlines every 3px, plus a top-left radial sheen. */
    /** The sheen's peak opacity. Section 1 states 0.5; this runs 0.40, chief designer's call.
        
        It briefly went to 0.90 while the sheen looked absent on the fascia. That was treating the
        symptom: the real fault was that the texture overlay was painted UNDER the header bezel
        instead of over everything, so the place the effect is most visible never received it at
        all. White lifts a #22304c bezel by around 90 levels and an already-near-white fascia by
        about 17, which is why "I can barely see it on the beige" and "it is obvious on the blue"
        are the same observation.
        
        Back-solved from the artwork's own bezel - peak 142 over a 50.7 base - the overlay measures
        alpha 0.45, so 0.5 was very close to the render and needed no adjustment once the LAYERING
        was fixed. 0.40 is a deliberate step below both: the effect is now carried mostly by the
        header bezel, where it is strong, and the render's own level reads slightly hot there. */
    inline constexpr float sheenAlpha = 0.40f;

    inline constexpr float scanlinePitch = 3.0f;

    // --- Header bezel (measured: fill x 15..1185, y 15..118, 1px border outside) -------------
    // GUI-SPEC.md section 1's region table.
    inline constexpr float headerX = 16.0f;
    inline constexpr float headerY = 16.0f;
    inline constexpr float headerW = 1308.0f;
    inline constexpr float headerH = 104.0f;
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
    // Section 9. All three column captions - PROGRAM, IN, OUT - sit on ONE line at y 41, and the
    // LCD cell and both meter wells share a single 33px band at y 61. Those two shared baselines
    // are the point: the header previously had the captions and wells on three different rows, so
    // nothing lined up across the columns.
    inline constexpr float programLabelX = 357.0f;
    inline constexpr float programLabelY = 41.0f;
    inline constexpr float programLabelH = 12.0f;
    inline constexpr float programWellX = 357.0f;
    inline constexpr float programWellY = 61.0f;
    inline constexpr float programWellW = 641.0f;

    /** **34, and it is the suite's figure rather than this panel's.** BRAND.md fixes the header
        part height at 34px in every casting - not a proportion of the panel, because the castings
        are differently-sized units rather than scales of one design, and a manufacturer uses the
        same physical part across a product line. The LCD, both Program buttons and both meter
        wells all measure it; see headerButtonH and meterWellH, which follow this rather than
        repeating the number.

        Border-box, so the 1px border is inside it and the content is 32. That distinction is not
        pedantry here: reconstructing border-box from a content figure by adding padding is where
        four castings picked up 0.5-2px of drift between row-mates. */
    inline constexpr float programWellH = 34.0f;

    /** The bank indicator is printed ON the LCD glass now, not a badge beside it: same 16px face as
        the program name, 16px padding either side, separated from the name by a 1px vertical rule.
        No border, no fill, no radius - it is text on the display, and there is no separate bank
        control anywhere on the panel. */
    inline constexpr float lcdBankPadX = 16.0f;
    inline constexpr float lcdRuleInsetY = 7.0f;
    /** **The LCD's general type: the FACT/USER bank tag and the Program dropdown's rows.** 16px is
        the approved size for both and must stay 16 - ReflectLookAndFeel's menu font reasons about
        matching "the LCD's 16" explicitly.

        **Not the program name.** That is drawn a point larger; see lcdNameTextSize. One constant
        used to serve all three, declared at 16 while ProgramHeader hard-coded 17 at the name's draw
        call - so raising the declaration to match what the NAME drew would have taken the bank tag
        and every menu row up with it, which is a visual change to two things nobody asked to
        change. They are separate constants now because they are separate decisions. */
    inline constexpr float lcdTextSize = 16.0f;
    inline constexpr float lcdTextTracking = 0.13f;

    /** **The Program dropdown's rows follow the bank tag, deliberately - raise one and you raise
        the other.** The alias is here rather than a bare reuse of lcdTextSize so that the
        dependency is visible at the point someone would change it.

        The list is an extension of the display it drops out of: anchored flush to the glass, at the
        glass's width, showing the same Program names in the same phosphor. At GUI-SPEC.md section
        9's 13px against the LCD's 16 it read as a different, smaller thing hanging off the bar
        rather than the bar continuing downward - and 13px is small for a bank you scan rather than
        read. **Matched to 16 as a deliberate deviation from section 9, taken by the chief
        designer**; the 13px is logged for the designers to fold in. It also makes the list taller,
        so more of the bank sits behind the scroll cap; that is the accepted cost.

        This reasoning used to live only in ReflectLookAndFeel::getPopupMenuFont, where someone
        editing this header would never meet it - and it was silently invalidated once, when
        lcdTextSize was raised to 17 to match the program name and took every menu row with it. The
        dependency was never the problem; its being invisible from here was. */
    inline constexpr float menuRowTextSize = lcdTextSize;

    /** **The program name, which is drawn at 17px / .16em and always has been.** The theme simply
        did not say so: ProgramHeader carried these as literals at the draw call while the theme
        declared 16 / .13, and the character budget was computed from the declaration rather than
        from what rendered. Declaring them here changes nothing on screen - it makes the theme
        describe the panel. */
    inline constexpr float lcdNameTextSize = 17.0f;
    inline constexpr float lcdNameTextTracking = 0.16f;
    inline constexpr float lcdChevronInsetRight = 12.0f;

    /** Section 9's capacity note: the cell holds 36 characters at 16px against a longest readout of
        19 ("DIGITAL GRAIN: 100"), so the live readout never needs the name cell to widen. */
    /** **37, MEASURED against the font the paint path draws**, not quoted from the spec.
        Tests/DisplayBudgetTests.cpp recomputes it from these very constants each run: the name area
        is 474.0px once the runtime bank cell and the chevron inset are taken off the well, and
        IBM Plex Mono at 17px / .16em advances 12.78px.

        It read 36, derived at the theme's declared 16px / .13em - which is not what was drawn. */
    inline constexpr int lcdCharacterBudget = 41;

    /** **How this panel spells the LCD parameter readout.**

        A presentation decision, so it lives with the other presentation constants - and that
        placement is load-bearing for the test: ProgramHeader.h reaches the processor, which needs
        JucePlugin_* macros that only exist in the plugin target, so a test reading the format from
        there could not link. The test must read the SHIPPING format rather than a copy, or it
        asserts against itself.

        **`asAuthored`, and this casting is the reason the alternative is named `ValueCase::all`
        rather than something that sounds harmless.** The parameters here bake their unit into the
        value text, so upper-casing it printed `DAMPING HF: 4.8 KHZ`, `DECAY: 4.6 S` and
        `OUTPUT TRIM: +2.5 DB`. A capital S is a different unit from a lowercase one and KHZ is not
        a unit at all.

        The revert is core's 900 ms, which is what this panel already used - `lcdReadoutHoldMs` is
        replaced by this rather than deleted silently, so a reader looking for the old constant
        finds out where it went rather than concluding the revert was removed. */
    inline nf::ReadoutFormat readoutFormat()
    {
        nf::ReadoutFormat f;
        f.nameCharacterBudget = lcdCharacterBudget;
        return f;
    }

    /** **Measured off screenshots/header/04-user-edited-save-delete-lit.png at 3x**, not derived
        from the header's padding, and the whole row closes on itself: 357 + 641 = 998, +8 -> SAVE
        1006 + 62 = 1068, +8 -> DELETE 1076 + 70 = 1146, +16 -> IN at **1162**, which is where the
        render puts the meter well to the pixel. A chain that lands on an independently measured
        edge is the check that the row is right rather than merely plausible.

        The two buttons differ in width **by design** - each is sized by its longest legend, STORE
        at 5 characters and DELETE at 6. Only the 34px height is shared. If a content size ever
        changes, take the difference out of padding: the 34 is the number that stays put.

        The DELETE-to-IN gap is 16px against 10px between the two meter wells, so the meters read
        as their own pair rather than as two more buttons. Both gaps were measured off the render
        rather than assumed - the 10 lands exactly (well border ends 1226.99, next begins 1237.00).

        One pixel is unresolved and recorded rather than smoothed over: the render draws the IN
        well 65 wide and the OUT well 64, which cannot both be right on a row whose spec table
        gives one figure for the pair. 64 is taken from the spec; the odd pixel is sub-pixel
        rounding in the export and is raised with the designers. */
    inline constexpr float saveButtonX = 1006.0f;
    inline constexpr float saveButtonW = 62.0f;
    inline constexpr float deleteButtonX = 1076.0f;
    inline constexpr float deleteButtonW = 70.0f;
    inline constexpr float headerButtonY = programWellY;
    inline constexpr float headerButtonH = programWellH;
    inline constexpr float lcdRadius = 3.0f;

    /** The two stacked legends. 10px is BRAND.md's floor for functional text and **both** legends
        are functional, so neither is set smaller than the other to fit - the pair is what sets the
        34px height (2 x 10px ink + leading + padding needs ~27px). */
    inline constexpr float legendTextSize = 10.0f;
    inline constexpr float legendTracking = 0.20f;
    inline constexpr float legendLineHeight = 12.0f;
    inline constexpr float legendGap = 1.0f;

    // FACT/USER badge sits 14px in from the well's left edge; the chevron 11px from its right.
    inline constexpr float badgeInsetX = 14.0f;
    inline constexpr float badgeW = 40.0f;
    inline constexpr float badgeH = 18.0f;
    inline constexpr float chevronInsetX = 11.0f;
    inline constexpr float chevronSize = 9.0f;

    // IN / OUT meters. Same caption line and same band as the LCD - meterWellH follows
    // programWellH rather than repeating 34, because they are the same decision, not two that
    // happen to agree. See programWellH for why that height is the suite's and not this panel's.
    inline constexpr float meterLabelY = 41.0f;
    inline constexpr float meterLabelH = 12.0f;
    inline constexpr float meterWellY = programWellY;
    inline constexpr float meterWellW = 64.0f;
    inline constexpr float meterWellH = programWellH;
    inline constexpr float meterInX = 1162.0f;
    inline constexpr float meterOutX = 1236.0f;

    // --- Body row ------------------------------------------------------------
    // Panel padding box minus the body row's own `padding: 20px 4px 6px`.
    inline constexpr float bodyTop = 139.0f;
    inline constexpr float bodyLeft = 18.0f;
    inline constexpr float bodyRight = 1182.0f;

    // Columns. Widths include each column's own padding, because the design's inner elements are
    // content-box: `width: 336px; padding-right: 22px` occupies 358px.
    // GUI-SPEC.md section 1's region table. FOUR columns in v1.1, not three: DAMPING was lifted out
    // of the tank column and given its own home alongside ALGORITHM, which is what let the pair be
    // promoted from 44px to 52px and keep their full mark sets.
    //
    //   Col 1  ALGORITHM + DAMPING     18 .. 318
    //   Col 2  REVERB TANK            319 .. 609
    //   Col 3  TANK LIVE + CHARACTER  610 .. 1145
    //   Col 4  OUTPUT                1146 .. 1322
    inline constexpr float col1X = 18.0f;
    inline constexpr float col1W = 300.0f;
    // Column 1's CONTENT centre is 158, not the column's geometric 168: the ALGORITHM rotary and
    // the damping pair are both centred there (the two damping dials at 94 and 222 have their
    // midpoint at 158), and the DAMPING pill sits on the same axis. Measured off 01-panel.png.
    inline constexpr float col1Centre = 158.0f;
    inline constexpr float col1GeometricCentre = col1X + col1W * 0.5f;  // 168, the box, not the content

    inline constexpr float col2X = 319.0f;
    inline constexpr float col2W = 290.0f;
    inline constexpr float col2Centre = col2X + col2W * 0.5f;           // 464

    inline constexpr float col3X = 610.0f;
    inline constexpr float col3W = 535.0f;
    inline constexpr float col3Centre = col3X + col3W * 0.5f;           // 877.5

    inline constexpr float col4X = 1146.0f;
    inline constexpr float col4W = 176.0f;
    inline constexpr float col4Centre = col4X + col4W * 0.5f;           // 1234

    inline constexpr float divider1X = 318.0f;
    inline constexpr float divider2X = 609.0f;
    inline constexpr float divider3X = 1145.0f;
    inline constexpr float dividerTop = 137.0f;
    inline constexpr float dividerBottom = 626.0f;                      // 137 + 489

    // Kept as aliases so the many call sites reading "left/centre/right column" still compile and
    // still mean something with four columns: the tank column is the one they referred to as the
    // left, CHARACTER shares col 3 with the scope, and OUTPUT is still the rightmost.
    inline constexpr float leftColumnX = col2X;
    inline constexpr float leftColumnW = col2W;
    inline constexpr float leftColumnCentre = col2Centre;

    inline constexpr float centreColumnX = col3X;
    inline constexpr float centreColumnW = col3W;
    inline constexpr float centreColumnCentre = col3Centre;

    inline constexpr float rightColumnX = col4X;
    inline constexpr float rightColumnW = col4W;
    inline constexpr float rightColumnCentre = col4Centre;

    // --- ALGORITHM rotary (measured: body x 134..238, y 161..265) -------------
    // GUI-SPEC.md section 5's dial-centre table - a tick-arc centre like the knobs, and the rotary
    // is 104px so the radius stays 52.
    inline constexpr float algoCentreX = 158.0f;
    inline constexpr float algoCentreY = 275.6f;
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
    inline constexpr std::array<float, 4> algoDetentDegrees { { -135.0f, -45.0f, 45.0f, 135.0f } };
    inline constexpr float algoPointerTopInset = 9.0f;
    inline constexpr float algoPointerWidth = 3.0f;
    inline constexpr float algoPointerLengthFraction = 0.36f;   // of the radius
    // Below the rotary's own tick ring, measured off 01-panel.png. It moved with the knob when
    // ALGORITHM's centre went to 275.6, and was left behind at the v1.0 value - which is why the
    // caption vanished off the bottom of column 1 rather than landing somewhere obviously wrong.
    inline constexpr float algoCaptionY = 336.0f;
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
    // tick angles, so these are copied rather than derived.
    //
    // **Tests/PrintedScaleTests.cpp asserts every numeral below against ParamFormat** - the mapping
    // that actually drives the pointer - so a curve changing without its ring fails a build. This
    // comment previously claimed that test existed when it did not, which is worse than claiming
    // nothing: it stops the next reader looking.
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

    inline constexpr float tankPillY = 221.0f;
    inline constexpr float characterPillY = 360.0f;
    inline constexpr float outputPillY = 137.0f;

    // The horizontal rule inside col 1, separating ALGORITHM from DAMPING (section 1: y 269,
    // x 18 -> 300).
    // DAMPING is a section PILL in v1.1, on column 1's content axis - not the 9px label set to the
    // left of the pair that v1.0 had. Measured at y 383, centre 158.
    inline constexpr float dampingPillY = 383.0f;

    // **Measured at 368, not section 1's stated 269.** The rotary's body spans y 224..328, so 269
    // draws this rule straight through the middle of the ALGORITHM knob - which is what it did.
    // 368 is the gap between the knob's bottom and the DAMPING pill at 383, the only place a
    // separator between those two groups can sit. The artwork outranks the prose, and its dip at
    // 368 (169.7 against a 210 surround, with the groove's white highlight at 369) is unambiguous.
    inline constexpr float leftDividerY = 368.0f;           // engraved line + 1px white below

    // DAMPING is set to the LEFT of its knob pair, not above it:
    // `right: calc(50% + 70px); top: 18px` within the damping block.
    inline constexpr float dampingLabelRight = leftColumnCentre - 70.0f;   // 116
    inline constexpr float dampingLabelY = 533.0f;
    inline constexpr float dampingLabelSize = 9.0f;
    inline constexpr float dampingLabelTracking = 0.24f;

    // --- TANK LIVE scope (measured: screen border box x 407..953, y 174..344) -
    inline constexpr float ledX = 632.0f;
    inline constexpr float ledY = 136.6f;
    inline constexpr float ledSize = 15.0f;
    inline constexpr float ledGlowRadius = 26.0f;
    inline constexpr float ledLabelX = 427.0f;              // 11px gap after the lamp
    inline constexpr float ledLabelSize = 11.0f;
    inline constexpr float ledLabelTracking = 0.26f;
    inline constexpr float scopeHeaderY = 136.6f;
    inline constexpr float scopeHeaderH = 15.0f;

    /** The TANK LIVE lamp - the plugin's ONE LED, per BRAND.md's one-accent rule. Section 11 puts
        it at x 632, the scope column's left edge, on the header row. */

    inline constexpr float scopeHeaderTextSize = 10.0f;
    inline constexpr float scopeHeaderTracking = 0.20f;
    inline constexpr float scopeHeaderGap = 18.0f;

    inline constexpr float scopeBezelX = 632.0f;
    inline constexpr float scopeBezelY = 163.6f;
    inline constexpr float scopeBezelW = 491.0f;
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
    // Measured off 01-panel.png: the stamp's ink runs to x 1320 with its cap band centred on
    // y 619.2, so the 13px line it is drawn in starts at 613. It had been left at the v1.0
    // coordinates, which on the wider panel put it beside OUTPUT TRIM's label rather than in the
    // panel's own bottom-right corner.
    inline constexpr float versionRight = 1320.0f;
    inline constexpr float versionY = 613.0f;
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

        // Nudged down by half the descent so the CAP INK is centred, not the font box.
        //
        // Every pill label is all-caps and has no descenders, but vertical centring works on the
        // font's full height - ascent plus descent - so the empty descender space pushes the visible
        // glyphs up by half of it. It reads as the label sitting high in its pill, which is exactly
        // what it was doing.
        Text::drawTracked (g, text, font, tracking, r.translated (0.0f, font.getDescent() * 0.5f),
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
