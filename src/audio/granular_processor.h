#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

// Pre-allocated grain pool granular processor. No allocation in process().
// Stereo: capture_l_/capture_r_ share same grain read positions.
class GranularProcessor {
  public:
    void prepare(double sample_rate, int max_block_size) noexcept;
    void process(juce::AudioBuffer<float>& buffer, float spray, float pitch_spread, bool freeze) noexcept;
    void reset() noexcept;

  private:
    static constexpr int k_capture_size = 8192; // ~185ms at 44100Hz; power-of-2 for wrap mask
    static constexpr int k_num_grains = 16;

    struct Grain {
        bool active = false;
        float read_pos = 0.0f; // fractional position in capture_l_/capture_r_
        float rate = 1.0f;     // playback rate (pitch shift via fractional advance)
        int env_pos = 0;       // Hann envelope position [0, length)
        int length = 0;        // grain length in samples
    };

    float capture_l_[k_capture_size] = {};
    float capture_r_[k_capture_size] = {};
    int write_pos_ = 0;
    bool frozen_ = false;
    Grain grains_[k_num_grains] = {};
    int grain_timer_ = 0;       // samples until next grain spawn
    int grain_len_ = 2205;      // 50ms at 44100 (recomputed in prepare)
    int grain_interval_ = 1102; // 25ms density (recomputed in prepare)
    float sample_rate_ = 44100.0f;
    juce::Random rng_;

    void spawn_grain(float spray, float pitch_spread) noexcept;
    static float hann(int pos, int length) noexcept;
    static float lerp_buf(const float* buf, float pos) noexcept; // wraps at k_capture_size
};
