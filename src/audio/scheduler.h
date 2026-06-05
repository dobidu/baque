#pragma once

#include "voice_pool.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Scheduler de eventos MIDI com precisão de sample.
// Itera o MidiBuffer usando JUCE 8 range-for e despacha vozes no offset correto.
// Sem alocação dinâmica no caminho quente.
class Scheduler {
  public:
    Scheduler() = default;

    // Processa todos os eventos MIDI do bloco atual.
    // Vozes são disparadas no offset exato de cada note-on dentro do bloco.
    void process(const juce::MidiBuffer& midi,
                 VoicePool& pool,
                 const float* sample_data,
                 int num_samples,
                 int block_size) noexcept;
};
