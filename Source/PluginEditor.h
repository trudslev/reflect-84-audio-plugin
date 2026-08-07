#pragma once

#include "PluginProcessor.h"
#include "GUI/ReflectEditorContent.h"

/**
    A thin shell. It owns one ReflectEditorContent drawn at the fixed reference canvas size and
    applies a single uniform scale transform to it, with the constrainer locking the aspect ratio.

    This is why nothing else in the GUI has a resized(): every component lays out once against
    ReflectTheme::Layout's absolute coordinates and never learns the real window size.
*/
class Reflect84AudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit Reflect84AudioProcessorEditor (Reflect84AudioProcessor&);
    ~Reflect84AudioProcessorEditor() override = default;

    void resized() override;

private:
    ReflectEditorContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reflect84AudioProcessorEditor)
};
