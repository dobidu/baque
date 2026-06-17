#include "midi_screen.h"

#include "../plugin_processor.h"

#include <algorithm>
#include <cmath>

MidiScreen::MidiScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc), laf_(laf) {
    // Clock master — SR3: init from engine state, not hardcoded false
    clock_master_state_ = proc_.clock_master_;
    clock_master_btn_.setClickingTogglesState(false);
    clock_master_btn_.setToggleState(clock_master_state_, juce::dontSendNotification);
    clock_master_btn_.onClick = [this] {
        clock_master_state_ = !clock_master_state_;
        clock_master_btn_.setToggleState(clock_master_state_, juce::dontSendNotification);
        proc_.push_ui_command({UiCommandType::set_clock_master, 0, 0, clock_master_state_ ? 1 : 0});
    };
    addAndMakeVisible(clock_master_btn_);

    // Template buttons: apply_template a=0 (TR-8) or a=1 (TR-8S)
    tr8_btn_.onClick = [this] {
        proc_.push_ui_command({UiCommandType::apply_template, 0, 0, 0});
    };
    tr8s_btn_.onClick = [this] {
        proc_.push_ui_command({UiCommandType::apply_template, 1, 0, 0});
    };
    addAndMakeVisible(tr8_btn_);
    addAndMakeVisible(tr8s_btn_);

    // Per-lane routing table
    for (int i = 0; i < 16; ++i) {
        const std::size_t idx = static_cast<std::size_t>(i);

        lane_labels_[idx].setText("LANE " + juce::String(i + 1), juce::dontSendNotification);
        lane_labels_[idx].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(lane_labels_[idx]);

        note_labels_[idx].setText("--", juce::dontSendNotification);
        note_labels_[idx].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(note_labels_[idx]);

        // channel_state_ advisory-init from engine routing
        channel_state_[idx] = proc_.lane_routing_.channel_of(i);
        channel_btns_[idx].setButtonText("CH " + juce::String(channel_state_[idx]));
        channel_btns_[idx].onClick = [this, i] {
            const std::size_t k = static_cast<std::size_t>(i);
            channel_state_[k] = (channel_state_[k] % 16) + 1;
            channel_btns_[k].setButtonText("CH " + juce::String(channel_state_[k]));
            proc_.push_ui_command({UiCommandType::set_lane_channel, i, 0, channel_state_[k]});
        };
        addAndMakeVisible(channel_btns_[idx]);

        // MODE buttons — mutually exclusive toggles; onClick sets state immediately
        // (no timer re-sync to avoid flicker while UiCommand is in-flight, SR1)
        const auto cur_mode = proc_.lane_routing_.mode[idx];
        mode_int_[idx].setButtonText("INT");
        mode_int_[idx].setToggleState(cur_mode == LaneMode::internal, juce::dontSendNotification);
        mode_int_[idx].onClick = [this, i] {
            const std::size_t k = static_cast<std::size_t>(i);
            proc_.push_ui_command({UiCommandType::set_lane_mode, i, 0, 0});
            mode_int_[k].setToggleState(true,  juce::dontSendNotification);
            mode_ext_[k].setToggleState(false, juce::dontSendNotification);
            mode_both_[k].setToggleState(false, juce::dontSendNotification);
        };

        mode_ext_[idx].setButtonText("EXT");
        mode_ext_[idx].setToggleState(cur_mode == LaneMode::external, juce::dontSendNotification);
        mode_ext_[idx].onClick = [this, i] {
            const std::size_t k = static_cast<std::size_t>(i);
            proc_.push_ui_command({UiCommandType::set_lane_mode, i, 0, 1});
            mode_int_[k].setToggleState(false, juce::dontSendNotification);
            mode_ext_[k].setToggleState(true,  juce::dontSendNotification);
            mode_both_[k].setToggleState(false, juce::dontSendNotification);
        };

        mode_both_[idx].setButtonText("BOTH");
        mode_both_[idx].setToggleState(cur_mode == LaneMode::both, juce::dontSendNotification);
        mode_both_[idx].onClick = [this, i] {
            const std::size_t k = static_cast<std::size_t>(i);
            proc_.push_ui_command({UiCommandType::set_lane_mode, i, 0, 2});
            mode_int_[k].setToggleState(false, juce::dontSendNotification);
            mode_ext_[k].setToggleState(false, juce::dontSendNotification);
            mode_both_[k].setToggleState(true,  juce::dontSendNotification);
        };

        addAndMakeVisible(mode_int_[idx]);
        addAndMakeVisible(mode_ext_[idx]);
        addAndMakeVisible(mode_both_[idx]);

        // LEARN — visual-only in v1; no engine lane-note MIDI learn
        learn_btns_[idx].setButtonText("LEARN");
        learn_btns_[idx].onClick = [this, i] {
            const std::size_t k = static_cast<std::size_t>(i);
            learn_btns_[k].setToggleState(!learn_btns_[k].getToggleState(),
                                          juce::dontSendNotification);
        };
        addAndMakeVisible(learn_btns_[idx]);
    }
}

