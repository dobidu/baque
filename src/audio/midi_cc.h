#pragma once

#include "plock_pattern.h"

#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <cstdint>

// Faixa de valor de cada PLockParam — única fonte de verdade (SR1).
// plock_param_norm() e plock_param_denorm() leem esta tabela: sem literal duplicado, sem drift.
// Ordem: mesmo índice que PLockParam enum (0..14).
struct ParamRange {
    float lo;
    float hi;
};

static constexpr std::array<ParamRange, k_plock_param_count> k_param_range = {{
    {20.0f, 20000.0f}, // 0  filter_cutoff  [20, 20000] Hz  (lei linear em v1; log adiado)
    {0.0f, 1.0f},      // 1  filter_res
    {0.0f, 1.0f},      // 2  reverb_mix
    {0.0f, 1.0f},      // 3  delay_mix
    {0.001f, 2.0f},    // 4  delay_time
    {-60.0f, 0.0f},    // 5  sidechain_threshold
    {4.0f, 24.0f},     // 6  bit_depth
    {1.0f, 4.0f},      // 7  sr_factor
    {0.0f, 1.0f},      // 8  granular_spray
    {0.0f, 1.0f},      // 9  granular_pitch_spread
    {0.0f, 1.0f},      // 10 granular_freeze
    {0.0f, 10.0f},     // 11 scatter_type
    {0.0f, 1.0f},      // 12 scatter_depth
    {0.0f, 1.0f},      // 13 tape_stop
    {0.0f, 1.0f},      // 14 gate_depth
}};

// Normaliza valor de param para [0,1] — lê k_param_range (SR1).
inline float plock_param_norm(PLockParam p, float value) noexcept {
    const auto& r = k_param_range[static_cast<size_t>(p)];
    const float n = (value - r.lo) / (r.hi - r.lo);
    return n < 0.0f ? 0.0f : (n > 1.0f ? 1.0f : n);
}

// Desnormaliza norm01 para valor de param — lê k_param_range (SR1, sem drift).
inline float plock_param_denorm(PLockParam p, float norm01) noexcept {
    const float c = norm01 < 0.0f ? 0.0f : (norm01 > 1.0f ? 1.0f : norm01);
    const auto& r = k_param_range[static_cast<size_t>(p)];
    return r.lo + c * (r.hi - r.lo);
}

// Mapeamento CC out por param — POD, single-writer (mesmo padrão de LaneRouting).
// CONTRATO SINGLE-WRITER: sem writer concorrente em v1. Fase 10 (UI) deve migrar p/ atomics.
struct CcOutRouting {
    bool enabled = false;
    uint8_t channel = 1; // 1-16; 0 → 1 no emit

    std::array<bool, k_plock_param_count> cc_enabled{};
    std::array<uint8_t, k_plock_param_count> cc_number{}; // 0-127 por param

    [[nodiscard]] int channel_of() const noexcept {
        const int v = static_cast<int>(channel);
        return v < 1 ? 1 : (v > 16 ? 16 : v);
    }
};

// Binding inbound CC → PLockParam.
// channel==0: sentinel "não ligado"; target==PLockParam::count: nenhum.
struct CcBinding {
    uint8_t cc = 0;
    uint8_t channel = 0;
    PLockParam target = PLockParam::count;
};

// Mapa de bindings de MIDI learn — inbound CC → PLockParam.
//
// CONTRATO DE THREADING (M1 — audit 09-03):
//   learn_arm: message thread arma via atomic store (release); audio thread lê via load (acquire)
//   e, na captura, armazena o sentinel (int)count para desarmar. Handshake RT-safe, sem lock.
//   bindings/count: OWNED pela audio thread durante a captura. Em v1 sem reader concorrente.
//   Fase 10 (UI) DEVE ler via snapshot lock-free / command queue antes de adicionar reader —
//   mesma decisão de pad-params e PerfState. NÃO expor getter mutável para UI.
//
// NÃO-COPIÁVEL (M1): std::atomic<int> member. Mute no lugar via clear() + set_binding().
// Nunca coloque em container copiável. Nunca atribua struct no state-load — use clear() + rebuild.
struct CcLearnMap {
    static constexpr int k_max = 16;

    std::array<CcBinding, k_max> bindings{};
    int count = 0;
    // Sentinel: (int)PLockParam::count = não-armado. Target válido [0,14] = armado.
    // Arm: message thread — atomic store release. Disarm: audio thread — atomic store release.
    std::atomic<int> learn_arm{static_cast<int>(PLockParam::count)};

    CcLearnMap() = default;
    CcLearnMap(const CcLearnMap&) = delete;
    CcLearnMap& operator=(const CcLearnMap&) = delete;
    CcLearnMap(CcLearnMap&&) = delete;
    CcLearnMap& operator=(CcLearnMap&&) = delete;

    void clear() noexcept {
        count = 0;
        learn_arm.store(static_cast<int>(PLockParam::count), std::memory_order_relaxed);
    }

    void set_binding(int i, CcBinding b) noexcept {
        jassert(i >= 0 && i < k_max);
        bindings[static_cast<size_t>(i)] = b;
    }
};
