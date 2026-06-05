#pragma once

#include "pad_bank.h"
#include "sample_voice.h"
#include "voice_pool.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

// Scheduler de eventos MIDI com precisão de sample.
// Itera o MidiBuffer usando JUCE 8 range-for e despacha vozes no offset correto.
// Fase 4 (04-01): roteia nota → pad (PadBank) → voz com parâmetros do pad.
// Fase 4 (04-02): stateful — runtime_ guarda estado de reprodução por pad:
//   rr_index (round-robin, Tarefa 2) e last_voice (note-off por nota).
//   Este é estado de comportamento de reprodução, não estado de transporte.
// Sem alocação dinâmica no caminho quente.
//
// Mapeamento de velocity FIXADO (auditoria 04-01): ganho = (velocity / 127) × pad.gain,
// linear, sem curva, sem piso. getFloatVelocity() do JUCE já é velocity/127.
// Curvas de velocity são tema da Fase 6+ e devem mudar SOMENTE aqui.
class Scheduler {
  public:
    // Estado de reprodução por pad — mantido no audio thread.
    struct PadRuntime {
        uint8_t rr_index = 0;              // Índice round-robin para seleção de camada (Tarefa 2)
        SampleVoice* last_voice = nullptr; // Voz mais recente do pad (note-off routing)
    };

    Scheduler() = default;

    // Deve ser chamado em prepareToPlay antes do primeiro process().
    // Limpa runtime_ (rr_index e last_voice) e salva sample_rate para ADSR (Tarefa 3).
    void prepare(double sample_rate) noexcept;

    // Processa todos os eventos MIDI do bloco atual.
    // Vozes são disparadas no offset exato de cada note-on dentro do bloco,
    // com os parâmetros do pad mapeado (nota fora do banco ou pad vazio = ignora).
    void process(const juce::MidiBuffer& midi, VoicePool& pool, const PadBank& bank, int block_size) noexcept;

  private:
    std::array<PadRuntime, PadBank::k_num_pads> runtime_{};
    double sample_rate_ = 48000.0;
};
