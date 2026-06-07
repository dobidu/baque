#pragma once
#include "fx_params.h"
#include "sidechain_comp.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Cadeia de FX de áudio do BAQUE — filtro, reverb, delay, sidechain compressor com SmoothedValue.
// RT-safe: sem alocação, sem locks em process(). prepare() aloca; chamar off audio thread.
// sidechain_threshold em FxParams drive SidechainCompressor (Fase 6-03).
class FxChain {
  public:
    // Aloca buffers internos e configura DSP. Chamar em prepareToPlay(), nunca em processBlock().
    void prepare(double sample_rate, int max_block_size) noexcept;

    // Aplica cadeia de FX ao buffer estéreo. fx_params define alvos dos SmoothedValues.
    // Chamado em processBlock() após renderização de vozes + aplicação de ganho.
    void process(juce::AudioBuffer<float>& buffer, const FxParams& params) noexcept;

    // Zera estado interno (delay buffer, filter state, reverb tail). Chamar em releaseResources().
    void reset() noexcept;

  private:
    double sample_rate_ = 44100.0;
    bool is_prepared_ = false;

    // Filtro passa-baixas (StateVariableTPT — um por canal para processamento per-sample)
    juce::dsp::StateVariableTPTFilter<float> filter_l_;
    juce::dsp::StateVariableTPTFilter<float> filter_r_;
    juce::SmoothedValue<float> cutoff_smoother_;
    juce::SmoothedValue<float> res_smoother_;

    // Reverb (juce::dsp::Reverb — estéreo nativo)
    juce::dsp::Reverb reverb_;
    juce::SmoothedValue<float> reverb_mix_smoother_;

    // Delay estéreo com interpolação linear
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay_line_{192000};
    juce::SmoothedValue<float> delay_mix_smoother_;
    juce::SmoothedValue<float> delay_time_smoother_;

    // Sidechain compressor — envelope follower + gain computer, trigger interno (Fase 6-03)
    SidechainCompressor sidechain_comp_;

    static constexpr float k_smooth_time_s = 0.02f;  // 20ms ramp — sem cliques em p-lock
    static constexpr float k_delay_feedback = 0.45f; // feedback 45% — sem self-oscillation
    static constexpr float k_reverb_room = 0.6f;
    static constexpr float k_reverb_damp = 0.5f;
    static constexpr float k_reverb_width = 1.0f;
};
