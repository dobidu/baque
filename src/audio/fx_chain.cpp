#include "fx_chain.h"

void FxChain::prepare(double sample_rate, int max_block_size) noexcept {
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    is_prepared_ = true;

    lo_fi_.prepare(sample_rate, max_block_size);
    granular_.prepare(sample_rate, max_block_size);

    const auto block_size = static_cast<juce::uint32>(max_block_size);

    // Filtro: mono spec por canal (processSample per-channel)
    juce::dsp::ProcessSpec mono_spec{sample_rate, block_size, 1};
    filter_l_.prepare(mono_spec);
    filter_r_.prepare(mono_spec);
    filter_l_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter_r_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    // Reverb: spec estéreo (juce::dsp::Reverb lida com mono/estéreo em runtime via channel count)
    juce::dsp::ProcessSpec stereo_spec{sample_rate, block_size, 2};
    reverb_.prepare(stereo_spec);

    // Delay: spec estéreo — canal 0=L, canal 1=R
    delay_line_.prepare(stereo_spec);

    sidechain_comp_.prepare(sample_rate, max_block_size);

    // SmoothedValues: 20ms ramp; setCurrentAndTargetValue evita artefato no primeiro bloco
    cutoff_smoother_.reset(sample_rate, k_smooth_time_s);
    cutoff_smoother_.setCurrentAndTargetValue(20000.0f);

    res_smoother_.reset(sample_rate, k_smooth_time_s);
    res_smoother_.setCurrentAndTargetValue(0.0f); // FxParams::filter_res [0,1] default 0.1

    reverb_mix_smoother_.reset(sample_rate, k_smooth_time_s);
    reverb_mix_smoother_.setCurrentAndTargetValue(0.0f);

    delay_mix_smoother_.reset(sample_rate, k_smooth_time_s);
    delay_mix_smoother_.setCurrentAndTargetValue(0.0f);

    delay_time_smoother_.reset(sample_rate, k_smooth_time_s);
    delay_time_smoother_.setCurrentAndTargetValue(0.25f);
}

