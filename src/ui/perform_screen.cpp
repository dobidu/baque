#include "perform_screen.h"

#include "pad_grid_component.h"
#include "sample_editor_component.h"
#include "sequencer_component.h"

#include <algorithm>

PerformScreen::PerformScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc)
    , laf_(laf)
    , pad_grid_(std::make_unique<PadGridComponent>(proc, laf))
    , sequencer_(std::make_unique<SequencerComponent>(proc, laf))
    , sample_editor_(std::make_unique<SampleEditorComponent>(proc, laf)) {
    // SR2: sole authority for focused pad across all children
    pad_grid_->on_pad_clicked = [this](int idx) { setFocusedPad(idx); };

    addAndMakeVisible(*pad_grid_);
    addAndMakeVisible(*sequencer_);
    addAndMakeVisible(*sample_editor_);
    addAndMakeVisible(feel_field_placeholder_);
}

PerformScreen::~PerformScreen() = default;

void PerformScreen::resized() {
    auto r = getLocalBounds().reduced(4);
    const int seq_h = r.getHeight() * 40 / 100;
    sequencer_->setBounds(r.removeFromBottom(seq_h));

    constexpr int pad_w = 260;
    constexpr int editor_w = 260;
    pad_grid_->setBounds(r.removeFromLeft(pad_w));
    sample_editor_->setBounds(r.removeFromRight(editor_w));
    feel_field_placeholder_.setBounds(r);
}

void PerformScreen::setFocusedPad(int pad_idx) noexcept {
    focused_pad_ = std::clamp(pad_idx, 0, 15);
    pad_grid_->setFocusedPad(focused_pad_);
    sequencer_->setFocusedLane(focused_pad_);
    sample_editor_->setFocusedPad(focused_pad_);
}
