#pragma once

#include "feel_engine.h"
#include "feel_pattern.h"
#include "note_tracker.h"
#include "step_clock.h"
#include "step_pattern.h"
#include "transport_state.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

// Sequenciador de steps: converte padrão + posição ppq → eventos MidiBuffer.
// Fase 3 (03-02): swing global + troca de padrão sem glitch + NoteTracker para note-off correto.
// Fase 5 (05-01): per-step timing offset via FeelEngine + deferred note queue.
class Sequencer {
  public:
    Sequencer() = default;

    // Gera eventos MIDI para o bloco atual.
    // midi_out deve estar limpo pelo chamador; não faz alocação.
    // feel/feel_engine: opcionais (nullptr = comportamento pré-Fase-5).
    void generate(const TransportState& transport,
                  juce::MidiBuffer& midi_out,
                  int block_size,
                  double sample_rate,
                  const FeelPattern* feel = nullptr,
                  FeelEngine* feel_engine = nullptr,
                  int64_t block_start_sample = 0) noexcept;

    // Define padrão imediatamente (sem esperar transição 15→0). Uso: setup inicial e testes.
    void set_pattern(const StepPattern& p) noexcept;

    // Agenda troca de padrão na próxima transição 15→0 (bar boundary).
    // Thread-safe: message thread escreve next_pattern_ ANTES do store com release.
    void set_next_pattern(const StepPattern& p) noexcept;

    // Controla o swing global via StepClock (delegação).
    void set_swing(float amount) noexcept { clock_.set_swing(amount); }

    // Acesso ao padrão ativo.
    [[nodiscard]] StepPattern& pattern() noexcept { return pattern_; }
    [[nodiscard]] const StepPattern& pattern() const noexcept { return pattern_; }

  private:
    StepPattern pattern_;
    StepPattern next_pattern_;
    std::atomic<bool> pattern_pending_{false}; // release/acquire entre message e audio thread
    StepClock clock_;
    NoteTracker note_tracker_;

    // Último step disparado — duplo-disparo guard + detecção de transição 15→0.
    int last_step_fired_ = -1;

    // ppq da posição final do último bloco processado — detecção de regressão (loop/restart). (M3)
    double last_ppq_ = -1.0;
};
