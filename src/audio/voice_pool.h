#pragma once

#include "sample_voice.h"

#include <array>

// Pool de vozes pré-alocado — sem alocação dinâmica após a construção.
// Todas as 64 vozes residem em um std::array estático.
class VoicePool {
  public:
    static constexpr int k_pool_size = 64; // Alvo ESCOPO §9: ≥64 vozes

    VoicePool() = default;

    // Retorna uma voz disponível para ativação.
    // NUNCA retorna nullptr: se o pool estiver cheio, rouba a voz mais antiga.
    // RT-safe: sem alocação, sem lock.
    SampleVoice* allocate() noexcept;

    // Processa todas as vozes ativas nos buffers de saída estéreo.
    // Zera os buffers antes de misturar.
    void process_all(float* out_left, float* out_right, int num_frames) noexcept;

    // Desativa todas as vozes (chamado em prepareToPlay).
    void reset_all() noexcept;

  private:
    std::array<SampleVoice, k_pool_size> voices_;
};
