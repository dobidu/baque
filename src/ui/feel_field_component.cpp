#include "feel_field_component.h"

#include <algorithm>
#include <cmath>

FeelFieldComponent::FeelFieldComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc)
    , laf_(laf) {}

FeelFieldComponent::~FeelFieldComponent() {
    stopTimer();
}

void FeelFieldComponent::visibilityChanged() {
    if (isVisible())
        startTimerHz(30);
    else
        stopTimer();
}

void FeelFieldComponent::timerCallback() {
    // No playing flag needed — current_step_==-1 when stopped is the correct guard (audit SR1)
    current_step_ = proc_.ui_snapshot().current_step.load(std::memory_order_relaxed);
    repaint();
}

juce::Point<float> FeelFieldComponent::nodePos(
    int step, float timing_ms, float bpm, float ring_radius, juce::Point<float> centre) const noexcept {
    // Perfect-grid angle: step 0 = top (-90 deg), clockwise
    constexpr float k_deg_per_step = 360.0f / 16.0f;
    const float ms_per_step = (bpm > 0.0f) ? (60000.0f / (bpm * 4.0f)) : 125.0f;
    const float offset_deg =
        (ms_per_step > 0.0f) ? juce::jlimit(-k_deg_per_step, k_deg_per_step, (timing_ms / ms_per_step) * k_deg_per_step)
                             : 0.0f;
    const float angle_deg = static_cast<float>(step) * k_deg_per_step + offset_deg - 90.0f;
    const float angle_rad = angle_deg * juce::MathConstants<float>::pi / 180.0f;
    return centre + juce::Point<float>(ring_radius * std::cos(angle_rad), ring_radius * std::sin(angle_rad));
}

void FeelFieldComponent::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
    const auto b = getLocalBounds().toFloat();
    const juce::Point<float> centre = b.getCentre();
    const float max_r = std::min(b.getWidth(), b.getHeight()) * 0.46f;

    // Snapshot once per paint (M1)
    const StepPattern pat = proc_.current_pattern();
    const FeelPattern feel = proc_.current_feel_pattern();
    const float bpm = proc_.ui_snapshot().bpm.load(std::memory_order_relaxed);

    // Background disc
    g.setColour(laf_.surface().withAlpha(0.6f));
    g.fillEllipse(centre.x - max_r, centre.y - max_r, max_r * 2.0f, max_r * 2.0f);

    // Grid ticks (16 perfect positions)
    g.setColour(laf_.text().withAlpha(0.2f));
    for (int s = 0; s < 16; ++s) {
        const float angle = (s * 360.0f / 16.0f - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
        const float x0 = centre.x + (max_r - 6.0f) * std::cos(angle);
        const float y0 = centre.y + (max_r - 6.0f) * std::sin(angle);
        const float x1 = centre.x + max_r * std::cos(angle);
        const float y1 = centre.y + max_r * std::sin(angle);
        g.drawLine(x0, y0, x1, y1, (s % 4 == 0) ? 1.5f : 0.7f);
    }

    // Concentric ring separators
    g.setColour(laf_.text().withAlpha(0.08f));
    for (int ring = 1; ring < 4; ++ring) {
        const float r = max_r * (0.35f + ring * 0.165f);
        g.drawEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 0.5f);
    }

    // Step nodes — 4 rings: outer (lanes 0-3) to inner (lanes 12-15)
    static constexpr float k_ring_radii[4] = {0.88f, 0.72f, 0.55f, 0.40f};
    static constexpr float k_ring_alphas[4] = {0.9f, 0.72f, 0.56f, 0.42f};
    for (int ring = 0; ring < 4; ++ring) {
        const float ring_r = max_r * k_ring_radii[ring];
        for (int lane_in_ring = 0; lane_in_ring < 4; ++lane_in_ring) {
            const int lane = ring * 4 + lane_in_ring;
            for (int step = 0; step < 16; ++step) {
                if (!pat.is_active(lane, step))
                    continue;
                const float vel = static_cast<float>(pat.get_velocity(lane, step)) / 127.0f;
                const float t_ms = feel.enabled ? feel.steps[step].timing_ms : 0.0f;
                const auto pos = nodePos(step, t_ms, bpm, ring_r, centre);
                const float radius = 2.5f + vel * 5.0f;
                g.setColour(laf_.ember().withAlpha(k_ring_alphas[ring] * (0.5f + vel * 0.5f)));
                g.fillEllipse(pos.x - radius, pos.y - radius, radius * 2.0f, radius * 2.0f);
            }
        }
    }

    // Playhead arm (current_step_==-1 when stopped per UiStateSnapshot contract)
    if (current_step_ >= 0) {
        const float angle =
            (static_cast<float>(current_step_) * 360.0f / 16.0f - 90.0f) * juce::MathConstants<float>::pi / 180.0f;
        const float px = centre.x + max_r * std::cos(angle);
        const float py = centre.y + max_r * std::sin(angle);
        g.setColour(laf_.ember().withAlpha(0.5f));
        g.drawLine(centre.x, centre.y, px, py, 1.5f);
    }

    // Centre readout
    const juce::Font small_font(juce::FontOptions(laf_.monoTypeface()).withHeight(10.0f));
    g.setFont(small_font);
    g.setColour(laf_.text().withAlpha(0.6f));
    const juce::String feel_label = feel.enabled ? "GROOVE" : "STRAIGHT";
    g.drawText(feel_label,
               juce::Rectangle<float>(centre.x - 30.0f, centre.y - 8.0f, 60.0f, 16.0f),
               juce::Justification::centred);
    if (feel.enabled && feel.humanize_timing_ms > 0.0f) {
        g.setColour(laf_.text().withAlpha(0.4f));
        g.drawText("HMZ " + juce::String(static_cast<int>(feel.humanize_timing_ms)) + "ms",
                   juce::Rectangle<float>(centre.x - 30.0f, centre.y + 4.0f, 60.0f, 12.0f),
                   juce::Justification::centred);
    }
}
