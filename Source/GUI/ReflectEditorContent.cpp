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

        auto knob = std::make_unique<ReflectKnob> (spec.size);
        knob->setName (spec.label);
        knob->setCentrePosition ({ spec.centreX, spec.centreY });

        if (auto* param = processorRef.apvts.getParameter (spec.paramID))
        {
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
}

ReflectEditorContent::~ReflectEditorContent()
{
    setLookAndFeel (nullptr);
}