MidiScreen::~MidiScreen() {
    stopTimer();
}

void MidiScreen::timerCallback() {
    // SR1: do NOT update mode toggles here — onClick sets them immediately for
    // responsive UI; timer re-reading stale advisory would cause 100ms flicker
    // while a set_lane_mode UiCommand is in-flight.
    // Update note labels only (advisory read; tolerates torn data).
    const auto pat = proc_.current_pattern();
    for (int i = 0; i < 16; ++i) {
        const std::size_t idx = static_cast<std::size_t>(i);
        note_labels_[idx].setText(
            juce::MidiMessage::getMidiNoteName(static_cast<int>(pat.get_note(i, 0)),
                                               true, true, 4),
            juce::dontSendNotification);
    }
    repaint();
}

void MidiScreen::visibilityChanged() {
    isVisible() ? startTimerHz(10) : stopTimer();
}

void MidiScreen::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
}

void MidiScreen::resized() {
    auto area = getLocalBounds();

    // Left panel: 280px wide
    constexpr int k_left_w = 280;
    auto left = area.removeFromLeft(k_left_w);
    clock_master_btn_.setBounds(left.removeFromTop(40).reduced(8, 4));
    const int tmpl_h = 36;
    auto tmpl_row = left.removeFromTop(tmpl_h);
    tr8_btn_.setBounds(tmpl_row.removeFromLeft(tmpl_row.getWidth() / 2).reduced(4, 4));
    tr8s_btn_.setBounds(tmpl_row.reduced(4, 4));

    // Right panel: header row + 16 data rows
    const int header_h = 24;
    const int row_h = (area.getHeight() - header_h) / 16;

    // Column x-offsets (proportional within right panel)
    const int right_x = area.getX();
    const int w = area.getWidth();
    // Columns: LANE(60) NOTE(55) INT(45) EXT(45) BOTH(50) CH(55) LEARN(60) — scale to w
    const int col_w[7] = {
        static_cast<int>(w * 0.15f), // LANE
        static_cast<int>(w * 0.13f), // NOTE
        static_cast<int>(w * 0.11f), // INT
        static_cast<int>(w * 0.11f), // EXT
        static_cast<int>(w * 0.13f), // BOTH
        static_cast<int>(w * 0.13f), // CH
        static_cast<int>(w * 0.13f), // LEARN
    };
    int col_x[7];
    col_x[0] = right_x;
    for (int c = 1; c < 7; ++c)
        col_x[c] = col_x[c - 1] + col_w[c - 1];

    const int data_y0 = area.getY() + header_h;
    for (int i = 0; i < 16; ++i) {
        const std::size_t idx = static_cast<std::size_t>(i);
        const int y = data_y0 + i * row_h;
        lane_labels_[idx].setBounds(col_x[0], y, col_w[0], row_h);
        note_labels_[idx].setBounds(col_x[1], y, col_w[1], row_h);
        mode_int_[idx].setBounds(col_x[2], y, col_w[2], row_h);
        mode_ext_[idx].setBounds(col_x[3], y, col_w[3], row_h);
        mode_both_[idx].setBounds(col_x[4], y, col_w[4], row_h);
        channel_btns_[idx].setBounds(col_x[5], y, col_w[5], row_h);
        learn_btns_[idx].setBounds(col_x[6], y, col_w[6], row_h);
    }
}
