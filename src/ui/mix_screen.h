#pragma once

#include "baque_fader.h"
#include "baque_knob.h"
#include "baque_look_and_feel.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

class BaqueProcessor;

// KNOWN LIMITATION: mute_state_[]/solo_state_[] are message-thread-local.
// After setStateInformation (preset load), these diverge from engine PerfState.
// PerfState mute/solo is not exposed in UiStateSnapshot — deferred to Phase 10/11.
// Buttons show correct state only for commands issued in the current session.
class MixScreen : public juce::Component, private juce::Timer {
  public:
    MixScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~MixScreen() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    [[nodiscard]] bool isTimerRunning() const noexcept { return juce::Timer::isTimerRunning(); }

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;

    std::array<BaqueFader, 16> lane_faders_;
    std::array<BaqueKnob, 16> lane_pans_;
    std::array<juce::TextButton, 16> mute_btns_;
    std::array<juce::TextButton, 16> solo_btns_;
    std::array<bool, 16> mute_state_{};
    std::array<bool, 16> solo_state_{};
    std::array<float, 16> lane_meter_{};

    BaqueFader master_fader_;
    float master_meter_l_ = 0.0f;
    float master_meter_r_ = 0.0f;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> master_attachment_;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixScreen)
};
