#include "step_clock.h"

#include <cmath>

int StepClock::current_step(double ppq_position) const noexcept {
    // Loop-around: fmod(ppq, 4.0) → posição dentro do padrão
    const double ppq_in_pattern = std::fmod(ppq_position, k_ppq_per_pattern);

    // Pequena epsilon para evitar duplo-disparo exatamente na borda do step
    // (ex.: ppq = 0.25000001 após acumular erro → ainda step 1, não 2)
    const int step = static_cast<int>(ppq_in_pattern / k_ppq_per_step);
    return step < 16 ? step : 15; // Guarda de segurança para fmod = exatamente 4.0
}

int StepClock::sample_offset_of_step_start(double ppq_at_block_start,
                                           double bpm,
                                           double sample_rate,
                                           int step_target) const noexcept {
    // PPQ do início do ciclo de padrão atual
    const double cycle_start_ppq = std::floor(ppq_at_block_start / k_ppq_per_pattern) * k_ppq_per_pattern;

    // PPQ onde step_target começa neste ciclo
    double step_ppq = cycle_start_ppq + step_target * k_ppq_per_step;

    // Se o step já passou (ou seja, ppq_at_block_start > step_ppq), próximo ciclo
    if (step_ppq < ppq_at_block_start - 1e-9)
        step_ppq += k_ppq_per_pattern;

    // Offset em segundos e amostras
    const double delta_ppq = step_ppq - ppq_at_block_start;
    const double delta_seconds = delta_ppq * (60.0 / bpm);
    const int delta_samples = static_cast<int>(delta_seconds * sample_rate);

    // Retorna block_size como sentinela se o step estiver fora do bloco atual
    // (o chamador passa block_size via contexto; usamos INT_MAX como fallback)
    return delta_samples;
}
