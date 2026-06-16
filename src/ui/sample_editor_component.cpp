#include "sample_editor_component.h"

#include "../audio/ui_command_queue.h"

#include <algorithm>

static constexpr const char* k_mode_labels[3] = {"ONE-SHOT", "GATE", "LOOP"};
static constexpr const char* k_knob_labels[3] = {"PITCH", "DECAY", "PAN"};

SampleEditorComponent::SampleEditorComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc)
    , laf_(laf) {
    pad_name_label_.setText("PAD 1", juce::dontSendNotification);
    pad_name_label_.setFont(juce::Font(juce::FontOptions(laf_.monoTypeface()).withHeight(14.0f)));
    addAndMakeVisible(pad_name_label_);

    for (int i = 0; i < 3; ++i) {
        mode_buttons_[i].setButtonText(k_mode_labels[i]);
        mode_buttons_[i].setClickingTogglesState(true);
        addAndMakeVisible(mode_buttons_[i]);
    }
    mode_buttons_[0].setToggleState(true, juce::dontSendNotification);

    mode_buttons_[0].onClick = [this] { setPlayMode(PlayMode::one_shot); };
    mode_buttons_[1].onClick = [this] { setPlayMode(PlayMode::gate); };
    mode_buttons_[2].onClick = [this] { setPlayMode(PlayMode::loop); };

    for (int i = 0; i < 3; ++i) {
        knobs_[i] = std::make_unique<BaqueKnob>();
        addAndMakeVisible(*knobs_[i]);

        knob_labels_[i].setText(k_knob_labels[i], juce::dontSendNotification);
        knob_labels_[i].setFont(juce::Font(juce::FontOptions(laf_.monoTypeface()).withHeight(10.0f)));
        knob_labels_[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(knob_labels_[i]);
    }

    // PITCH: ±24 semitones
    knobs_[0]->setRange(-24.0, 24.0, 1.0);
    knobs_[0]->setValue(0.0, juce::dontSendNotification);
    knobs_[0]->onDragStart = [this] { knob_drag_pad_ = focused_pad_; };
    knobs_[0]->onValueChange = [this] {
        proc_.push_ui_command({UiCommandType::set_pad_pitch_semitones,
                               knob_drag_pad_,
                               0,
                               static_cast<int32_t>(knobs_[0]->getValue()),
                               0.0f});
    };

    // DECAY: 0-2000ms
    knobs_[1]->setRange(0.0, 2000.0, 1.0);
    knobs_[1]->setValue(0.0, juce::dontSendNotification);
    knobs_[1]->onDragStart = [this] { knob_drag_pad_ = focused_pad_; };
    knobs_[1]->onValueChange = [this] {
        proc_.push_ui_command(
            {UiCommandType::set_pad_adsr_decay, knob_drag_pad_, 0, 0, static_cast<float>(knobs_[1]->getValue())});
    };

    // PAN: -100..+100 (maps to -1..+1)
    knobs_[2]->setRange(-100.0, 100.0, 1.0);
    knobs_[2]->setValue(0.0, juce::dontSendNotification);
    knobs_[2]->onDragStart = [this] { knob_drag_pad_ = focused_pad_; };
    knobs_[2]->onValueChange = [this] {
        proc_.push_ui_command(
            {UiCommandType::set_pad_pan, knob_drag_pad_, 0, 0, static_cast<float>(knobs_[2]->getValue()) / 100.0f});
    };
}

void SampleEditorComponent::resized() {
    auto r = getLocalBounds().reduced(6);
    pad_name_label_.setBounds(r.removeFromTop(24));

    auto mode_row = r.removeFromTop(28);
    const int btn_w = mode_row.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
        mode_buttons_[i].setBounds(mode_row.removeFromLeft(btn_w).reduced(2));

    const int wf_h = r.getHeight() * 40 / 100;
    waveform_bounds_ = r.removeFromTop(wf_h);

    const int knob_w = r.getWidth() / 3;
    const int knob_size = std::min(knob_w - 8, r.getHeight() - 20);
    for (int i = 0; i < 3; ++i) {
        auto col = r.removeFromLeft(knob_w);
        knobs_[i]->setBounds(col.removeFromTop(knob_size).reduced(4));
        knob_labels_[i].setBounds(col.reduced(2));
    }
}

void SampleEditorComponent::paint(juce::Graphics& g) {
    g.fillAll(laf_.surface());
    g.setColour(laf_.surface().brighter(0.3f));
    g.fillRect(waveform_bounds_.toFloat());
    g.setColour(laf_.text().withAlpha(0.4f));
    g.setFont(juce::Font(juce::FontOptions(laf_.monoTypeface()).withHeight(10.0f)));
    g.drawText("~ waveform ~", waveform_bounds_, juce::Justification::centred);
}

void SampleEditorComponent::setFocusedPad(int pad_idx) {
    focused_pad_ = std::clamp(pad_idx, 0, 15);
    pad_name_label_.setText("PAD " + juce::String(focused_pad_ + 1), juce::dontSendNotification);
    updateFromPad();
}

bool SampleEditorComponent::setPlayMode(PlayMode mode) {
    for (int i = 0; i < 3; ++i)
        mode_buttons_[i].setToggleState(i == static_cast<int>(mode), juce::dontSendNotification);
    return proc_.push_ui_command({UiCommandType::set_pad_play_mode, focused_pad_, 0, static_cast<int32_t>(mode), 0.0f});
}

void SampleEditorComponent::updateFromPad() {
    const auto& p = proc_.current_pad(focused_pad_);
    knobs_[0]->setValue(p.pitch_semitones, juce::dontSendNotification);
    knobs_[1]->setValue(static_cast<double>(p.adsr.decay_ms), juce::dontSendNotification);
    knobs_[2]->setValue(static_cast<double>(p.pan) * 100.0, juce::dontSendNotification);
    for (int i = 0; i < 3; ++i)
        mode_buttons_[i].setToggleState(i == static_cast<int>(p.play_mode), juce::dontSendNotification);
}
