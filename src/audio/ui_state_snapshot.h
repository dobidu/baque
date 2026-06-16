#pragma once

#include <array>
#include <atomic>
#include <cstdint>

// Snapshot do estado do engine publicado no fim de cada processBlock.
// Leitura pela message thread a qualquer instante — por-campo atômico, relaxed.
// Tearing entre campos é aceitável e documentado: cada campo é coerente individualmente;
// a UI decai/interpola em sua própria cadência de frame (AC-4 — Fase 10-01).
struct UiStateSnapshot {
    std::atomic<int32_t> current_step{-1};   // 0-15 quando tocando; -1 quando parado
    std::atomic<bool>    is_playing{false};
    std::atomic<float>   master_peak_l{0.0f};
    std::atomic<float>   master_peak_r{0.0f};
    // Escrito em cada note-on de midi_buffer_seq_ (lanes internas). Decay cabe à UI.
    // Lanes EXT-only não pulsam: note-ons vão para midi_buffer_ext_ (v1 limitation).
    std::array<std::atomic<uint8_t>, 16> lane_last_velocity{};
    std::atomic<float> bpm{120.0f};

    UiStateSnapshot() = default;
    UiStateSnapshot(const UiStateSnapshot&) = delete;
    UiStateSnapshot& operator=(const UiStateSnapshot&) = delete;
    UiStateSnapshot(UiStateSnapshot&&) = delete;
    UiStateSnapshot& operator=(UiStateSnapshot&&) = delete;
};
