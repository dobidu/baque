#pragma once
#include <cstdint>

// Parâmetros de FX disponíveis para p-lock (automação por step).
// Ordem importa — índice no array PLockStep::values.
enum class PLockParam : uint8_t {
    filter_cutoff = 0,
    filter_res = 1,
    reverb_mix = 2,
    delay_mix = 3,
    delay_time = 4,
    sidechain_threshold = 5,
    bit_depth = 6,
    sr_factor = 7,
    granular_spray = 8,
    granular_pitch_spread = 9,
    granular_freeze = 10,
    scatter_type = 11,
    scatter_depth = 12,
    tape_stop = 13,
    gate_depth = 14,
    count = 15
};

static constexpr int k_plock_param_count = static_cast<int>(PLockParam::count);

// Override por step: values[i] é ativo somente se active[i]=true.
struct PLockStep {
    float values[k_plock_param_count] = {};
    bool active[k_plock_param_count] = {};
};

// Padrão de p-locks para 16 steps. enabled=false → engine ignora completamente.
struct PLockPattern {
    static constexpr int k_steps = 16;
    PLockStep steps[k_steps];
    bool enabled = false;
};

// Evento único emitido pelo Sequencer quando um step com p-lock dispara.
struct PLockEvent {
    PLockParam param;
    float value;
};

// Batch de eventos p-lock gerado por Sequencer::generate() — stack-allocated, sem alocação.
// Eventos além de k_max são descartados (overflow guard — AC-8).
struct PLockBatch {
    static constexpr int k_max = 32;
    PLockEvent events[k_max];
    int count = 0;

    void push(PLockParam param, float value) noexcept {
        if (count < k_max)
            events[count++] = {param, value};
    }
};
