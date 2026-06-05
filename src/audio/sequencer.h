#pragma once

#include "step_clock.h"
#include "step_pattern.h"
#include "transport_state.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Sequenciador de steps: converte padrão + posição ppq → eventos MidiBuffer.
// Integrado ao processBlock; eventos são despachados pelo Scheduler existente.
class Sequencer {
  public:
    Sequencer() = default;

    // Gera eventos MIDI para o bloco atual.
    // midi_out deve estar limpo pelo chamador; não faz alocação.
    void
    generate(const TransportState& transport, juce::MidiBuffer& midi_out, int block_size, double sample_rate) noexcept;

    // Acesso ao padrão (para binding de UI nas fases futuras).
    [[nodiscard]] StepPattern& pattern() noexcept { return pattern_; }
    [[nodiscard]] const StepPattern& pattern() const noexcept { return pattern_; }

  private:
    StepPattern pattern_;
    StepClock clock_;

    // Último step disparado — guarda contra duplo-disparo ao cruzar fronteiras de bloco.
    int last_step_fired_ = -1;
};
