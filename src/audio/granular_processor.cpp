#include "granular_processor.h"

#include <algorithm>
#include <cmath>

void GranularProcessor::prepare(double sample_rate, int /*max_block_size*/) noexcept {
    sample_rate_ = static_cast<float>(sample_rate);
    grain_len_ = std::min(k_capture_size / 2,
                          static_cast<int>(sample_rate * 0.050)); // 50ms
    grain_interval_ = static_cast<int>(sample_rate * 0.025);      // 25ms density
    reset();
}

void GranularProcessor::reset() noexcept {
    std::fill(capture_l_, capture_l_ + k_capture_size, 0.0f);
    std::fill(capture_r_, capture_r_ + k_capture_size, 0.0f);
    write_pos_ = 0;
    frozen_ = false;
    for (auto& g : grains_)
        g.active = false;
    grain_timer_ = 0; // spawn first grain at first sample of first process() call
}

void GranularProcessor::process(juce::AudioBuffer<float>& buffer,
                                float spray,
                                float pitch_spread,
                                bool freeze) noexcept {
    const int num_ch = std::min(buffer.getNumChannels(), 2);
    const int num_samples = buffer.getNumSamples();
    float* ch0 = num_ch > 0 ? buffer.getWritePointer(0) : nullptr;
    float* ch1 = num_ch > 1 ? buffer.getWritePointer(1) : nullptr;

    frozen_ = freeze;

    for (int i = 0; i < num_samples; ++i) {
        // 1. Capture input into ring buffer (skip if frozen — hold captured state)
        if (!frozen_) {
            capture_l_[write_pos_] = ch0 != nullptr ? ch0[i] : 0.0f;
            capture_r_[write_pos_] = ch1 != nullptr ? ch1[i] : 0.0f;
            write_pos_ = (write_pos_ + 1) & (k_capture_size - 1);
        }

        // 2. Spawn grain when timer elapses
        if (grain_timer_ <= 0) {
            spawn_grain(spray, pitch_spread);
            grain_timer_ = grain_interval_;
        }
        --grain_timer_;

        // 3. Accumulate all active grains (Hann-windowed, linear-interpolated read)
        float out_l = 0.0f, out_r = 0.0f;
        for (auto& g : grains_) {
            if (!g.active)
                continue;
            const float env = hann(g.env_pos, g.length);
            out_l += env * lerp_buf(capture_l_, g.read_pos);
            out_r += env * lerp_buf(capture_r_, g.read_pos);
            g.read_pos += g.rate;
            if (g.read_pos >= static_cast<float>(k_capture_size))
                g.read_pos -= static_cast<float>(k_capture_size);
            if (g.read_pos < 0.0f)
                g.read_pos += static_cast<float>(k_capture_size);
            ++g.env_pos;
            if (g.env_pos >= g.length)
                g.active = false;
        }

        // 4. Replace input buffer with granular output
        if (ch0 != nullptr)
            ch0[i] = out_l;
        if (ch1 != nullptr)
            ch1[i] = out_r;
    }
}

void GranularProcessor::spawn_grain(float spray, float pitch_spread) noexcept {
    for (auto& g : grains_) {
        if (g.active)
            continue;
        const float spray_samples = spray * (static_cast<float>(k_capture_size) * 0.25f);
        const float scatter = (rng_.nextFloat() - 0.5f) * 2.0f * spray_samples;
        // Base position: grain_len_ samples behind current write head
        const float base = static_cast<float>((write_pos_ - grain_len_ + k_capture_size) & (k_capture_size - 1));
        g.read_pos = base + scatter;
        if (g.read_pos < 0.0f)
            g.read_pos += static_cast<float>(k_capture_size);
        if (g.read_pos >= static_cast<float>(k_capture_size))
            g.read_pos -= static_cast<float>(k_capture_size);
        // Pitch shift: rate = 1.0 ± pitch_spread * 0.5
        g.rate = 1.0f + (rng_.nextFloat() - 0.5f) * pitch_spread;
        g.env_pos = 0;
        g.length = grain_len_;
        g.active = true;
        return;
    }
    // All grain slots busy — drop this spawn. Not a bug: pool is pre-allocated;
    // adding retry or dynamic allocation here violates the zero-alloc audio-thread contract.
    // audit-added: SR2 — document intentional skip; prevents future "fix" that adds RT alloc.
}

float GranularProcessor::hann(int pos, int length) noexcept {
    jassert(length > 0); // audit-added: SR1
    static constexpr float k_2pi = 6.28318530718f;
    return 0.5f * (1.0f - std::cos(k_2pi * static_cast<float>(pos) / static_cast<float>(length)));
}

float GranularProcessor::lerp_buf(const float* buf, float pos) noexcept {
    const int i0 = static_cast<int>(pos) & (k_capture_size - 1);
    const int i1 = (i0 + 1) & (k_capture_size - 1);
    const float t = pos - std::floor(pos);
    return buf[i0] * (1.0f - t) + buf[i1] * t;
}
