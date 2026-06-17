#pragma once
#include "../audio/feel_pattern.h"
#include "../plugin_processor.h"
#include "baque_look_and_feel.h"

#include <juce_gui_basics/juce_gui_basics.h>

class FeelFieldComponent : public juce::Component, private juce::Timer {
  public:
    explicit FeelFieldComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~FeelFieldComponent() override;
    void paint(juce::Graphics& g) override;
    void resized() override {}
    void visibilityChanged() override;
    [[nodiscard]] bool isTimerRunning() const noexcept { return juce::Timer::isTimerRunning(); }

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;
    int current_step_ = -1;
    // No playing flag — current_step_ == -1 when stopped (UiStateSnapshot contract).
    // The "if (current_step_ >= 0)" guard in paint() is the correct stopped-state check (audit SR1).
    void timerCallback() override;
    [[nodiscard]] juce::Point<float>
    nodePos(int step, float timing_ms, float bpm, float ring_radius, juce::Point<float> centre) const noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeelFieldComponent)
};
