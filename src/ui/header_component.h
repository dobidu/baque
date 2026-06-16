#pragma once
#include "../plugin_processor.h"
#include "baque_look_and_feel.h"
#include "baque_meter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

class HeaderComponent : public juce::Component, private juce::Timer {
  public:
    enum Screen { PERFORM = 0, FX, MIX, PERF_FX, BROWSER, MIDI, NUM_SCREENS };

    static constexpr int k_height = 60;

    HeaderComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~HeaderComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void setActiveScreen(Screen s);

    // Called when user clicks a NAV button.
    std::function<void(Screen)> on_screen_changed;

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;
    BaqueMeter meter_;

    std::array<juce::TextButton, NUM_SCREENS> nav_buttons_;

    juce::Label bpm_label_;

    void timerCallback() override;
    void buildNavButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderComponent)
};
