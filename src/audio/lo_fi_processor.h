#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

struct LoFiPreset {
    float bit_depth;
    float sr_factor;
};

// SP-1200: 12-bit ADC, 26.04kHz native sample rate
static constexpr LoFiPreset k_sp1200 = {12.0f, 44100.0f / 26040.0f}; // factor ~1.694
// SP-303: 12-bit ADC, native sample rate (no SR reduction)
static constexpr LoFiPreset k_sp303 = {12.0f, 1.0f};
// 8-bit: 8-bit ADC at 22050Hz
static constexpr LoFiPreset k_8bit = {8.0f, 44100.0f / 22050.0f}; // factor 2.0

// RT-safe lo-fi DSP: ZOH sample-rate reduction, then bit-depth quantization.
// Processing order matches hardware ADC: sample at reduced rate, then quantize.
// Handles up to 2 channels (L+R). Mono: only held_l_ active. No allocation in process().
class LoFiProcessor {
  public:
    // Resets phase and held state. Call before first use or on stream restart.
    void prepare(double sample_rate, int max_block_size) noexcept;

    // In-place lo-fi processing. bit_depth in bits (e.g. 8.0, 12.0, 16.0).
    // sr_factor >= 1.0 (e.g. 2.0 = half sample rate). RT-safe.
    void process(juce::AudioBuffer<float>& buffer, float bit_depth, float sr_factor) noexcept;

  private:
    float phase_ = 1.0f;  // ZOH phase accumulator; starts at 1.0f so first sample advances
    float held_l_ = 0.0f; // held sample — left / mono channel
    float held_r_ = 0.0f; // held sample — right channel
};
