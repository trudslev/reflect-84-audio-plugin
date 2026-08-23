#include "ReflectEditorContent.h"

#include "../PluginProcessor.h"

using namespace ReflectTheme;

ReflectEditorContent::ReflectEditorContent (Reflect84AudioProcessor& processor)
    : processorRef (processor),
      tankScope (processor),
      programHeader (processor),
      panelReadouts (processor)
{
    setLookAndFeel (&lookAndFeel);

    /*  `ABOUT-PART.md`. §9's materials and §1's five strings are everything this casting supplies —
        the geometry, the type, the row order, the dismissal set and the link treatment are all
        `nf::AboutPart`'s, held once so they cannot drift apart across six panels.

        §8: the credits name the faces this casting EMBEDS, not the ones it draws with. All four are
        OFL, which is why the line is one sentence. */
    const nf::AboutMaterials aboutMaterials {
        Colour::aboutGlass, Colour::aboutBody, Colour::aboutDim, Colour::aboutAccent,
        Colour::aboutRing,
        Colour::aboutWellTop, Colour::aboutWellBottom, Colour::aboutWellInk,
        Font::labelTypeface(), Font::labelMediumTypeface(), Font::monoTypeface()
    };

    const nf::AboutContent aboutContent {
        "REFLECT-84", "RF-84",
        NF_VERSION,                 // semver, from PROJECT_VERSION - never a literal
        nf::suiteRelease,           // §1: a separate string, and neither derives from the other
        "github.com/trudslev/reflect-84-audio-plugin",
        "Barlow Condensed, IBM Plex Mono, Share Tech Mono and Jost, "
        "all under the SIL Open Font License."
    };

    aboutBox = std::make_unique<nf::AboutBox> (aboutMaterials, aboutContent);
    aboutTab = std::make_unique<nf::AboutTab> (aboutMaterials, juce::String ("v") + NF_VERSION_SHORT,
                                               Layout::versionSize, Layout::versionTracking);
    aboutTab->onClick = [this] { aboutBox->open(); };

    setSize ((int) Layout::canvasWidth, (int) Layout::canvasHeight);

    panelBackground.setBounds (getLocalBounds());

    /*  §2's law places the tab from the canvas height; §4's places the box. Both are core's, so a
        panel that changes height moves them without either being restated here.

        **Added LAST, and that is not tidiness.** JUCE paints children in the order they were added,
        so registering the tab beside its construction at the top of this constructor put it under
        `panelBackground` — drawn, correct, and invisible. The box has to be above everything for
        the same reason, and its veil covers the whole canvas. */

    addAndMakeVisible (panelBackground);

    for (size_t i = 0; i < Layout::knobs.size(); ++i)
    {
        const auto& spec = Layout::knobs[i];

        auto knob = std::make_unique<ReflectKnob> (spec.size, spec.scale);
        knob->setName (spec.label);
        knob->setCentrePosition ({ spec.centreX, spec.centreY });

        // **Guarded on the control's own drag state, not on the attachment.** A SliderAttachment
        // also fires when a Program is applied and on every host automation step; without this the
        // LCD latches onto whichever parameter moved last and flickers for the length of a song.
        // BRAND.md: "Only direct user manipulation triggers it."
        if (auto* param = processorRef.apvts.getParameter (spec.paramID))
        {
            auto* rawKnob = knob.get();

            knob->onDragStart = [this, param] { programHeader.showParameter (*param); };
            knob->onDragEnd   = [this]        { programHeader.releaseParameter(); };

            // **onValueChange, not just onDragStart.** showParameter renders the value once, so
            // wiring only the drag boundaries left the LCD showing whatever the knob held at the
            // instant it was grabbed - frozen for the whole gesture, then reverting. Every other
            // casting updates here.
            //
            // This is also the only place that can tell a person from automation, which is why the
            // guard's disarm rides along with the hand-off rather than sitting beside it. It was
            // **never called anywhere in this plugin** - so after restoring a session the guard
            // stayed armed indefinitely, and selecting the currently-loaded Program from the host
            // to revert an edit did nothing at all. Through nf::connectUserEdit the two are one
            // call, so that omission is no longer expressible.
            nf::connectUserEdit (*rawKnob, processorRef.userEdits,
                                 [this, param] { programHeader.showParameter (*param); });

            // Every parameter here is stored 0-1, so the default IS the parameter's own default
            // value - no range conversion needed for double-click-to-default.
            knob->setDoubleClickReturnValue (true, (double) param->getDefaultValue());
        }

        addAndMakeVisible (*knob);

        knobAttachments[i] = std::make_unique<APVTS::SliderAttachment> (
            processorRef.apvts, spec.paramID, *knob);

        knobs[i] = std::move (knob);
    }

    addAndMakeVisible (algorithmSwitch);
    algorithmAttachment = std::make_unique<APVTS::SliderAttachment> (
        processorRef.apvts, ParamIDs::algorithm, algorithmSwitch);

    /*  **ALGORITHM reports to the LCD too.** BRAND.md: every control that changes a parameter
        announces itself there, switches included. A rule about which controls are "self-evident"
        is harder to apply consistently than no rule at all - and a detented switch is often the
        LEAST obvious thing on a panel, because turning a knob shows you its own scale while
        flipping a switch shows you nothing.

        Same guard as the knobs, for the same reason: a SliderAttachment fires on Program recall and
        on every host automation step, so without it the LCD latches onto whichever parameter moved
        last. The restore guard's disarm comes with it - this is a real user edit and it has to
        disarm exactly as a knob move does. */
    if (auto* algorithmParam = processorRef.apvts.getParameter (ParamIDs::algorithm))
    {
        nf::connectUserEdit (algorithmSwitch, processorRef.userEdits,
                             [this, algorithmParam] { programHeader.showParameter (*algorithmParam); });

        algorithmSwitch.onDragEnd = [this] { programHeader.releaseParameter(); };
    }

    addAndMakeVisible (tankScope);
    addAndMakeVisible (programHeader);

    // Last, so the live numbers sit above everything - it is mouse-transparent, so nothing below
    // it becomes unreachable.
    panelReadouts.setBounds (getLocalBounds());
    addAndMakeVisible (panelReadouts);

    // After panelReadouts, because the Program list has to sit above the live numbers as well as
    // the panel. It opens inside this, so it can neither move its top edge nor grow past the panel.
    // A SIBLING of programHeader, never a child: that component covers only the header strip, so a
    // list parented there would be clipped to a 432x42 box.
    // **The Program list is a sibling of the header, at the LCD's width, flush off its bottom
    // edge and running to the panel's bottom.** A SIBLING and not a child: ProgramHeader narrows
    // its own hitTest to the header strip, and JUCE stops searching a component's children once
    // its hitTest rejects the point - so a list parented there would be dead everywhere except
    // the strip it drops from.
    programList.setBounds ((int) Layout::programWellX,
                           ProgramHeader::listTopY(),
                           (int) Layout::programWellW,
                           ProgramHeader::listHeight (getHeight()));
    addChildComponent (programList);      // added hidden; the header shows it
    programList.toFront (false);
    programHeader.setProgramList (&programList);

    /*  §2's law places the tab from the canvas height; §4's places the box. Both are core's, so a
        panel that changes height moves them without either being restated here.

        **Registered LAST, and that is not tidiness.** JUCE paints children in the order they were
        added, so registering these beside their construction at the top of this constructor put
        the tab under `panelBackground` — drawn, correct, and invisible in the capture. The box
        needs to be above everything for the same reason: its veil covers the whole canvas. */
    aboutTab->layoutFor (getHeight());
    aboutBox->setBounds (getLocalBounds());
    addAndMakeVisible (*aboutTab);
    addChildComponent (*aboutBox);
}

