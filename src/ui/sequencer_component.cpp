#include "sequencer_component.h"

#include "../audio/ui_command_queue.h"

#include <algorithm>

SequencerComponent::SequencerComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc)
    , laf_(laf) {
    mode_buttons_[0].setButtonText("TODAS");
    mode_buttons_[0].setClickingTogglesState(true);
    mode_buttons_[0].setToggleState(true, juce::dontSendNotification);
    mode_buttons_[0].onClick = [this] { setMode(true); };

    mode_buttons_[1].setButtonText("FOCO");
    mode_buttons_[1].setClickingTogglesState(true);
    mode_buttons_[1].onClick = [this] { setMode(false); };

    for (auto& btn : mode_buttons_)
        addAndMakeVisible(btn);
}

SequencerComponent::~SequencerComponent() {
    stopTimer();
}

void SequencerComponent::visibilityChanged() {
    if (isVisible())
        startTimerHz(30);
    else
        stopTimer();
}

void SequencerComponent::timerCallback() {
    current_step_ = proc_.ui_snapshot().current_step.load(std::memory_order_relaxed);
    repaint();
}

void SequencerComponent::resized() {
    auto r = getLocalBounds();
    auto toggle_row = r.removeFromTop(k_toggle_h);
    const int btn_w = 72;
    mode_buttons_[1].setBounds(toggle_row.removeFromRight(btn_w).reduced(2));
    mode_buttons_[0].setBounds(toggle_row.removeFromRight(btn_w).reduced(2));
}

juce::Rectangle<int> SequencerComponent::stepBounds(int lane, int step) const noexcept {
    const auto r = getLocalBounds().withTrimmedTop(k_toggle_h);
    const int step_area_w = r.getWidth() - k_label_w;
    const int row_h = (r.getHeight() > 0) ? r.getHeight() / k_lanes : 1;
    const int cell_w = (step_area_w > 0) ? step_area_w / k_steps : 1;
    const int x = r.getX() + k_label_w + step * cell_w;
    const int y = r.getY() + lane * row_h;
    return {x, y, cell_w - 1, row_h - 1};
}

juce::Rectangle<int> SequencerComponent::focusBarBounds(int step) const noexcept {
    const auto r = getLocalBounds().withTrimmedTop(k_toggle_h);
    const int bar_area_w = r.getWidth() - k_label_w;
    const int bar_w = (bar_area_w > 0) ? bar_area_w / k_steps : 1;
    const int x = r.getX() + k_label_w + step * bar_w;
    return {x, r.getY(), bar_w - 1, r.getHeight()};
}

void SequencerComponent::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());

    const StepPattern pat = proc_.current_pattern(); // one copy per repaint (M1)
    const auto r = getLocalBounds().withTrimmedTop(k_toggle_h);
    const juce::Font small_font(juce::FontOptions(laf_.monoTypeface()).withHeight(9.0f));

    if (todas_mode_) {
        for (int lane = 0; lane < k_lanes; ++lane) {
            // Lane label
            const auto label_r = juce::Rectangle<int>(
                r.getX(), r.getY() + lane * (r.getHeight() / k_lanes), k_label_w, r.getHeight() / k_lanes);
            g.setColour(laf_.text().withAlpha(0.6f));
            g.setFont(small_font);
            g.drawText(juce::String::formatted("L%02d", lane + 1), label_r, juce::Justification::centredRight);

            for (int step = 0; step < k_steps; ++step) {
                const auto cell = stepBounds(lane, step);
                const bool beat = (step % 4 == 0);

                juce::Colour bg = laf_.surface().brighter(beat ? 0.25f : 0.1f);
                if (step == current_step_)
                    bg = laf_.ember().withAlpha(0.35f);
                g.setColour(bg);
                g.fillRect(cell);

                if (pat.is_active(lane, step)) {
                    const float vel_alpha = static_cast<float>(pat.get_velocity(lane, step)) / 127.0f;
                    g.setColour(laf_.accent().withAlpha(0.4f + vel_alpha * 0.5f));
                    g.fillRect(cell);
                }
            }
        }
    } else {
        // FOCO: focused lane only, bars showing velocity
        g.setColour(laf_.text().withAlpha(0.7f));
        g.setFont(small_font);
        g.drawText(juce::String::formatted("L%02d", focused_lane_ + 1),
                   juce::Rectangle<int>(r.getX(), r.getY(), k_label_w, r.getHeight()),
                   juce::Justification::centredRight);

        for (int step = 0; step < k_steps; ++step) {
            const auto bar_r = focusBarBounds(step);
            const bool beat = (step % 4 == 0);
            g.setColour(laf_.surface().brighter(beat ? 0.25f : 0.1f));
            g.fillRect(bar_r);

            if (step == current_step_) {
                g.setColour(laf_.ember().withAlpha(0.35f));
                g.fillRect(bar_r);
            }

            if (pat.is_active(focused_lane_, step)) {
                const float vel = static_cast<float>(pat.get_velocity(focused_lane_, step)) / 127.0f;
                const int bar_h = static_cast<int>(static_cast<float>(bar_r.getHeight()) * vel);
                const auto filled = bar_r.withTop(bar_r.getBottom() - bar_h);
                g.setColour(laf_.accent());
                g.fillRect(filled);
            }
        }
    }
}

void SequencerComponent::mouseDown(const juce::MouseEvent& e) {
    if (!todas_mode_)
        return;
    const auto pos = e.getPosition();
    for (int lane = 0; lane < k_lanes; ++lane) {
        for (int step = 0; step < k_steps; ++step) {
            if (stepBounds(lane, step).contains(pos)) {
                toggleStep(lane, step);
                return;
            }
        }
    }
}

bool SequencerComponent::toggleStep(int lane, int step) {
    if (lane < 0 || lane >= k_lanes || step < 0 || step >= k_steps)
        return false;
    const StepPattern pat = proc_.current_pattern();
    // No optimistic repaint — 30fps timer reconciles within 33ms (SR1)
    return proc_.push_ui_command({UiCommandType::set_step, lane, step, pat.is_active(lane, step) ? 0 : 1, 0.0f});
}

void SequencerComponent::setMode(bool todas) noexcept {
    todas_mode_ = todas;
    mode_buttons_[0].setToggleState(todas, juce::dontSendNotification);
    mode_buttons_[1].setToggleState(!todas, juce::dontSendNotification);
    repaint();
}

void SequencerComponent::setFocusedLane(int lane) noexcept {
    focused_lane_ = std::clamp(lane, 0, k_lanes - 1);
    repaint();
}
