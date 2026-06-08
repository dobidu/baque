#include "step_pattern.h"

StepPattern::StepPattern() noexcept {
    // Inicializa com padrão de bumbo em cada tempo — produz saída audível imediatamente
    for (int step : {0, 4, 8, 12})
        steps_[0][static_cast<std::size_t>(step)].active = true;
}

bool StepPattern::is_active(int lane, int step) const noexcept {
    return steps_[static_cast<std::size_t>(lane)][static_cast<std::size_t>(step)].active;
}

uint8_t StepPattern::get_note(int lane, int step) const noexcept {
    return steps_[static_cast<std::size_t>(lane)][static_cast<std::size_t>(step)].note;
}

StepPattern::TrigCondition StepPattern::get_trig(int lane, int step) const noexcept {
    return steps_[static_cast<std::size_t>(lane)][static_cast<std::size_t>(step)].trig;
}

void StepPattern::set_active(int lane, int step, bool val) noexcept {
    steps_[static_cast<std::size_t>(lane)][static_cast<std::size_t>(step)].active = val;
}

void StepPattern::set_note(int lane, int step, uint8_t note) noexcept {
    steps_[static_cast<std::size_t>(lane)][static_cast<std::size_t>(step)].note = note;
}

void StepPattern::set_trig(int lane, int step, TrigCondition cond) noexcept {
    steps_[static_cast<std::size_t>(lane)][static_cast<std::size_t>(step)].trig = cond;
}
