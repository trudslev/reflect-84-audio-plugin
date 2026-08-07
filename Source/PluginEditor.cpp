#include "PluginEditor.h"

namespace
{
    constexpr int referenceWidth  = (int) ReflectTheme::Layout::canvasWidth;
    constexpr int referenceHeight = (int) ReflectTheme::Layout::canvasHeight;
}

Reflect84AudioProcessorEditor::Reflect84AudioProcessorEditor (Reflect84AudioProcessor& p)
    : AudioProcessorEditor (&p), content (p)
{
    addAndMakeVisible (content);

    setResizable (true, true);

    if (auto* constrainer = getConstrainer())
    {
        constrainer->setFixedAspectRatio ((double) referenceWidth / (double) referenceHeight);
        constrainer->setSizeLimits (referenceWidth / 2, referenceHeight / 2,
                                    referenceWidth * 2, referenceHeight * 2);
    }

    // The reference canvas is wider than most screens are tall at 1:1, so open at 80% rather than
    // full size and let the user scale up.
    setSize (juce::roundToInt (referenceWidth * 0.8), juce::roundToInt (referenceHeight * 0.8));
}

void Reflect84AudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) referenceWidth;

    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, referenceWidth, referenceHeight);
}
