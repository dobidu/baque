#include "pad_grid_component.h"

#include <algorithm>

PadGridComponent::PadGridComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc)
    , laf_(laf) {}

PadGridComponent::~PadGridComponent() {
    stopTimer();
}

void PadGridComponent::visibilityChanged() {
    if (isVisible())
        startTimerHz(30);
    else
        stopTimer();
}

void PadGridComponent::timerCallback() {
    for (int i = 0; i < 16; ++i) {
        const float v =
            proc_.ui_snapshot().lane_last_velocity[static_cast<std::size_t>(i)].load(std::memory_order_relaxed) /
            127.0f;
        display_velocity_[static_cast<std::size_t>(i)] =
            std::max(v, display_velocity_[static_cast<std::size_t>(i)] * 0.88f);
    }
    repaint();
}

juce::Rectangle<float> PadGridComponent::padBounds(int idx) const noexcept {
    const float gap = 4.0f;
    const auto b = getLocalBounds().toFloat();
    const float cell_w = (b.getWidth() - gap * (k_cols + 1)) / static_cast<float>(k_cols);
    const float cell_h = (b.getHeight() - gap * (k_rows + 1)) / static_cast<float>(k_rows);
    const int col = idx % k_cols;
    const int row = idx / k_cols;
    const float x = b.getX() + gap + static_cast<float>(col) * (cell_w + gap);
    const float y = b.getY() + gap + static_cast<float>(row) * (cell_h + gap);
    return {x, y, cell_w, cell_h};
}

void PadGridComponent::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
    for (int i = 0; i < 16; ++i) {
        const auto r = padBounds(i);
        const float vel = display_velocity_[static_cast<std::size_t>(i)];

        g.setColour(laf_.surface().brighter(0.15f));
        g.fillRoundedRectangle(r, 6.0f);

        if (vel > 0.01f) {
            g.setColour(laf_.ember().withAlpha(vel * 0.6f));
            g.fillRoundedRectangle(r, 6.0f);
        }

        if (i == focused_pad_) {
            g.setColour(laf_.ember());
            g.drawRoundedRectangle(r.reduced(1.0f), 6.0f, 1.5f);
        }

        g.setColour(laf_.text().withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(laf_.monoTypeface()).withHeight(11.0f)));
        g.drawText(juce::String::formatted("P%02d", i + 1), r.toNearestInt(), juce::Justification::centred);
    }
}

void PadGridComponent::resized() {}

void PadGridComponent::mouseDown(const juce::MouseEvent& e) {
    const auto pos = e.getPosition().toFloat();
    for (int i = 0; i < 16; ++i) {
        if (padBounds(i).contains(pos)) {
            if (on_pad_clicked)
                on_pad_clicked(i);
            return;
        }
    }
}

void PadGridComponent::setFocusedPad(int pad_idx) noexcept {
    focused_pad_ = std::clamp(pad_idx, 0, 15);
    repaint();
}
