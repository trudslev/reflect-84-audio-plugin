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

    setSize ((int) Layout::canvasWidth, (int) Layout::canvasHeight);

    panelBackground.setBounds (getLocalBounds());
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
            // This is also the only place that can tell a person from automation, which is why
            // noteUserEdit belongs here and nowhere else. It disarms the processor's
            // justRestoredState guard, and it was **never called anywhere in this plugin** -
            // so after restoring a session the guard stayed armed indefinitely, and selecting the
            // currently-loaded Program from the host to revert an edit did nothing at all.
            // A ValueTree listener cannot be used instead: it fires for automation too, and a host
            // that writes automation on load before replaying its remembered program would disarm
            // the guard exactly when it is needed.
            knob->onValueChange = [this, param, rawKnob]
            {
                if (! rawKnob->isMouseButtonDown())
                    return;

                processorRef.noteUserEdit();
                programHeader.showParameter (*param);
            };

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
        last. noteUserEdit belongs here as well - this is a real user edit and it has to disarm the
        restore guard exactly as a knob move does. */
    if (auto* algorithmParam = processorRef.apvts.getParameter (ParamIDs::algorithm))
    {
        algorithmSwitch.onValueChange = [this, algorithmParam]
        {
            if (! algorithmSwitch.isMouseButtonDown())
                return;

            processorRef.noteUserEdit();
            programHeader.showParameter (*algorithmParam);
        };

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
