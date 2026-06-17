#include "fx_screen.h"

#include "../plugin_processor.h"

#include <algorithm>

static constexpr const char* k_param_ids[6] = {
    "filter_cutoff", "filter_res", "reverb_mix", "delay_time", "delay_mix", "sidechain_threshold"};

static constexpr const char* k_labels[6] = {"CUTOFF", "RESO", "REVERB", "DELAY TIME", "DELAY MIX", "SIDECHAIN"};

FxScreen::FxScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : laf_(laf) {
    for (int i = 0; i < 6; ++i) {
        labels_[i].setText(k_labels[i], juce::dontSendNotification);
        labels_[i].setJustificationType(juce::Justification::centred);
        attachments_[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            proc.getAPVTS(), k_param_ids[i], knobs_[i]);
        addAndMakeVisible(labels_[i]);
        addAndMakeVisible(knobs_[i]);
    }
}

void FxScreen::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
}

void FxScreen::resized() {
    const int col_w = getWidth() / 3;
    const int row_h = getHeight() / 2;
    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
            const int i = row * 3 + col;
            const int x = col * col_w;
            const int y = row * row_h;
            const int label_h = row_h / 4;
            labels_[i].setBounds(x, y, col_w, label_h);
            knobs_[i].setBounds(x, y + label_h, col_w, row_h - label_h);
        }
    }
}
