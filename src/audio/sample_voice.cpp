#include "sample_voice.h"

#include <cmath>

void SampleVoice::trigger(const float* data, int num_samples, float gain) noexcept {
    data_ = data;
    num_samples_ = num_samples;
    position_ = 0;
    gain_ = gain;
    active_ = true;
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
        // Aguarda o offset de início (precisão de sample — preenchido por trigger_at em 02-02)
        if (i < start_offset_) {
            continue; // NOLINT(readability-braces-around-statements)
        }

        if (position_ >= num_samples_) { // NOLINT(readability-braces-around-statements)
            active_ = false;
            break;
        }

        float sample = data_[position_++] * gain_;

        // Fade-in linear nas primeiras k_fade_frames amostras (evita clique no ataque)
        if (position_ <= k_fade_frames) {
            sample *= static_cast<float>(position_) / static_cast<float>(k_fade_frames);
        }

        // Fade-out nas últimas k_fade_frames amostras (evita clique no final)
        const int samples_remaining = num_samples_ - position_;
        if (samples_remaining < k_fade_frames) {
            sample *= static_cast<float>(samples_remaining) / static_cast<float>(k_fade_frames);
        }

        // Fade-out por note-off
        if (fade_out_remaining_ > 0) {
            sample *= static_cast<float>(fade_out_remaining_) / static_cast<float>(k_fade_frames);
            if (--fade_out_remaining_ == 0) {
                active_ = false;
                break;
            }
        }

        // Mono → estéreo
        out_left[i] += sample;
        out_right[i] += sample;
    }

    // Zera o offset após o primeiro bloco (próximos blocos começam no frame 0)
    start_offset_ = 0;
}
