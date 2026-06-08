#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdint>

// FX de performance "gater": gate de amplitude beat-synced (1/16 de nota), com gate_depth [0,1]
// (0 = off/bypass, 1 = chop total → silêncio na metade OFF). Duty 50% (1ª metade ON, 2ª OFF).
// Bordas com fade curto (~2 ms) anti-click. RT-safe, até 2 canais. Sem alocação em process().
//
// Fase do gate via contador interno (sample_counter_ % period), persiste entre blocos e é
// COMPARTILHADA entre L e R (coerência estéreo). bpm clampeado a >= 1.
class GaterProcessor {
  public:
    void prepare(double sample_rate, int max_block_size) noexcept;
    void process(juce::AudioBuffer<float>& buffer, float gate_depth, double bpm) noexcept;
    void reset() noexcept;

    // Período do gate em samples (uma semicolcheia = 1/16 de nota).
    [[nodiscard]] static int gate_period_samples(double sample_rate, double bpm) noexcept;

  private:
    static constexpr float k_eps = 1.0e-4f;

    double sample_rate_ = 44100.0;
    int64_t sample_counter_ = 0; // fase contínua entre blocos
    int fade_len_ = 96;          // ~2 ms a 48 kHz (recomputado em prepare)
};