void ReflectEditorContent::paintOverChildren (juce::Graphics& g)
{
    // The texture overlay goes on FIRST, over every child - section 1 calls it full bleed. It used
    // to be painted inside PanelBackground, which meant the header bezel covered it immediately;
    // and the bezel is where it matters most, because white lifts #22304c by around 90 levels
    // against about 17 on the near-white fascia.
    panelBackground.paintTextureOverlay (g);

    const auto* bypass = processorRef.getBypassParameter();

    if (bypass == nullptr || ! bypass->get())
        return;

    // **A multiply, not an alpha blend.** BRAND.md's Bypass section: multiplying preserves relative
    // contrast and reads as darkness, while blending toward the panel colour reads as fog laid over
    // it. JUCE has no multiply blend mode, but a multiply by a grey k is just "keep k of what is
    // there", which is what drawing opaque black at (1 - k) alpha does.
    //
    // 0.50, matching CHORUS-60. BRAND.md records that 0.70 was tried and read as a dimmer switch
    // rather than a light being out.
    //
    // Nothing else changes: no pointer moves, no control is redrawn, dimmed individually,
    // desaturated or flattened, and the accent is not drained - the LED and the trace darken with
    // everything else, by the same factor. The legibility floors deliberately do not apply here;
    // the panel is not operable in this state and conveying that is the job. No caption either: if
    // a panel needs to print "settings retained", the visual is misleading and should be fixed.
    constexpr float multiply = 0.50f;

    g.setColour (juce::Colours::black.withAlpha (1.0f - multiply));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), Layout::panelRadius);
}

ReflectEditorContent::~ReflectEditorContent()
{
    setLookAndFeel (nullptr);
}
