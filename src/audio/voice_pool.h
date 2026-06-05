#pragma once

#include "sample_voice.h"

#include <array>
#include <cstdint>

class PadBank; // forward decl — full def in voice_pool.cpp via pad_bank.h

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

    // Armazena sample_rate para repassar às vozes em trigger_at (ADSR ms→frames).
    // Deve ser chamado em prepareToPlay antes do primeiro process().
    void prepare(double sample_rate) noexcept;

    // Dispara uma voz com offset de início preciso em samples (para dispatch em 02-02).
    // start_offset = posição dentro do bloco atual a partir da qual a voz começa.
    // Fase 4 (04-01): rate (varispeed), reverse e pan repassados à voz.
    // Fase 4 (04-02): pad_index rastreado na voz para choke e roteamento de note-off.
    // Fase 4 (04-02 T3): adsr e play_mode repassados à voz para envelope e comportamento.
    // Retorna ponteiro não-proprietário para a voz alocada (nunca nullptr — roubo garante).
    // RT-safe: sem alocação.
    [[nodiscard]] SampleVoice* trigger_at(int start_offset,
                                          const float* sample_data,
                                          int num_samples,
                                          float gain,
                                          double rate = 1.0,
                                          bool reverse = false,
                                          float pan = 0.0f,
                                          int pad_index = -1,
                                          const AdsrParams& adsr = {},
                                          PlayMode play_mode = PlayMode::one_shot) noexcept;

    // Chama note_off() em todas as vozes ativas cujo pad pertence ao grupo de choke.
    // Deve ser chamado ANTES de trigger_at() para o novo note-on.
    // RT-safe: iteração linear sem alocação.
    void choke_group(uint8_t group, const PadBank& bank) noexcept;

  private:
    std::array<SampleVoice, k_pool_size> voices_;
    double sample_rate_ = 48000.0;
};
