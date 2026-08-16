#include "PanelReadouts.h"

#include "../PluginProcessor.h"

using namespace ReflectTheme;

namespace
{
    /** Formats one knob's value using the same ParamFormat helpers the host-facing parameter text
        uses, so the panel and the DAW's automation lane can never disagree. */
    juce::String formatFor (const char* paramID, float value)
    {
        const juce::String id { paramID };

        if (id == ParamIDs::size)       return ParamFormat::sizeText (value);
        if (id == ParamIDs::decay)      return ParamFormat::decayText (value);
        if (id == ParamIDs::preDelay)   return ParamFormat::preDelayText (value);
        if (id == ParamIDs::density)    return ParamFormat::densityText (value);
        if (id == ParamIDs::dampHF)     return ParamFormat::dampHFText (value);
        if (id == ParamIDs::dampLF)     return ParamFormat::dampLFText (value);
        if (id == ParamIDs::modulation) return ParamFormat::modText (value);
        if (id == ParamIDs::grain)      return ParamFormat::grainText (value);
        if (id == ParamIDs::width)      return ParamFormat::widthText (value);
        if (id == ParamIDs::mix)        return ParamFormat::mixText (value);
        if (id == ParamIDs::trim)       return ParamFormat::trimText (value);

        jassertfalse;   // a knob was added to Layout::knobs without a formatter
        return {};
    }

    /** The meters read in dB and are floored rather than showing "-inf": a rack unit's LED display
        parks at its bottom reading, it does not go blank. */
    juce::String formatMeter (float db)
    {
        return ReflectTheme::formatMeterDb (db);
    }
}

//==============================================================================
PanelReadouts::PanelReadouts (Reflect84AudioProcessor& processor)
    : processorRef (processor)
{
    setInterceptsMouseClicks (false, false);
    refresh();
    startTimerHz (20);
}

PanelReadouts::~PanelReadouts()
{
    stopTimer();
}

bool PanelReadouts::refresh()
{
    bool changed = false;

    for (size_t i = 0; i < Layout::knobs.size(); ++i)
    {
        const auto& spec = Layout::knobs[i];
        const float value = processorRef.apvts.getRawParameterValue (spec.paramID)->load();

        if (auto text = formatFor (spec.paramID, value); text != readouts[i])
        {
            readouts[i] = std::move (text);
            changed = true;
        }
    }

    if (auto text = formatMeter (processorRef.getInputMeterDb()); text != inputText)
    {
        inputText = std::move (text);
        changed = true;
    }

    if (auto text = formatMeter (processorRef.getOutputMeterDb()); text != outputText)
    {
        outputText = std::move (text);
        changed = true;
    }

    return changed;
}

void PanelReadouts::timerCallback()
{
    if (refresh())
        repaint();
}

//==============================================================================
void PanelReadouts::paint (juce::Graphics& g)
{
    // The standing knob readouts are GONE, deliberately - BRAND.md's "No standing numeric readouts
    // under knobs", and GUI-SPEC.md section 7. Hardware panels print a SCALE and you read the
    // pointer against it; the printed scales the knobs now carry are that reference, and live values
    // appear in the PROGRAM LCD only while a control is actually being moved.
    //
    // What stays below is display content rather than panel text: the IN/OUT meter numerals live
    // inside their own LCD wells, and the scope's RT60 header and grain legend belong to the scope.
    // Those are readouts on a screen, not numbers printed on a faceplate.

    // --- IN / OUT wells ------------------------------------------------------
    const auto drawMeter = [&g] (float x, const juce::String& text)
    {
        const juce::Rectangle<float> well { x, Layout::meterWellY, Layout::meterWellW, Layout::meterWellH };

        Paint::drawLcdWell (g, well);

        g.setFont (Font::mono (16.0f));
        g.setColour (Colour::meterText);
        g.drawText (text, well, juce::Justification::centred, false);
    };

    drawMeter (Layout::meterInX, inputText);
    drawMeter (Layout::meterOutX, outputText);
}
