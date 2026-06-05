#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdint>

// Manages deferred MIDI notes (offsets that push past block boundary) and per-step timing math.
// NOT RT-allocated: deferred_ is a fixed array. All methods noexcept.
class FeelEngine {
  public:
    static constexpr int k_max_deferred = 128;

    struct DeferredNote {
        juce::MidiMessage msg{};
        int64_t target_sample = 0;
    };

    // Clear deferred queue. Call from prepareToPlay() and on transport start.
    void prepare() noexcept;

    // Convert timing_ms to sample count delta. Pure, no state.
    [[nodiscard]] static int timing_ms_to_samples(float timing_ms, double sample_rate) noexcept;

    // Flush deferred notes whose target falls within [block_start, block_start + block_size).
    // Adds matching notes to buf. Future notes stay queued. Past notes (stale) discarded + jassertfalse.
    void flush_deferred(juce::MidiBuffer& buf, int64_t block_start, int block_size) noexcept;

    // Add a note to the deferred queue. Drops + jassertfalse if queue full or msg > 3 bytes.
    void defer(const juce::MidiMessage& msg, int64_t target_sample) noexcept;

    [[nodiscard]] int deferred_count() const noexcept { return deferred_count_; }

  private:
    DeferredNote deferred_[k_max_deferred]{};
    int deferred_count_ = 0;
};
