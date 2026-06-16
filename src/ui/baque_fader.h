#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class BaqueFader : public juce::Slider {
  public:
    BaqueFader() {
        setSliderStyle(juce::Slider::LinearVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }
    ~BaqueFader() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueFader)
};
