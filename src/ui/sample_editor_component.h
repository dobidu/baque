#pragma once

#include "../audio/pad_bank.h"
#include "../plugin_processor.h"
#include "baque_knob.h"
#include "baque_look_and_feel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

class SampleEditorComponent : public juce::Component {
  public:
    explicit SampleEditorComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~SampleEditorComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setFocusedPad(int pad_idx);

    // Pushes set_pad_play_mode command; returns push result (M4 non-vacuous).
    [[nodiscard]] bool setPlayMode(PlayMode mode);

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;
    int focused_pad_ = 0;
    int knob_drag_pad_ = 0; // SR3: captured at drag-start to prevent mid-drag race

    juce::Label pad_name_label_;
    std::array<juce::TextButton, 3> mode_buttons_;
    std::array<std::unique_ptr<BaqueKnob>, 3> knobs_;
    std::array<juce::Label, 3> knob_labels_;
    juce::Rectangle<int> waveform_bounds_;

    void updateFromPad();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleEditorComponent)
};
