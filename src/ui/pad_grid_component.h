#pragma once
#include "../plugin_processor.h"
#include "baque_look_and_feel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

class PadGridComponent : public juce::Component, private juce::Timer {
  public:
    explicit PadGridComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~PadGridComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setFocusedPad(int pad_idx) noexcept;
    [[nodiscard]] int focusedPad() const noexcept { return focused_pad_; }
    [[nodiscard]] bool isTimerRunning() const noexcept { return juce::Timer::isTimerRunning(); }

    // Called when the user clicks a pad; PerformScreen wires this to setFocusedPad.
    std::function<void(int)> on_pad_clicked;

  private:
    static constexpr int k_rows = 4;
    static constexpr int k_cols = 4;

    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;
    std::array<float, 16> display_velocity_{};
    int focused_pad_ = 0;

    void timerCallback() override;
    [[nodiscard]] juce::Rectangle<float> padBounds(int idx) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadGridComponent)
};