void FxChain::process(juce::AudioBuffer<float>& buffer, const FxParams& params) noexcept {
    jassert(is_prepared_);
    if (!is_prepared_)
        return;

    const int num_samples = buffer.getNumSamples();
    const int num_channels = buffer.getNumChannels();

    auto* left = buffer.getWritePointer(0);
    auto* right = num_channels > 1 ? buffer.getWritePointer(1) : left;

    // Define alvos dos SmoothedValues a partir dos FxParams do bloco atual
    cutoff_smoother_.setTargetValue(params.filter_cutoff);
    res_smoother_.setTargetValue(params.filter_res);
    reverb_mix_smoother_.setTargetValue(params.reverb_mix);
    delay_mix_smoother_.setTargetValue(params.delay_mix);
    delay_time_smoother_.setTargetValue(params.delay_time);

    // Avança smoothers ao longo do bloco; usa valor final para atualização de coeficiente
    // (block-rate: coeficiente constante dentro do bloco — SR2 D2: deferred per-sample)
    const float cutoff = juce::jlimit(20.0f, 20000.0f, cutoff_smoother_.skip(num_samples));
    const float res_q = juce::jlimit(0.1f, 20.0f, res_smoother_.skip(num_samples) * 20.0f + 0.1f);
    const float rev_mix = juce::jlimit(0.0f, 1.0f, reverb_mix_smoother_.skip(num_samples));
    const float dly_mix = juce::jlimit(0.0f, 1.0f, delay_mix_smoother_.skip(num_samples));
    const float dly_t = juce::jlimit(0.001f, 2.0f, delay_time_smoother_.skip(num_samples));

    // --- LoFi (primeiro: coloration na fonte, antes do FX chain) ---
    // bit_depth/sr_factor aplicados direto — lo-fi e chaveamento discreto, sem ramp SmoothedValue.
    // Adicionar SmoothedValue aqui silenciaria transientemente ao mudar preset — comportamento errado.
    lo_fi_.process(buffer, params.bit_depth, params.sr_factor);

    // --- Granular (segundo: clouds/freeze, após lo-fi, antes do FX chain) ---
    // granular_freeze convertido float→bool: >= 0.5f = true.
    // Sem SmoothedValue: freeze é chaveamento discreto, não crossfade.
    // Bypass quando todos os params são neutros (spray=0, pitch=0, freeze=false):
    // grains lêem do capture buffer — se vazio, output seria silêncio (destruiria sinal).
    // Passthrough em params neutros é correto: sem efeito audível esperado.
    // wet/dry mix deferred to Phase 10 UI.
    const bool granular_freeze = params.granular_freeze >= 0.5f;
    if (params.granular_spray > 0.0f || params.granular_pitch_spread > 0.0f || granular_freeze)
        granular_.process(buffer, params.granular_spray, params.granular_pitch_spread, granular_freeze);

    // --- Filtro LP ---
    filter_l_.setCutoffFrequency(cutoff);
    filter_r_.setCutoffFrequency(cutoff);
    filter_l_.setResonance(res_q);
    filter_r_.setResonance(res_q);

    for (int i = 0; i < num_samples; ++i) {
        left[i] = filter_l_.processSample(0, left[i]);
        if (num_channels > 1)
            right[i] = filter_r_.processSample(0, right[i]);
    }

    // --- Reverb ---
    // KNOWN PATTERN (audit SR1): setParameters() chamado todo bloco mesmo sem mudança.
    // RT-safe: sem alocação, noexcept — recalcula coeficientes Comb/AllPass in-place.
    // Dirty-flag optimization deferred to Phase 12 (Hardening).
    juce::dsp::Reverb::Parameters rp{};
    rp.roomSize = k_reverb_room;
    rp.damping = k_reverb_damp;
    rp.wetLevel = rev_mix;
    rp.dryLevel = 1.0f - rev_mix;
    rp.width = k_reverb_width;
    rp.freezeMode = 0.0f;
    reverb_.setParameters(rp);

    juce::dsp::AudioBlock<float> block(buffer);
    reverb_.process(juce::dsp::ProcessContextReplacing<float>(block));

    // --- Delay com feedback ---
    const int delay_samples = juce::roundToInt(dly_t * static_cast<float>(sample_rate_));
    delay_line_.setDelay(static_cast<float>(delay_samples));

    // INVARIANT (audit SR2): pop BEFORE push — lê sample atrasado primeiro, depois escreve
    // o sample atual no delay buffer. Inverter a ordem introduz erro de 1 sample no feedback.
    for (int i = 0; i < num_samples; ++i) {
        const float wet_l = delay_line_.popSample(0);
        const float wet_r = (num_channels > 1) ? delay_line_.popSample(1) : wet_l;

        delay_line_.pushSample(0, left[i] + wet_l * k_delay_feedback);
        if (num_channels > 1)
            delay_line_.pushSample(1, right[i] + wet_r * k_delay_feedback);

        left[i] = left[i] * (1.0f - dly_mix) + wet_l * dly_mix;
        if (num_channels > 1)
            right[i] = right[i] * (1.0f - dly_mix) + wet_r * dly_mix;
    }

    // --- Sidechain Compressor ---
    sidechain_comp_.process(buffer, params.sidechain_threshold);
}

void FxChain::reset() noexcept {
    lo_fi_.prepare(sample_rate_, max_block_size_);
    granular_.reset();

    filter_l_.reset();
    filter_r_.reset();
    reverb_.reset();
    delay_line_.reset();

    sidechain_comp_.reset();

    // Fixa SmoothedValues no target atual — evita ramp ao re-preparar
    cutoff_smoother_.setCurrentAndTargetValue(cutoff_smoother_.getTargetValue());
    res_smoother_.setCurrentAndTargetValue(res_smoother_.getTargetValue());
    reverb_mix_smoother_.setCurrentAndTargetValue(reverb_mix_smoother_.getTargetValue());
    delay_mix_smoother_.setCurrentAndTargetValue(delay_mix_smoother_.getTargetValue());
    delay_time_smoother_.setCurrentAndTargetValue(delay_time_smoother_.getTargetValue());
}
