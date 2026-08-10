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
            knob->onDragStart = [this, param] { programHeader.showParameter (*param); };
            knob->onDragEnd   = [this]        { programHeader.releaseParameter(); };

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
    const int hostTop = ProgramHeader::menuHostTop();
    menuHost.setBounds (0, hostTop, getWidth(),
                        juce::jmin (ProgramHeader::menuHostHeight(), getHeight() - hostTop));
    menuHost.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (menuHost);
    menuHost.toFront (false);
    programHeader.setMenuParent (&menuHost);
}

void ReflectEditorContent::paintOverChildren (juce::Graphics& g)
{
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
