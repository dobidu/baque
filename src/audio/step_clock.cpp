#include "step_clock.h"

#include <algorithm>
#include <cmath>

int StepClock::current_step(double ppq_position) const noexcept {
    // Loop-around: fmod(ppq, 4.0) → posição dentro do padrão
    const double ppq_in_pattern = std::fmod(ppq_position, k_ppq_per_pattern);

    // Pequena epsilon para evitar duplo-disparo exatamente na borda do step
    // (ex.: ppq = 0.25000001 após acumular erro → ainda step 1, não 2)
    const int step = static_cast<int>(ppq_in_pattern / k_ppq_per_step);
    return step < 16 ? step : 15; // Guarda de segurança para fmod = exatamente 4.0
}

void StepClock::set_swing(float amount) noexcept {
    // Clamp: [0.5, 0.75] — 0.5 = sem swing, 0.75 = máximo MPC
    const float clamped = std::clamp(amount, 0.5f, 0.75f);
    swing_amount_.store(clamped, std::memory_order_relaxed);
}

int StepClock::sample_offset_of_step_start(double ppq_at_block_start,
                                           double bpm,
                                           double sample_rate,
                                           int step_target) const noexcept {
    // PPQ do início do ciclo de padrão atual
    const double cycle_start_ppq = std::floor(ppq_at_block_start / k_ppq_per_pattern) * k_ppq_per_pattern;

    // PPQ onde step_target começa neste ciclo (grid reto)
    double step_ppq = cycle_start_ppq + step_target * k_ppq_per_step;

    // Se o step já passou neste ciclo → próximo ciclo
    if (step_ppq < ppq_at_block_start - 1e-9)
        step_ppq += k_ppq_per_pattern;

    // Offset de grid em segundos e amostras
    const double delta_ppq = step_ppq - ppq_at_block_start;
    const double delta_seconds = delta_ppq * (60.0 / bpm);
    int grid_samples = static_cast<int>(delta_seconds * sample_rate);

    // Swing: steps de índice ímpar (1, 3, 5... = subdivisões "off-beat") são atrasados
    if (step_target % 2 == 1) {
        const float swing = swing_amount_.load(std::memory_order_relaxed);
        const double step_duration_samples = (60.0 * sample_rate) / (bpm * 4.0);
        const int swing_offset = static_cast<int>((swing - 0.5f) * 2.0f * step_duration_samples);
        grid_samples += swing_offset;
    }

    return grid_samples;
}
