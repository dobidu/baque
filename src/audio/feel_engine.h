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

    // Set PRNG seed. Resets live prng_state_ and stores for prepare() reset.
    // seed=0 treated as seed=1 (xorshift32 invariant: state must never be 0).
    void set_seed(uint32_t seed) noexcept;

    // Gaussian timing jitter in samples. Advances PRNG twice (Box-Muller). Note-on only.
    [[nodiscard]] int next_timing_jitter_samples(float stddev_ms, double sample_rate) noexcept;

    // Apply Gaussian velocity jitter. Clamps result to [1, 127]. Note-on only.
    [[nodiscard]] uint8_t apply_vel_jitter(uint8_t base_vel, float stddev_pct) noexcept;

  private:
    DeferredNote deferred_[k_max_deferred]{};
    int deferred_count_ = 0;
    uint32_t seed_ = 1;       // stored seed; restored by prepare()
    uint32_t prng_state_ = 1; // live xorshift32 state (must never be 0)
};
