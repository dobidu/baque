#include "sample_voice.h"

#include <cassert>
#include <cmath>

namespace {
constexpr float k_pi = 3.14159265358979f;
} // namespace

void SampleVoice::trigger(const float* data,
                          int num_samples,
                          float gain,
                          double rate,
                          bool reverse,
                          float pan,
                          int pad_index,
                          const AdsrParams& adsr,
                          PlayMode play_mode,
                          double sample_rate) noexcept {
    data_ = data;
    num_samples_ = num_samples;
    rate_ = rate;
    reverse_ = reverse;
    pad_index_ = pad_index;
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

    // ADSR init — ms → frames (uma multiplicação por trigger, não por frame)
    play_mode_ = play_mode;
    env_sustain_ = adsr.sustain;
    release_ms_ = adsr.release_ms;
    sample_rate_ = sample_rate;

    const auto sr = static_cast<float>(sample_rate);
    const int atk_f = static_cast<int>(adsr.attack_ms * 0.001f * sr + 0.5f);
    dec_frames_ = static_cast<int>(adsr.decay_ms * 0.001f * sr + 0.5f);

    if (atk_f > 0) {
        env_state_ = EnvState::attack;
        env_level_ = 0.0f;
        env_rate_ = 1.0f / static_cast<float>(atk_f);
        env_frames_ = atk_f;
    } else if (dec_frames_ > 0) {
        env_state_ = EnvState::decay;
        env_level_ = 1.0f;
        env_rate_ = (1.0f - adsr.sustain) / static_cast<float>(dec_frames_);
        env_frames_ = dec_frames_;
    } else {
        env_state_ = EnvState::sustain_hold;
        env_level_ = adsr.sustain;
        env_rate_ = 0.0f;
        env_frames_ = 0;
    }
}

void SampleVoice::note_off() noexcept {
    if (!active_)
        return;

    // one_shot: ignora note-off completamente
    if (play_mode_ == PlayMode::one_shot)
        return;

    // gate / loop: inicia release (idempotente se já em release)
    if (env_state_ == EnvState::release)
        return;

    // Sustain=0 → envelope já em zero → desativa imediatamente (evita vazamento de voz)
    if (env_level_ <= 0.0f) {
        active_ = false;
        return;
    }

    if (fade_out_remaining_ == 0) {
        // Calcula duração de release
        const int rel_f = (release_ms_ > 0.0f)
                              ? static_cast<int>(release_ms_ * 0.001f * static_cast<float>(sample_rate_) + 0.5f)
                              : k_fade_frames; // legado: 32 frames

        env_state_ = EnvState::release;
        env_rate_ = (rel_f > 0) ? (-env_level_ / static_cast<float>(rel_f)) : -1.0f;
        env_frames_ = rel_f;
    }
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

        // Loop wrap (play_mode=loop) — checar ANTES do fim-de-sample
        if (reverse_ ? (position_ <= 0.0) : (position_ >= static_cast<double>(num_samples_ - 1))) {
            if (play_mode_ == PlayMode::loop && env_state_ != EnvState::release) {
                // Wrap para o início; NÃO resetar frames_rendered_ (métrica de roubo de voz)
                position_ = reverse_ ? static_cast<double>(num_samples_ - 1) : 0.0;
            } else {
                active_ = false;
                break;
            }
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

        // Fade-in linear nos primeiros k_fade_frames frames quando sem ADSR attack
        // (quando attack_ms > 0, env_level_ sobe de 0 — ADSR faz a rampa, fade-in duplicaria)
        if (frames_rendered_ < k_fade_frames && env_state_ != EnvState::attack) {
            sample *= static_cast<float>(frames_rendered_ + 1) / static_cast<float>(k_fade_frames);
        }

        // Fade-out nas proximidades do fim (em frames de saída restantes; anti-click varispeed)
        const double remaining_source = reverse_ ? position_ : (static_cast<double>(num_samples_ - 1) - position_);
        const double remaining_frames = remaining_source / rate_;
        if (remaining_frames < static_cast<double>(k_fade_frames)) {
            sample *= static_cast<float>(remaining_frames) / static_cast<float>(k_fade_frames);
        }

        // Envelope ADSR: aplica e avança o estado
        sample *= env_level_;
        switch (env_state_) {
        case EnvState::attack:
            env_level_ += env_rate_;
            if (--env_frames_ <= 0 || env_level_ >= 1.0f) {
                env_level_ = 1.0f;
                if (dec_frames_ > 0) {
                    env_state_ = EnvState::decay;
                    env_rate_ = (1.0f - env_sustain_) / static_cast<float>(dec_frames_);
                    env_frames_ = dec_frames_;
                } else {
                    env_state_ = EnvState::sustain_hold;
                    env_level_ = env_sustain_;
                }
            }
            break;
        case EnvState::decay:
            env_level_ -= env_rate_;
            if (--env_frames_ <= 0 || env_level_ <= env_sustain_) {
                env_level_ = env_sustain_;
                env_state_ = EnvState::sustain_hold;
            }
            break;
        case EnvState::sustain_hold:
            break;
        case EnvState::release:
            env_level_ += env_rate_; // env_rate_ é negativo
            if (--env_frames_ <= 0 || env_level_ <= 0.0f) {
                env_level_ = 0.0f;
                active_ = false;
            }
            break;
        default:
            break;
        }

        // Fade-out legado por note-off (pré-ADSR — active apenas se fade_out_remaining_ > 0)
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

        if (!active_)
            break; // Sai do loop sem goto após ADSR release completar

        // Avança a posição fracionária (varispeed; decresce no reverso)
        position_ += reverse_ ? -rate_ : rate_;
    }

    // Zera o offset após o primeiro bloco (próximos blocos começam no frame 0)
    start_offset_ = 0;
}
