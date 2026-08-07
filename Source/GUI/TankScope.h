#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ReflectTheme.h"

#include <array>

class Reflect84AudioProcessor;

/**
    TANK LIVE - the signature element.

    Covers the whole centre-column scope section: the header row (the plugin's single LED plus its
    DECAY TAIL / RT60 / timebase legends), the scope bezel, and the screen with its grid, noise
    tail, decay envelope and corner legends.

    The envelope is SYNTHESISED from parameter values, not measured from audio. That is what
    design/README.md section 6 specifies - an exponential decay from DECAY, wobbled by MODULATION,
    scaled by DENSITY, quantised by DIGITAL GRAIN, swept left to right - and it is the right model
    here: it is deterministic, it matches the approved artwork exactly, and it keeps drawing when
    no audio is playing. The noise hairlines behind it are seeded once at construction, never
    per frame.

    The one thing that does come from the audio thread is the lamp: it reports that the tank is
    still decaying, which is BRAND.md's rule of exactly one LED per plugin carrying the most
    important live state.

    Crucially, the stair-stepping is not a lookalike. Both the step count and the x-axis step come
    from GrainSpec, the same struct the audio-path quantiser derives its word length and
    sample-and-hold factor from - so what the screen shows and what the tank does cannot drift
    apart.
*/
class TankScope final : public juce::Component,
                        private juce::Timer
{
public:
    explicit TankScope (Reflect84AudioProcessor& processor);
    ~TankScope() override;

    void paint (juce::Graphics& g) override;

    static juce::Rectangle<int> canvasBounds();

private:
    void timerCallback() override;

    void paintHeader (juce::Graphics& g, float decaySeconds);
    void paintScreen (juce::Graphics& g);

    /** Maps a point in design/README.md's 600 x 168 viewBox onto the screen's real content area.
        The design's SVG uses preserveAspectRatio="none", so x and y scale independently. */
    juce::Point<float> fromViewBox (float x, float y) const;

    Reflect84AudioProcessor& processorRef;

    /** 0-1 sweep position. Animation only - explicitly NOT an automatable parameter. */
    float phase = 0.0f;

    float lampLevel = 0.0f;

    /** Fixed random hairline heights, generated once. Regenerating these per frame would turn a
        decaying tail into television static. */
    std::array<float, (size_t) ReflectTheme::Layout::scopeNoiseLines> noise {};

    juce::Rectangle<float> screenContentArea() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TankScope)
};
