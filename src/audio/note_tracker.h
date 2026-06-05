#pragma once

#include <array>
#include <cstdint>

// Rastreador de notas por lane: armazena a última nota MIDI disparada em cada lane.
// Permite que o sequenciador envie note-off com o número de nota correto.
// Usado exclusivamente no audio thread — sem sincronização necessária.
class NoteTracker {
  public:
    static constexpr int k_num_lanes = 16;

    NoteTracker() = default;

    // Registra a nota disparada numa lane.
    void note_triggered(int lane, uint8_t note) noexcept;

    // Retorna a última nota disparada nessa lane (0 se nenhuma).
    [[nodiscard]] uint8_t get_active_note(int lane) const noexcept;

    // Zera o rastreamento de todas as lanes (chamado em prepareToPlay).
    void reset() noexcept;

  private:
    std::array<uint8_t, k_num_lanes> active_notes_{};
};
