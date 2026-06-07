#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// Compressor de sidechain com trigger interno — envelope follower assimétrico (IIR) + gain computer.
// RT-safe: sem alocação em process(). Coeficientes pré-computados em prepare().
// threshold via FxParams::sidechain_threshold (dBFS). Ratio/attack/release são constantes nesta fase
// (Fase 10 expõe no UI se necessário). Posicionado ÚLTIMO na FxChain (pós-reverb/delay).
class SidechainCompressor {
  public:
    void prepare(double sample_rate, int max_block_size) noexcept;
    void process(juce::AudioBuffer<float>& buffer, float threshold_db) noexcept;
    void reset() noexcept;

  private:
    double sample_rate_ = 44100.0;
    bool is_prepared_ = false;
    float env_ = 0.0f;           // envelope follower state (linear amplitude, ≥0)
    float attack_coeff_ = 0.0f;  // IIR coeff for attack (key > env_)
    float release_coeff_ = 0.0f; // IIR coeff for release (key < env_)

    static constexpr float k_ratio = 8.0f;       // 8:1 — pump agressivo
    static constexpr float k_attack_s = 0.005f;  // 5ms — detecta transiente rápido
    static constexpr float k_release_s = 0.200f; // 200ms — release lento → swell característico
};
