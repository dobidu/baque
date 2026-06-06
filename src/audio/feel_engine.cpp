#include "feel_engine.h"

#include <cmath>

static uint32_t xorshift32(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static float gaussian_sample(uint32_t& state) noexcept {
    constexpr float k_two_pi = 6.28318530f;
    constexpr float k_inv = 1.0f / 4294967296.0f;
    const float u1 = static_cast<float>(xorshift32(state)) * k_inv + 1e-7f;
    const float u2 = static_cast<float>(xorshift32(state)) * k_inv;
    return std::sqrt(-2.0f * std::log(u1)) * std::cos(k_two_pi * u2);
}

void FeelEngine::prepare() noexcept {
    deferred_count_ = 0;
    prng_state_ = seed_;
}

void FeelEngine::set_seed(uint32_t seed) noexcept {
    seed_ = (seed == 0) ? 1u : seed;
    prng_state_ = seed_;
}

int FeelEngine::next_timing_jitter_samples(float stddev_ms, double sample_rate) noexcept {
    const float jitter_ms = stddev_ms * gaussian_sample(prng_state_);
    return timing_ms_to_samples(jitter_ms, sample_rate);
}

uint8_t FeelEngine::apply_vel_jitter(uint8_t base_vel, float stddev_pct) noexcept {
    const float jitter = (stddev_pct / 100.0f) * static_cast<float>(base_vel) * gaussian_sample(prng_state_);
    const float result = static_cast<float>(base_vel) + jitter;
    return static_cast<uint8_t>(std::clamp(std::round(result), 1.0f, 127.0f));
}

int FeelEngine::timing_ms_to_samples(float timing_ms, double sample_rate) noexcept {
    return static_cast<int>(std::round(static_cast<double>(timing_ms) * sample_rate / 1000.0));
}

void FeelEngine::flush_deferred(juce::MidiBuffer& buf, int64_t block_start, int block_size) noexcept {
    const int64_t block_end = block_start + static_cast<int64_t>(block_size);
    int write = 0;
    for (int i = 0; i < deferred_count_; ++i) {
        const auto& d = deferred_[i];
        if (d.target_sample >= block_start && d.target_sample < block_end) {
            buf.addEvent(d.msg, static_cast<int>(d.target_sample - block_start));
        } else if (d.target_sample >= block_end) {
            deferred_[write++] = deferred_[i]; // future — keep
        } else {
            jassertfalse; // stale past note — discard; queue leak prevention (M2)
        }
    }
    deferred_count_ = write;
}

void FeelEngine::defer(const juce::MidiMessage& msg, int64_t target_sample) noexcept {
    jassert(msg.getRawDataSize() <=
            3); // RT-safety: only inline msgs (note-on/off ≤3 bytes); larger msgs allocate on copy (M4)
    jassert(deferred_count_ < k_max_deferred);
    if (deferred_count_ >= k_max_deferred)
        return; // RT-safe drop
    deferred_[deferred_count_++] = {msg, target_sample};
}
