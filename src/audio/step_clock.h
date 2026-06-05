#pragma once

#include <atomic>

// Converte ppq_position do host → número de step (0–15) e offsets de sample.
// Resolução: 1/16 de nota (0.25 ppq). Padrão completo = 4.0 ppq (16 steps).
// swing_amount_ é atômico para acesso seguro entre message thread e audio thread.
class StepClock {
  public:
    static constexpr double k_ppq_per_step = 0.25;   // 1/16 de nota
    static constexpr double k_ppq_per_pattern = 4.0; // 16 steps × 0.25

    StepClock() = default;

    // --- Swing ---
    // Quantidade de swing: 0.5 = sem swing (50%), 0.75 = máximo (75% estilo MPC).
    // Thread-safe: escrita da message thread com relaxed; leitura do audio thread com relaxed.
    void set_swing(float amount) noexcept;

    // --- Timing ---
    [[nodiscard]] int current_step(double ppq_position) const noexcept;

    // Retorna o offset de sample onde step_target começa neste bloco.
    // Aplica swing para steps de índice ímpar (1, 3, 5... = subdivisões "off-beat").
    [[nodiscard]] int sample_offset_of_step_start(double ppq_at_block_start,
                                                  double bpm,
                                                  double sample_rate,
                                                  int step_target) const noexcept;

  private:
    // Swing: 0.5 = sem swing, 0.75 = máximo MPC.
    // Atômico: message thread escreve, audio thread lê — relaxed em ambos (valor único, sem ordering).
    std::atomic<float> swing_amount_{0.5f};
};
