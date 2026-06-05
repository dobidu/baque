#include "feel_engine.h"

#include <cmath>

void FeelEngine::prepare() noexcept {
    deferred_count_ = 0;
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
