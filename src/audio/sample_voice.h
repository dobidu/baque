#pragma once

#include <cstring>

// Representa o estado de reprodução de uma voz de sample.
// Sem alocação dinâmica — todos os campos são de tamanho fixo.
// O ponteiro data_ aponta para um buffer externo (AudioBuffer<float> do processador).
class SampleVoice {
  public:
    SampleVoice() = default;

    // Ativa a voz para reprodução a partir do início do bloco atual.
    // start_offset_ = 0 por padrão; trigger_at() (02-02) usa valor maior.
    void trigger(const float* data, int num_samples, float gain) noexcept;

    // Mistura frames desta voz nos buffers de saída estéreo.
    // Ignora frames antes de start_offset_ (dispatch preciso em 02-02).
    void process(float* out_left, float* out_right, int num_frames) noexcept;

    // Inicia fade-out gracioso (32 amostras) antes de desativar.
    void note_off() noexcept;

    [[nodiscard]] bool is_active() const noexcept { return active_; }
    [[nodiscard]] int get_position() const noexcept { return position_; }

    // Usado por VoicePool::trigger_at (02-02) para dispatch preciso.
    int start_offset_ = 0;

  private:
    const float* data_ = nullptr;
    int num_samples_ = 0;
    int position_ = 0;
    bool active_ = false;
    float gain_ = 1.0f;

    // Fade-out: amostras restantes no fade de desativação
    int fade_out_remaining_ = 0;
    static constexpr int k_fade_frames = 32;
};
