#pragma once

#include "baque_knob.h"
#include "baque_look_and_feel.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

class BaqueProcessor;

class FxScreen : public juce::Component {
  public:
    FxScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~FxScreen() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    BaqueLookAndFeel& laf_;
    std::array<BaqueKnob, 6> knobs_;
    std::array<juce::Label, 6> labels_;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 6> attachments_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxScreen)
};
