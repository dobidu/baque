#pragma once

#include <array>
#include <cstdint>

// Estado de performance do sequenciador (Fase 8-04) — POD, sem dependências JUCE.
// Lido na audio thread em Sequencer::generate(). fill_active gateia trig conditions fill/not_fill;
// mute/solo gateiam por lane.
//
// CONTRATO SINGLE-WRITER (auditoria SR1): em v1 não há writer concorrente (sem UI/APVTS para
// fill/mute/solo). A Fase 10 (UI) que escrever estes campos a partir da message thread DEVE migrar
// para std::atomic ou command queue — mesma decisão de "Pad params single-writer" da Fase 4.
// Sem isso, vira data race silencioso.
struct PerfState {
    bool fill_active = false;
    std::array<bool, 16> mute{}; // por lane (StepPattern::k_num_lanes)
    std::array<bool, 16> solo{};
};
