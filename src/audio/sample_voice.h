#pragma once

#include "pad_bank.h"

#include <cstring>

// Representa o estado de reprodução de uma voz de sample.
// Sem alocação dinâmica — todos os campos são de tamanho fixo.
// O ponteiro data_ aponta para um buffer externo (pad do PadBank ou
// AudioBuffer<float> do processador) — ver contrato single-writer em pad_bank.h.
//
// Fase 4 (04-01): posição fracionária (interpolação linear), taxa varispeed,
// reverso e pan equal-power. Terminação com limites garantidos:
// - forward: desativa quando position_ >= num_samples_ - 1
// - reverse: desativa quando position_ <= 0
// O par de interpolação (floor(pos), floor(pos)+1) fica sempre dentro do buffer.
class SampleVoice {
  public:
    SampleVoice() = default;

    // Ativa a voz para reprodução a partir do início do bloco atual.
    // rate: taxa varispeed (1.0 = original; 2.0 = +12 st, metade da duração).
    // reverse: reprodução invertida (começa no fim do sample).
    // pan: -1 (esq) .. +1 (dir), lei equal-power calculada aqui (não por frame).
    // adsr/play_mode/sample_rate: parâmetros de envelope e comportamento (Fase 4 T3).
    // start_offset_ = 0 por padrão; trigger_at() (02-02) usa valor maior.
    void trigger(const float* data,
                 int num_samples,
                 float gain,
                 double rate = 1.0,
                 bool reverse = false,
                 float pan = 0.0f,
                 int pad_index = -1,
                 const AdsrParams& adsr = {},
                 PlayMode play_mode = PlayMode::one_shot,
                 double sample_rate = 48000.0) noexcept;

    // Mistura frames desta voz nos buffers de saída estéreo.
    // Ignora frames antes de start_offset_ (dispatch preciso em 02-02).
    void process(float* out_left, float* out_right, int num_frames) noexcept;

    // Inicia release ADSR (gate/loop) ou fade-out legado (one_shot ignorado — use choke()).
    void note_off() noexcept;

    // Força fade-out de 32 amostras independente do play_mode.
    // Usado por VoicePool::choke_group() — choke é externo ao comportamento de nota.
    void choke() noexcept {
        if (active_ && fade_out_remaining_ == 0)
            fade_out_remaining_ = k_fade_frames;
    }

    [[nodiscard]] bool is_active() const noexcept { return active_; }

    // Frames de saída produzidos desde o trigger — usado pelo roubo de voz
    // do VoicePool (voz com mais frames produzidos = mais antiga).
    // (Fase 4: a posição no sample é fracionária e decresce no reverso,
    //  então "frames produzidos" é a métrica estável de idade da voz.)
    [[nodiscard]] int get_position() const noexcept { return frames_rendered_; }

    // Índice do pad que ativou esta voz (-1 = não rastreado).
    // Usado pelo Scheduler para roteamento de note-off por nota e por
    // VoicePool::choke_group() para identificar o grupo de choke.
    [[nodiscard]] int pad_index() const noexcept { return pad_index_; }

    // Usado por VoicePool::trigger_at (02-02) para dispatch preciso.
    // Semântica INALTERADA da Fase 2: atraso relativo ao início do bloco
    // atual (frames pulados), NÃO uma posição nos dados do sample.
    int start_offset_ = 0;

  private:
    const float* data_ = nullptr;
    int num_samples_ = 0;
    double position_ = 0.0; // Posição fracionária no sample (interpolação linear)
    double rate_ = 1.0;     // Incremento por frame (varispeed)
    bool reverse_ = false;
    bool active_ = false;
    float gain_left_ = 1.0f;  // Ganho do canal esq. (gain × lei de pan)
    float gain_right_ = 1.0f; // Ganho do canal dir.
    int frames_rendered_ = 0; // Frames de saída produzidos (idade da voz — NÃO resetar em loop)
    int pad_index_ = -1;      // Índice do pad dono desta voz; -1 = não rastreado

    // Fade-out: amostras restantes no fade de desativação (legado — substituído por ADSR release)
    int fade_out_remaining_ = 0;
    static constexpr int k_fade_frames = 32;

    // ADSR state machine (Fase 4 T3)
    enum class EnvState : uint8_t { idle, attack, decay, sustain_hold, release };
    EnvState env_state_ = EnvState::sustain_hold;
    float env_level_ = 1.0f;   // multiplicador do envelope [0..1]
    float env_rate_ = 0.0f;    // delta por frame (sinal depende do estágio)
    int env_frames_ = 0;       // frames restantes no estágio atual
    float env_sustain_ = 1.0f; // nível de sustain cached
    int dec_frames_ = 0;       // duração do decay em frames (cache para transição attack→decay)
    float release_ms_ = 0.0f;  // release em ms (cached para note_off())
    double sample_rate_ = 48000.0;
    PlayMode play_mode_ = PlayMode::one_shot;
};
