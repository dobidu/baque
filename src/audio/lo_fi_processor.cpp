#include "lo_fi_processor.h"

#include <algorithm>
#include <cmath>

void LoFiProcessor::prepare(double /*sample_rate*/, int /*max_block_size*/) noexcept {
    phase_ = 1.0f;
    held_l_ = 0.0f;
    held_r_ = 0.0f;
}

void LoFiProcessor::process(juce::AudioBuffer<float>& buffer, float bit_depth, float sr_factor) noexcept {
    const int num_ch = std::min(buffer.getNumChannels(), 2);
    const int num_samples = buffer.getNumSamples();
    // Hoist out of the per-sample loop — transcendental, not O(1)
    const float levels = std::pow(2.0f, bit_depth - 1.0f);

    float* ch0 = num_ch > 0 ? buffer.getWritePointer(0) : nullptr;
    float* ch1 = num_ch > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < num_samples; ++i) {
        phase_ += 1.0f;
        if (phase_ >= sr_factor) {
            if (ch0 != nullptr)
                held_l_ = ch0[i];
            if (ch1 != nullptr)
                held_r_ = ch1[i];
            phase_ -= sr_factor;
        }
        if (ch0 != nullptr)
            ch0[i] = std::round(held_l_ * levels) / levels;
        if (ch1 != nullptr)
            ch1[i] = std::round(held_r_ * levels) / levels;
    }
}
