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
};
