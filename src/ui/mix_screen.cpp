#include "mix_screen.h"

#include "../plugin_processor.h"

#include <algorithm>
#include <cmath>

MixScreen::MixScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc)
    , laf_(laf) {
    for (int i = 0; i < 16; ++i) {
        lane_faders_[i].setRange(0.0, 1.0, 0.0);
        lane_faders_[i].setValue(1.0, juce::dontSendNotification);
        lane_pans_[i].setRange(-1.0, 1.0, 0.0);
        lane_pans_[i].setValue(0.0, juce::dontSendNotification);
        mute_btns_[i].setButtonText("M");
        solo_btns_[i].setButtonText("S");

        lane_faders_[i].onValueChange = [this, i] {
            proc_.push_ui_command(
                {UiCommandType::set_pad_gain, i, 0, 0, static_cast<float>(lane_faders_[i].getValue())});
        };
        lane_pans_[i].onValueChange = [this, i] {
            proc_.push_ui_command({UiCommandType::set_pad_pan, i, 0, 0, static_cast<float>(lane_pans_[i].getValue())});
        };
        mute_btns_[i].onClick = [this, i] {
            mute_state_[i] = !mute_state_[i];
            proc_.push_ui_command({UiCommandType::set_mute, i, 0, mute_state_[i] ? 1 : 0, 0.0f});
            repaint();
        };
        solo_btns_[i].onClick = [this, i] {
            solo_state_[i] = !solo_state_[i];
            proc_.push_ui_command({UiCommandType::set_solo, i, 0, solo_state_[i] ? 1 : 0, 0.0f});
            repaint();
        };

        addAndMakeVisible(lane_faders_[i]);
        addAndMakeVisible(lane_pans_[i]);
        addAndMakeVisible(mute_btns_[i]);
        addAndMakeVisible(solo_btns_[i]);
    }

    master_fader_.setRange(0.0, 1.0, 0.0);
    master_fader_.setValue(0.8, juce::dontSendNotification);
    master_attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.getAPVTS(), "master_gain", master_fader_);
    addAndMakeVisible(master_fader_);
}

MixScreen::~MixScreen() {
    stopTimer();
}

void MixScreen::visibilityChanged() {
    isVisible() ? startTimerHz(30) : stopTimer();
}

void MixScreen::timerCallback() {
    constexpr float k_decay = 0.88f;
    const auto& snap = proc_.ui_snapshot();
    for (int i = 0; i < 16; ++i) {
        const float v = snap.lane_last_velocity[i].load(std::memory_order_relaxed) / 127.0f;
        lane_meter_[i] = std::max(v, lane_meter_[i] * k_decay);
    }
    master_meter_l_ = std::max(snap.master_peak_l.load(std::memory_order_relaxed), master_meter_l_ * k_decay);
    master_meter_r_ = std::max(snap.master_peak_r.load(std::memory_order_relaxed), master_meter_r_ * k_decay);
    repaint();
}

void MixScreen::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());

    constexpr int k_master_w = 72;
    const int lane_w = (getWidth() - k_master_w) / 16;
    const int h = getHeight();

    for (int i = 0; i < 16; ++i) {
        const int x = i * lane_w;
        const int pan_h = h / 5;
        const int fader_h = (h * 50) / 100;
        const auto meter_y = pan_h;
        const int bar_h = static_cast<int>(lane_meter_[i] * fader_h);
        g.setColour(BaqueLookAndFeel::ember().withAlpha(lane_meter_[i]));
        g.fillRect(x, meter_y + fader_h - bar_h, lane_w, bar_h);

        if (mute_state_[i]) {
            g.setColour(BaqueLookAndFeel::ember().withAlpha(0.25f));
            g.fillRect(mute_btns_[i].getBounds());
        }
        if (solo_state_[i]) {
            g.setColour(laf_.accent().withAlpha(0.25f));
            g.fillRect(solo_btns_[i].getBounds());
        }
    }

    // Master L/R meter bars (8px each, left side of master strip)
    const int mx = getWidth() - k_master_w;
    const int bar_l = static_cast<int>(master_meter_l_ * h);
    const int bar_r = static_cast<int>(master_meter_r_ * h);
    g.setColour(BaqueLookAndFeel::ember());
    g.fillRect(mx, h - bar_l, 8, bar_l);
    g.fillRect(mx + 8, h - bar_r, 8, bar_r);
}

void MixScreen::resized() {
    constexpr int k_master_w = 72;
    const int lane_w = (getWidth() - k_master_w) / 16;
    const int h = getHeight();

    for (int i = 0; i < 16; ++i) {
        const int x = i * lane_w;
        const int pan_h = h / 5;
        const int fader_h = (h * 50) / 100;
        const int btn_h = (h - pan_h - fader_h) / 2;
        lane_pans_[i].setBounds(x, 0, lane_w, pan_h);
        lane_faders_[i].setBounds(x, pan_h, lane_w, fader_h);
        mute_btns_[i].setBounds(x, pan_h + fader_h, lane_w, btn_h);
        solo_btns_[i].setBounds(x, pan_h + fader_h + btn_h, lane_w, btn_h);
    }

    // Master strip: 16px for L/R meter bars, remainder for fader
    const int mx = getWidth() - k_master_w + 16;
    master_fader_.setBounds(mx, 0, k_master_w - 16, h);
}
