#include "perf_fx_screen.h"

#include "../plugin_processor.h"

#include <algorithm>

static constexpr const char* k_stub_labels[7] = {
    "STUTTER 1/8", "STUTTER 1/16", "STUTTER 1/32", "RISER", "CRUSH", "FILL", "---"};

PerfFxScreen::PerfFxScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc), laf_(laf) {
    scatter_type_label_.setText("TYPE", juce::dontSendNotification);
    scatter_type_label_.setJustificationType(juce::Justification::centred);
    scatter_depth_label_.setText("DEPTH", juce::dontSendNotification);
    scatter_depth_label_.setJustificationType(juce::Justification::centred);

    scatter_type_attach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.getAPVTS(), "scatter_type", scatter_type_knob_);
    scatter_depth_attach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.getAPVTS(), "scatter_depth", scatter_depth_knob_);

    addAndMakeVisible(scatter_type_label_);
    addAndMakeVisible(scatter_type_knob_);
    addAndMakeVisible(scatter_depth_label_);
    addAndMakeVisible(scatter_depth_knob_);

    // Momentary pads: register 'this' as MouseListener so mouseDown/mouseUp on buttons
    // routes to PerfFxScreen::mouseDown/mouseUp with e.eventComponent for dispatch.
    tape_stop_btn_.addMouseListener(this, false);
    gate_btn_.addMouseListener(this, false);
    addAndMakeVisible(tape_stop_btn_);
    addAndMakeVisible(gate_btn_);

    for (int i = 0; i < 7; ++i) {
        perf_labels_[static_cast<std::size_t>(i)].setText(k_stub_labels[i],
                                                           juce::dontSendNotification);
        perf_labels_[static_cast<std::size_t>(i)].setJustificationType(
            juce::Justification::centred);
        perf_labels_[static_cast<std::size_t>(i)].setColour(juce::Label::textColourId,
                                                              laf_.text().withAlpha(0.35f));
        addAndMakeVisible(perf_labels_[static_cast<std::size_t>(i)]);
    }

    // XY pad: x→filter_cutoff (normalized [0,1]), y→filter_res (normalized [0,1])
    xy_pad_.on_drag = [this](float x, float y) {
        proc_.getAPVTS().getParameter("filter_cutoff")->setValueNotifyingHost(x);
        proc_.getAPVTS().getParameter("filter_res")->setValueNotifyingHost(y);
    };
    xy_pad_.on_release = [this] {
        proc_.getAPVTS().getParameter("filter_cutoff")->setValueNotifyingHost(
            proc_.getAPVTS().getParameter("filter_cutoff")->getDefaultValue());
        proc_.getAPVTS().getParameter("filter_res")->setValueNotifyingHost(
            proc_.getAPVTS().getParameter("filter_res")->getDefaultValue());
    };
    addAndMakeVisible(xy_pad_);

    for (int i = 0; i < 4; ++i) {
        mute_group_btns_[static_cast<std::size_t>(i)].setButtonText(
            juce::String(k_group_names[i]) + " M");
        solo_group_btns_[static_cast<std::size_t>(i)].setButtonText(
            juce::String(k_group_names[i]) + " S");

        mute_group_btns_[static_cast<std::size_t>(i)].onClick = [this, i] {
            const std::size_t idx = static_cast<std::size_t>(i);
            group_mute_[idx] = !group_mute_[idx];
            const int c = group_mute_[idx] ? 1 : 0;
            for (int lane = i * 4; lane < (i + 1) * 4; ++lane)
                proc_.push_ui_command({UiCommandType::set_mute, lane, 0, c});
        };

        solo_group_btns_[static_cast<std::size_t>(i)].onClick = [this, i] {
            const std::size_t idx = static_cast<std::size_t>(i);
            group_solo_[idx] = !group_solo_[idx];
            const int c = group_solo_[idx] ? 1 : 0;
            for (int lane = i * 4; lane < (i + 1) * 4; ++lane)
                proc_.push_ui_command({UiCommandType::set_solo, lane, 0, c});
        };

        addAndMakeVisible(mute_group_btns_[static_cast<std::size_t>(i)]);
        addAndMakeVisible(solo_group_btns_[static_cast<std::size_t>(i)]);
    }
}

