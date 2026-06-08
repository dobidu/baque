#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

// FX de performance "tape-stop": resample do mix a partir de um ring, com taxa de leitura que cai
// de 1.0 → 0.0 conforme tape_stop [0,1] sobe (pitch + tempo despencam). RT-safe, até 2 canais.
//
// HALT = SILÊNCIO (audit M1): o ganho de saída é derivado do rate (out *= rate suavizado), de modo
// que rate→0 → saída→0. NUNCA segura a última amostra (DC sustentado no mix = risco de thump /
// falha de DC no pluginval). tape_stop <= eps → bypass exato (out=dry), read_pos_ ressincronizado.
// Rate suavizado PER-SAMPLE via SmoothedValue (~30 ms) — sem zipper (audit SR1).
class TapeStopProcessor {
  public:
    void prepare(double sample_rate, int max_block_size) noexcept;
    void process(juce::AudioBuffer<float>& buffer, float tape_stop) noexcept;
    void reset() noexcept;

  private:
    static float read_ring(const std::vector<float>& ring, int ring_size, double pos) noexcept;

    static constexpr float k_eps = 1.0e-4f;

    double sample_rate_ = 44100.0;
    int ring_size_ = 0;
    std::vector<float> ring_l_;
    std::vector<float> ring_r_;
    int write_pos_ = 0;
    double read_pos_ = 0.0;                    // índice fracionário de leitura no ring [0, ring_size_)
    juce::SmoothedValue<float> rate_smoother_; // alvo = 1 - tape_stop; suaviza per-sample
    bool was_active_ = false;                  // detecta borda bypass→ativo (ressync read_pos_)
};
