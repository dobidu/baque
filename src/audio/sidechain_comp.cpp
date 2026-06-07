#include "sidechain_comp.h"

#include <algorithm>
#include <cmath>

void SidechainCompressor::prepare(double sample_rate, int /*max_block_size*/) noexcept {
    sample_rate_ = sample_rate;
    env_ = 0.0f;
    const float sr = static_cast<float>(sample_rate);
    attack_coeff_ = std::exp(-1.0f / (k_attack_s * sr));
    release_coeff_ = std::exp(-1.0f / (k_release_s * sr));
    is_prepared_ = true;
}

void SidechainCompressor::process(juce::AudioBuffer<float>& buffer, float threshold_db) noexcept {
    jassert(is_prepared_);
    if (!is_prepared_)
        return;

    const int num_samples = buffer.getNumSamples();
    const int num_channels = buffer.getNumChannels();
    if (num_samples == 0 || num_channels == 0)
        return;

    auto* left = buffer.getWritePointer(0);
    auto* right = (num_channels > 1) ? buffer.getWritePointer(1) : nullptr;

    // Clamp antes da conversão — NaN/extremos de APVTS causariam buffer de NaN (SR2, audit 06-03)
    const float threshold_linear = juce::Decibels::decibelsToGain(juce::jlimit(-80.0f, 0.0f, threshold_db));

    for (int i = 0; i < num_samples; ++i) {
        // Key signal: mono peak (trigger interno — sinal da própria saída)
        float key = std::abs(left[i]);
        if (right)
            key = std::max(key, std::abs(right[i]));

        // Envelope follower IIR assimétrico
        const float coeff = (key > env_) ? attack_coeff_ : release_coeff_;
        env_ = coeff * env_ + (1.0f - coeff) * key;

        // Gain computer: hard knee no threshold
        float gain = 1.0f;
        if (env_ > threshold_linear) {
            const float env_db = juce::Decibels::gainToDecibels(env_);
            const float excess_db = env_db - threshold_db;             // > 0
            const float gain_db = excess_db * (1.0f / k_ratio - 1.0f); // < 0
            gain = juce::Decibels::decibelsToGain(gain_db);            // < 1.0
        }

        left[i] *= gain;
        if (right)
            right[i] *= gain;
    }
}

void SidechainCompressor::reset() noexcept {
    env_ = 0.0f;
}
