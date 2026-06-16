#pragma once

#include <array>
#include <cstdint>

// Roteamento de saída por lane (Fase 9-01) — POD, sem deps pesadas.
// Cada lane do StepPattern toca sample interno (internal), dirige hardware via MIDI out (external),
// ou ambos (both). channel[lane] = canal MIDI da saída EXT (1-16); 0 tratado como 1 no emit.
//
// CONTRATO (Fase 10-01): mutação ao vivo via BaqueProcessor::push_ui_command()
// (set_lane_mode / set_lane_channel). Escrita direta válida apenas em setup/testes.
enum class LaneMode : uint8_t { internal = 0, external = 1, both = 2 };

struct LaneRouting {
    std::array<LaneMode, 16> mode{};   // default internal → compat (sem EXT)
    std::array<uint8_t, 16> channel{}; // canal MIDI EXT por lane; 0 → canal 1 no emit

    // Canal MIDI válido [1,16]; 0 (default) vira 1.
    [[nodiscard]] int channel_of(int lane) const noexcept {
        const uint8_t c = channel[static_cast<std::size_t>(lane)];
        const int v = c == 0 ? 1 : static_cast<int>(c);
        return v < 1 ? 1 : (v > 16 ? 16 : v);
    }
};
