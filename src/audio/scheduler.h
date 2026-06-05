#pragma once

#include "pad_bank.h"
#include "voice_pool.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Scheduler de eventos MIDI com precisão de sample.
// Itera o MidiBuffer usando JUCE 8 range-for e despacha vozes no offset correto.
// Fase 4 (04-01): roteia nota → pad (PadBank) → voz com parâmetros do pad.
// Sem alocação dinâmica no caminho quente.
//
// Mapeamento de velocity FIXADO (auditoria 04-01): ganho = (velocity / 127) × pad.gain,
// linear, sem curva, sem piso. getFloatVelocity() do JUCE já é velocity/127.
// Curvas de velocity são tema da Fase 6+ e devem mudar SOMENTE aqui.
class Scheduler {
  public:
    Scheduler() = default;

    // Processa todos os eventos MIDI do bloco atual.
    // Vozes são disparadas no offset exato de cada note-on dentro do bloco,
    // com os parâmetros do pad mapeado (nota fora do banco ou pad vazio = ignora).
    void process(const juce::MidiBuffer& midi, VoicePool& pool, const PadBank& bank, int block_size) noexcept;
};
