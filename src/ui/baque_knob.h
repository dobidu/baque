#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class BaqueKnob : public juce::Slider {
  public:
    BaqueKnob() {
        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                            juce::MathConstants<float>::pi * 2.75f, true);
    }
    ~BaqueKnob() override = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueKnob)
};