void PerfFxScreen::mouseDown(const juce::MouseEvent& e) {
    if (e.eventComponent == &tape_stop_btn_)
        proc_.getAPVTS().getParameter("tape_stop")->setValueNotifyingHost(1.0f);
    else if (e.eventComponent == &gate_btn_)
        proc_.getAPVTS().getParameter("gate_depth")->setValueNotifyingHost(1.0f);
}

void PerfFxScreen::mouseUp(const juce::MouseEvent& e) {
    if (e.eventComponent == &tape_stop_btn_)
        proc_.getAPVTS().getParameter("tape_stop")->setValueNotifyingHost(0.0f);
    else if (e.eventComponent == &gate_btn_)
        proc_.getAPVTS().getParameter("gate_depth")->setValueNotifyingHost(0.0f);
}

void PerfFxScreen::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
}

void PerfFxScreen::resized() {
    const int col_w = getWidth() / 3;
    const int h = getHeight();

    // Left column: 2 knobs stacked (top half / bottom half)
    const int half_h = h / 2;
    const int label_h = half_h / 4;
    scatter_type_label_.setBounds(0, 0, col_w, label_h);
    scatter_type_knob_.setBounds(0, label_h, col_w, half_h - label_h);
    scatter_depth_label_.setBounds(0, half_h, col_w, label_h);
    scatter_depth_knob_.setBounds(0, half_h + label_h, col_w, half_h - label_h);

    // Center column: 3×3 grid of 9 pad slots
    const int pad_w = col_w / 3;
    const int pad_h = h / 3;
    auto slot_bounds = [&](int slot) -> juce::Rectangle<int> {
        return {col_w + (slot % 3) * pad_w, (slot / 3) * pad_h, pad_w, pad_h};
    };
    tape_stop_btn_.setBounds(slot_bounds(0));
    gate_btn_.setBounds(slot_bounds(1));
    for (int i = 0; i < 7; ++i)
        perf_labels_[static_cast<std::size_t>(i)].setBounds(slot_bounds(i + 2));

    // Right column: top 60% = XY pad; bottom 40% = 4 group rows
    const int right_x = col_w * 2;
    const int xy_h = static_cast<int>(static_cast<float>(h) * 0.6f);
    xy_pad_.setBounds(right_x, 0, col_w, xy_h);

    const int group_h = (h - xy_h) / 4;
    for (int i = 0; i < 4; ++i) {
        const int y = xy_h + i * group_h;
        mute_group_btns_[static_cast<std::size_t>(i)].setBounds(right_x, y, col_w / 2, group_h);
        solo_group_btns_[static_cast<std::size_t>(i)].setBounds(
            right_x + col_w / 2, y, col_w / 2, group_h);
    }
}

void PerfFxScreen::visibilityChanged() {
    if (!isVisible() && xy_pad_.on_release)
        xy_pad_.on_release();
}

// ── XyPad ──────────────────────────────────────────────────────────────────

void PerfFxScreen::XyPad::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillAll();
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);

    const float cx = cursor_x_ * static_cast<float>(getWidth());
    const float cy = (1.0f - cursor_y_) * static_cast<float>(getHeight());
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawHorizontalLine(static_cast<int>(cy), 0, getWidth());
    g.drawVerticalLine(static_cast<int>(cx), 0, getHeight());
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
}

void PerfFxScreen::XyPad::mouseDown(const juce::MouseEvent& e) {
    cursor_x_ = juce::jlimit(0.0f, 1.0f,
                              static_cast<float>(e.x) / static_cast<float>(getWidth()));
    cursor_y_ = juce::jlimit(0.0f, 1.0f,
                              1.0f - static_cast<float>(e.y) / static_cast<float>(getHeight()));
    repaint();
    if (on_drag) on_drag(cursor_x_, cursor_y_);
}

void PerfFxScreen::XyPad::mouseDrag(const juce::MouseEvent& e) {
    cursor_x_ = juce::jlimit(0.0f, 1.0f,
                              static_cast<float>(e.x) / static_cast<float>(getWidth()));
    cursor_y_ = juce::jlimit(0.0f, 1.0f,
                              1.0f - static_cast<float>(e.y) / static_cast<float>(getHeight()));
    repaint();
    if (on_drag) on_drag(cursor_x_, cursor_y_);
}

void PerfFxScreen::XyPad::mouseUp(const juce::MouseEvent&) {
    cursor_x_ = 0.5f;
    cursor_y_ = 0.5f;
    repaint();
    if (on_release) on_release();
}
