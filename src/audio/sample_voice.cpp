#include "sample_voice.h"

#include <cassert>
#include <cmath>

namespace {
constexpr float k_pi = 3.14159265358979f;
} // namespace

void SampleVoice::trigger(
    const float* data, int num_samples, float gain, double rate, bool reverse, float pan) noexcept {
    data_ = data;
    num_samples_ = num_samples;
    rate_ = rate;
    reverse_ = reverse;
    // Reverso começa no fim do sample; forward no início
    position_ = reverse ? static_cast<double>(num_samples - 1) : 0.0;

    // Lei de pan equal-power calculada UMA vez no trigger (não por frame):
    // pan -1 → (1, 0); 0 → (0.707, 0.707); +1 → (0, 1)
    const float angle = (pan + 1.0f) * k_pi * 0.25f;
    gain_left_ = gain * std::cos(angle);
    gain_right_ = gain * std::sin(angle);

    active_ = true;
    frames_rendered_ = 0;
    fade_out_remaining_ = 0;
    start_offset_ = 0; // Padrão; trigger_at() sobrescreve após esta chamada
}

void SampleVoice::note_off() noexcept {
    if (active_ && fade_out_remaining_ == 0)
        fade_out_remaining_ = k_fade_frames;
}

void SampleVoice::process(float* out_left, float* out_right, int num_frames) noexcept {
    if (!active_ || data_ == nullptr)
        return;

    for (int i = 0; i < num_frames; ++i) {
        // Aguarda o offset de início — semântica da Fase 2 INALTERADA:
        // atraso relativo ao bloco atual, vale igualmente para o reverso
        if (i < start_offset_) {
            continue; // NOLINT(readability-braces-around-statements)
        }

        // Terminação com limites garantidos (auditoria 04-01):
        // forward desativa em position_ >= num_samples_ - 1;
        // reverse desativa em position_ <= 0.
        // Assim floor(pos)+1 nunca lê além do buffer.
        if (reverse_ ? (position_ <= 0.0) : (position_ >= static_cast<double>(num_samples_ - 1))) {
            active_ = false;
            break;
        }

        // Interpolação linear entre floor(pos) e floor(pos)+1
        int index = static_cast<int>(position_);
        if (index > num_samples_ - 2) {
            index = num_samples_ - 2; // Borda do reverso: pos inicial == num_samples-1
        }
        const float frac = static_cast<float>(position_ - static_cast<double>(index));
        assert(index >= 0 && index + 1 < num_samples_); // Invariante de limites (auditoria)
        const float interpolated = data_[index] + frac * (data_[index + 1] - data_[index]);

        float sample = interpolated;

        // Fade-in linear nos primeiros k_fade_frames frames DE SAÍDA (evita clique
        // no ataque; baseado em frames produzidos — válido para qualquer rate/direção)
        if (frames_rendered_ < k_fade_frames) {
            sample *= static_cast<float>(frames_rendered_ + 1) / static_cast<float>(k_fade_frames);
        }

        // Fade-out nas proximidades do fim (em frames de saída restantes,
        // compensando a taxa — evita clique no final em qualquer varispeed)
        const double remaining_source = reverse_ ? position_ : (static_cast<double>(num_samples_ - 1) - position_);
        const double remaining_frames = remaining_source / rate_;
        if (remaining_frames < static_cast<double>(k_fade_frames)) {
            sample *= static_cast<float>(remaining_frames) / static_cast<float>(k_fade_frames);
        }

        // Fade-out por note-off
        if (fade_out_remaining_ > 0) {
            sample *= static_cast<float>(fade_out_remaining_) / static_cast<float>(k_fade_frames);
            if (--fade_out_remaining_ == 0) {
                active_ = false;
                out_left[i] += sample * gain_left_;
                out_right[i] += sample * gain_right_;
                ++frames_rendered_;
                break;
            }
        }

        // Mono → estéreo com pan equal-power
        out_left[i] += sample * gain_left_;
        out_right[i] += sample * gain_right_;
        ++frames_rendered_;

        // Avança a posição fracionária (varispeed; decresce no reverso)
        position_ += reverse_ ? -rate_ : rate_;
    }

    // Zera o offset após o primeiro bloco (próximos blocos começam no frame 0)
    start_offset_ = 0;
}
