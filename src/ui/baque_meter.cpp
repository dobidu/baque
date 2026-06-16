#include "baque_meter.h"

#include "baque_look_and_feel.h"

BaqueMeter::BaqueMeter(const UiStateSnapshot& snapshot)
    : snapshot_(snapshot) {}

void BaqueMeter::visibilityChanged() {
    if (isVisible())
        startTimerHz(30);
    else
        stopTimer();
}

void BaqueMeter::timerCallback() {
    const float peak_l = snapshot_.master_peak_l.load(std::memory_order_relaxed);
    const float peak_r = snapshot_.master_peak_r.load(std::memory_order_relaxed);

    display_l_ = std::max(peak_l, display_l_ * k_decay_per_tick);
    display_r_ = std::max(peak_r, display_r_ * k_decay_per_tick);
    repaint();
}

void BaqueMeter::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds();
    const int w = bounds.getWidth();
    const int h = bounds.getHeight();
    const int col_w = (w - 2) / 2; // two columns
    const int seg_h = (h - (k_segments + 1)) / k_segments;
    if (seg_h < 1)
        return;

    const float levels[2] = {display_l_, display_r_};
    const int col_x[2] = {0, col_w + 2};

    for (int ch = 0; ch < 2; ++ch) {
        const int lit = static_cast<int>(levels[ch] * static_cast<float>(k_segments));
        for (int seg = 0; seg < k_segments; ++seg) {
            const int y = h - (seg + 1) * (seg_h + 1);

            juce::Colour col;
            if (seg >= k_segments - 2)
                col = BaqueLookAndFeel::ember();
            else if (seg >= k_segments - 4)
                col = BaqueLookAndFeel::ocre();
            else
                col = juce::Colour(0xff44aa44u);

            if (seg < lit)
                g.setColour(col);
            else
                g.setColour(col.withAlpha(0.2f));

            g.fillRect(col_x[ch], y, col_w, seg_h);
        }
    }
}
