#pragma once

#include "baque_look_and_feel.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

class BaqueProcessor;

// KNOWN LIMITATIONS (v1):
// - channel_state_[] is message-thread-local; diverges from proc_.lane_routing_
//   after setStateInformation (preset load). timerCallback re-syncs note_labels_
//   but NOT mode toggles (to avoid flicker with in-flight UiCommands).
// - LEARN buttons are visual-only; engine lane-note MIDI learn deferred to Phase 10/11.
// - CC-out table not shown (cc_out_ not in UiStateSnapshot); deferred to Phase 10-07.
class MidiScreen : public juce::Component, private juce::Timer {
  public:
    MidiScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~MidiScreen() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    [[nodiscard]] bool isTimerRunning() const noexcept { return juce::Timer::isTimerRunning(); }

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;

    // Left panel: clock master + hardware template controls
    juce::TextButton clock_master_btn_{"CLOCK MASTER"};
    bool clock_master_state_ = false;
    juce::TextButton tr8_btn_{"Apply TR-8"};
    juce::TextButton tr8s_btn_{"Apply TR-8S"};

    // Right panel: 16-lane routing table
    std::array<juce::Label, 16>      lane_labels_;    // "LANE N" — read-only
    std::array<juce::Label, 16>      note_labels_;    // MIDI note name — advisory
    std::array<juce::TextButton, 16> mode_int_;
    std::array<juce::TextButton, 16> mode_ext_;
    std::array<juce::TextButton, 16> mode_both_;
    std::array<juce::TextButton, 16> channel_btns_;  // cycles 1-16 on click
    std::array<int, 16>              channel_state_{};
    std::array<juce::TextButton, 16> learn_btns_;    // visual-only in v1

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiScreen)
};
