#pragma once

// Converte ppq_position do host → número de step (0–15) e offsets de sample.
// Sem estado: todos os inputs passados explicitamente — RT-safe.
// Resolução: 1/16 de nota (0.25 ppq). Padrão completo = 4.0 ppq (16 steps).
class StepClock {
  public:
    static constexpr double k_ppq_per_step = 0.25;   // 1/16 de nota
    static constexpr double k_ppq_per_pattern = 4.0; // 16 steps × 0.25

    StepClock() = default;

    // Retorna o step atual (0–15) a partir do ppq_position do host.
    // Usa fmod para loop-around correto.
    [[nodiscard]] int current_step(double ppq_position) const noexcept;

    // Retorna o offset de sample (dentro do bloco) onde o step_target começa.
    // Retorna block_size se o step não ocorre dentro do bloco atual.
    [[nodiscard]] int sample_offset_of_step_start(double ppq_at_block_start,
                                                  double bpm,
                                                  double sample_rate,
                                                  int step_target) const noexcept;
};
