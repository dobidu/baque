#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cmath>
#include <cstdint>

// Um pad de sample: buffer próprio + parâmetros de reprodução.
//
// CONTRATO DE ESCRITA ÚNICA (single-writer) — INVARIANTE CRÍTICO:
// - Parâmetros (gain, pan, pitch, reverse) e o buffer são escritos SOMENTE
//   fora do audio thread (message/background thread).
// - Escritas só podem ocorrer enquanto NENHUMA voz referencia este pad:
//   o caminho de load DEVE invalidar as vozes antes (VoicePool::reset_all()
//   ou bracket suspendProcessing) — ver BaqueProcessor::prepareToPlay.
// - O audio thread apenas LÊ (ponteiros crus via data()).
// Fases futuras com UI/automação devem promover este contrato para
// atomics ou command queue ANTES de permitir edição ao vivo.
class SamplePad {
  public:
    // Parâmetros de reprodução (escrita única — ver contrato acima)
    float gain = 1.0f;       // Ganho linear do pad
    float pan = 0.0f;        // -1 (esq) .. +1 (dir); equal-power
    int pitch_semitones = 0; // ±24 semitons
    int pitch_cents = 0;     // ±100 cents finos
    bool reverse = false;    // Reprodução invertida (fim → início)

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

  private:
    juce::AudioBuffer<float> buffer_; // Mono (convenção da Fase 2)
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
