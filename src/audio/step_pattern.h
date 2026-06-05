#pragma once

#include <array>
#include <cstdint>

// Grid de steps do sequenciador: 16 lanes × 16 steps, sem alocação dinâmica.
// Cada step tem estado ativo/inativo e número de nota MIDI.
class StepPattern {
  public:
    static constexpr int k_num_steps = 16;
    static constexpr int k_num_lanes = 16;

    // Dados de um step: note MIDI + flag ativo.
    struct Step {
        bool active = false;
        uint8_t note = 36; // C2 = bumbo por convenção
    };

    // Padrão padrão: bumbo em cada tempo (steps 0,4,8,12 na lane 0).
    StepPattern() noexcept;

    [[nodiscard]] bool is_active(int lane, int step) const noexcept;
    [[nodiscard]] uint8_t get_note(int lane, int step) const noexcept;
    void set_active(int lane, int step, bool val) noexcept;
    void set_note(int lane, int step, uint8_t note) noexcept;

  private:
    std::array<std::array<Step, k_num_steps>, k_num_lanes> steps_;
};
