#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <array>
#include <cmath>
#include <cstdint>

// Uma camada de velocity: buffer próprio + faixa MIDI [vel_lo, vel_hi].
// Contrato single-writer idêntico ao buffer_ do SamplePad.
struct VelocityLayer {
    juce::AudioBuffer<float> buffer;
    uint8_t vel_lo = 0;
    uint8_t vel_hi = 127;
    [[nodiscard]] bool has_sample() const noexcept { return buffer.getNumSamples() > 0; }
    [[nodiscard]] const float* data() const noexcept { return has_sample() ? buffer.getReadPointer(0) : nullptr; }
    [[nodiscard]] int num_samples() const noexcept { return buffer.getNumSamples(); }
};

enum class PlayMode : uint8_t {
    one_shot = 0, // Reproduz até o fim; ignora note-off (padrão)
    gate,         // Para no note-off (inicia fase de release)
    loop,         // Repete o sample até note-off; release após note-off
};

struct AdsrParams {
    float attack_ms = 0.0f;  // 0 = sem ramp-up
    float decay_ms = 0.0f;   // 0 = instantâneo até sustain
    float sustain = 1.0f;    // Nível 0..1 durante sustain
    float release_ms = 0.0f; // 0 = usa k_fade_frames (comportamento legado)
};

// Um pad de sample: buffer próprio + parâmetros de reprodução.
//
// CONTRATO DE PARÂMETROS (Fase 10-01):
// - Parâmetros de reprodução (gain/pan/pitch/reverse/choke/adsr/play_mode): mutação
//   ao vivo via BaqueProcessor::push_ui_command(set_pad_*). Audio thread lê após drain.
// - Buffer de sample: carregamento APENAS fora do audio thread com VoicePool::reset_all()
//   antes (protocolo safe-load — Fase 4, auditoria 04-01). Buffers NÃO passam pela fila.
// - O audio thread lê ponteiros crus via data() — sem alloc na hot path.
class SamplePad {
  public:
    // Parâmetros de reprodução (escrita única — ver contrato acima)
    float gain = 1.0f;       // Ganho linear do pad
    float pan = 0.0f;        // -1 (esq) .. +1 (dir); equal-power
    int pitch_semitones = 0; // ±24 semitons
    int pitch_cents = 0;     // ±100 cents finos
    bool reverse = false;    // Reprodução invertida (fim → início)
    uint8_t choke_group = 0; // 0 = sem choke; 1–8 = grupo (ex.: hi-hat fecha grupo 1)
    AdsrParams adsr{};
    PlayMode play_mode = PlayMode::one_shot;

    // Caminho do arquivo fonte — salvo em getStateInformation para recarregar no load.
    // Vazio se carregado de BinaryData ou nunca carregado via load_sample_from_file.
    juce::File source_file_{};
    [[nodiscard]] const juce::File& source_file() const noexcept { return source_file_; }
    void set_source_file(const juce::File& f) noexcept { source_file_ = f; }

    // Taxa de reprodução varispeed: 2^((semitons + cents/100) / 12).
    // Pitch e duração mudam juntos (caráter SP/vinil). Calculada fora do
    // loop por-frame — chamar uma vez por trigger.
    [[nodiscard]] double playback_rate() const noexcept {
        const double total_semitones = static_cast<double>(pitch_semitones) + static_cast<double>(pitch_cents) / 100.0;
        return std::exp2(total_semitones / 12.0);
    }

    [[nodiscard]] bool has_sample() const noexcept { return buffer_.getNumSamples() > 0; }
    [[nodiscard]] const float* data() const noexcept { return has_sample() ? buffer_.getReadPointer(0) : nullptr; }
    [[nodiscard]] int num_samples() const noexcept { return buffer_.getNumSamples(); }

    // Acesso ao buffer para carregamento — APENAS fora do audio thread,
    // APENAS após invalidar as vozes (contrato single-writer acima).
    [[nodiscard]] juce::AudioBuffer<float>& sample_buffer() noexcept { return buffer_; }

    // Camadas de velocity (single-writer; se num_layers_==0, usa buffer_ diretamente).
    static constexpr int k_max_layers = 8;

    // Acesso RT-safe de leitura (audio thread):
    [[nodiscard]] const VelocityLayer& layer_at(int i) const noexcept { return layers_[static_cast<size_t>(i)]; }
    [[nodiscard]] int num_layers() const noexcept { return num_layers_; }

    // Acesso de escrita (off-audio-thread, single-writer):
    [[nodiscard]] VelocityLayer& layer(int i) noexcept { return layers_[static_cast<size_t>(i)]; }
    void set_num_layers(int n) noexcept { num_layers_ = (n < 0) ? 0 : (n > k_max_layers ? k_max_layers : n); }

  private:
    juce::AudioBuffer<float> buffer_; // Mono (convenção da Fase 2)
    std::array<VelocityLayer, k_max_layers> layers_{};
    int num_layers_ = 0;
};

// Banco de 16 pads (4×4 — ESCOPO §4.1; constante pronta para crescer a 8×8).
// Mapeamento de nota: índice do pad = nota − 36 (k_base_note, convenção
// do StepPattern da Fase 3; 16 pads = notas 36–51).
// Lookup aritmético em array — sem map, sem alocação, RT-safe.
class PadBank {
  public:
    static constexpr int k_num_pads = 16;
    static constexpr int k_base_note = 36; // C2 = pad 0 (bumbo por convenção)

    // Retorna o pad para a nota MIDI, ou nullptr se fora de [36, 52)
    // ou se o pad não tem sample carregado. RT-safe.
    [[nodiscard]] const SamplePad* pad_for_note(uint8_t note) const noexcept {
        const int index = static_cast<int>(note) - k_base_note;
        if (index < 0 || index >= k_num_pads) {
            return nullptr;
        }
        const SamplePad& p = pads_[static_cast<size_t>(index)];
        return p.has_sample() ? &p : nullptr;
    }

    // Acesso direto por índice para carregamento (fora do audio thread).
    [[nodiscard]] SamplePad& pad(int index) noexcept { return pads_[static_cast<size_t>(index)]; }
    [[nodiscard]] const SamplePad& pad(int index) const noexcept { return pads_[static_cast<size_t>(index)]; }

  private:
    std::array<SamplePad, k_num_pads> pads_;
};
