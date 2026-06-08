#pragma once

// Snapshot dos parâmetros de FX para um bloco de áudio.
// Gerado por PluginProcessor a partir do APVTS; p-locks sobrescrevem campos específicos.
// Passado para FxChain::process() em Fase 6-02.
struct FxParams {
    float filter_cutoff = 20000.0f;     // Hz [20, 20000]
    float filter_res = 0.1f;            // [0, 1]
    float reverb_mix = 0.0f;            // [0, 1]
    float delay_mix = 0.0f;             // [0, 1]
    float delay_time = 0.25f;           // seconds [0.001, 2.0]
    float sidechain_threshold = -12.0f; // dBFS [-60, 0]
    float bit_depth = 16.0f;            // bits [4, 24]; 16 = transparent
    float sr_factor = 1.0f;             // ZOH factor [1, 4]; 1.0 = no reduction
    float granular_spray = 0.0f;        // [0, 1]; 0 = no scatter
    float granular_pitch_spread = 0.0f; // [0, 1]; 0 = no pitch variation
    float granular_freeze = 0.0f;       // >= 0.5f = freeze on (float boolean convention); 0.0 = off
    float scatter_type = 0.0f;          // 0 = off, 1-10 = preset de glitch (discreto)
    float scatter_depth = 0.0f;         // [0, 1]; 0 = sem efeito (wet/dry)
    float tape_stop = 0.0f;             // [0, 1]; 0 = normal, 1 = parado (halt→silêncio)
    float gate_depth = 0.0f;            // [0, 1]; 0 = sem gate, 1 = chop total
};
