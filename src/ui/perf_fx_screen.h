#pragma once

#include "baque_knob.h"
#include "baque_look_and_feel.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <functional>
#include <memory>

class BaqueProcessor;

// KNOWN LIMITATIONS (v1):
// - group_mute_[]/group_solo_[] are message-thread-local; diverge from engine
//   PerfState after setStateInformation (preset load). Deferred to Phase 10/11.
// - STUTTER/RISER/CRUSH/FILL have no engine APVTS params in v1; shown as greyed stubs.
// - Scene Morph deferred to Phase 10/11.
// juce::Component already inherits juce::MouseListener — no additional base class.
class PerfFxScreen : public juce::Component {
  public:
    PerfFxScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~PerfFxScreen() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;

    // Scatter section (left column)
    BaqueKnob scatter_type_knob_;
    BaqueKnob scatter_depth_knob_;
    juce::Label scatter_type_label_;
    juce::Label scatter_depth_label_;
    // Attachments AFTER knobs — destroyed first (correct RAII order)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scatter_type_attach_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> scatter_depth_attach_;

    // Momentary pads (center column): TAPE STOP + GATE wired via mouseDown/mouseUp;
    // remaining 7 slots are greyed visual stubs (no engine params in v1)
    juce::TextButton tape_stop_btn_{"TAPE STOP"};
    juce::TextButton gate_btn_{"GATE"};
    std::array<juce::Label, 7> perf_labels_;

    // mouseDown/mouseUp via Component's existing MouseListener base (via addMouseListener).
    // DO NOT add explicit private juce::MouseListener — Component already provides it.
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // XY pad (right column, top): drag sets filter_cutoff (x) × filter_res (y);
    // mouseUp or visibilityChanged-to-false resets both to APVTS defaults.
    class XyPad : public juce::Component {
      public:
        std::function<void(float, float)> on_drag;   // (x_norm [0,1], y_norm [0,1])
        std::function<void()>             on_release;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override; // SR2: click-only fires on_drag
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;

      private:
        float cursor_x_ = 0.5f;
        float cursor_y_ = 0.5f;
    };
    XyPad xy_pad_;

    // Mute/solo groups (right column, bottom): 4 groups × M + S buttons
    static constexpr const char* k_group_names[4] = {
        "KICK/SUB", "SNARE/CLAP", "CHIMBAIS", "PERC/VOX"};
    std::array<juce::TextButton, 4> mute_group_btns_;
    std::array<juce::TextButton, 4> solo_group_btns_;
    std::array<bool, 4> group_mute_{};
    std::array<bool, 4> group_solo_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerfFxScreen)
};
