#pragma once
#include "../plugin_processor.h"
#include "baque_look_and_feel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class SequencerComponent : public juce::Component, private juce::Timer {
  public:
    explicit SequencerComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~SequencerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Toggles step on/off; pushes set_step UiCommand. Returns push result (false = queue full).
    bool toggleStep(int lane, int step);
    void setMode(bool todas) noexcept;
    void setFocusedLane(int lane) noexcept;

  private:
    static constexpr int k_lanes = 16;
    static constexpr int k_steps = 16;
    static constexpr int k_label_w = 48;
    static constexpr int k_toggle_h = 28;

    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;
    bool todas_mode_ = true;
    int focused_lane_ = 0;
    int current_step_ = -1;
    std::array<juce::TextButton, 2> mode_buttons_; // [0]=TODAS [1]=FOCO

    void timerCallback() override;
    [[nodiscard]] juce::Rectangle<int> stepBounds(int lane, int step) const noexcept;
    [[nodiscard]] juce::Rectangle<int> focusBarBounds(int step) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerComponent)
};
