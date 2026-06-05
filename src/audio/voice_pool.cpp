#include "voice_pool.h"

#include <cstring>

SampleVoice* VoicePool::allocate() noexcept {
    // Fase 1: busca uma voz inativa
    for (auto& v : voices_) {
        if (!v.is_active())
            return &v;
    }

    // Fase 2: pool cheio — rouba a voz com posição mais avançada (mais próxima de terminar)
    // RT-safe: apenas comparações de inteiros, sem sort, sem lock, sem alocação
    SampleVoice* oldest = &voices_[0];
    for (auto& v : voices_) {
        if (v.get_position() > oldest->get_position())
            oldest = &v;
    }
    return oldest; // Nunca retorna nullptr — o chamador sempre obtém uma voz válida
}

void VoicePool::process_all(float* out_left, float* out_right, int num_frames) noexcept {
    // Zera os buffers de saída antes de misturar
    std::memset(out_left, 0, static_cast<size_t>(num_frames) * sizeof(float));
    std::memset(out_right, 0, static_cast<size_t>(num_frames) * sizeof(float));

    for (auto& v : voices_) {
        if (v.is_active())
            v.process(out_left, out_right, num_frames);
    }
}

void VoicePool::reset_all() noexcept {
    for (auto& v : voices_)
        v = SampleVoice{}; // Reinicia para o estado padrão (inativo)
}

void VoicePool::trigger_at(int start_offset, const float* sample_data, int num_samples, float gain) noexcept {
    // RT-safe: allocate() nunca retorna nullptr (roubo obrigatório)
    SampleVoice* voice = allocate();
    voice->trigger(sample_data, num_samples, gain);
    voice->start_offset_ = start_offset; // Sobrescreve o 0 padrão de trigger()
}
