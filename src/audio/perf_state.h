#pragma once

#include <array>
#include <cstdint>

// Estado de performance do sequenciador (Fase 8-04) — POD, sem dependências JUCE.
// Lido na audio thread em Sequencer::generate(). fill_active gateia trig conditions fill/not_fill;
// mute/solo gateiam por lane.
//
// CONTRATO (Fase 10-01): mutação ao vivo via BaqueProcessor::push_ui_command()
// (set_fill / set_mute / set_solo). Escrita direta válida apenas em setup/testes
// antes de o processamento começar. O command queue é a única via thread-safe.
struct PerfState {
    bool fill_active = false;
    std::array<bool, 16> mute{}; // por lane (StepPattern::k_num_lanes)
    std::array<bool, 16> solo{};
};
