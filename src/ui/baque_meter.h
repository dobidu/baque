#pragma once
#include "../audio/ui_state_snapshot.h"

#include <juce_gui_basics/juce_gui_basics.h>

class BaqueMeter : public juce::Component, private juce::Timer {
  public:
    explicit BaqueMeter(const UiStateSnapshot& snapshot);
    ~BaqueMeter() override = default;

    void paint(juce::Graphics& g) override;
    void visibilityChanged() override;

    [[nodiscard]] bool isTimerRunning() const noexcept { return juce::Timer::isTimerRunning(); }

  private:
    static constexpr int k_segments = 12;
    static constexpr float k_decay_per_tick = 0.92f;

    const UiStateSnapshot& snapshot_;
    float display_l_ = 0.0f;
    float display_r_ = 0.0f;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueMeter)
};
