#include "slicer.h"

#include <algorithm>
#include <cmath>

std::vector<int>
Slicer::detect_onsets(const float* data, int num_samples, float sample_rate, float threshold, float min_slice_ms) {
    std::vector<int> onsets;
    onsets.push_back(0);

    if (data == nullptr || num_samples <= 0)
        return onsets;

    const int hop = 256;
    const int min_spacing = static_cast<int>(min_slice_ms * 0.001f * sample_rate);

    float prev_rms = 0.0f;
    int last_onset_sample = 0;

    for (int frame_start = 0; frame_start < num_samples; frame_start += hop) {
        const int frame_len = std::min(hop, num_samples - frame_start);

        float sum_sq = 0.0f;
        for (int i = 0; i < frame_len; ++i) {
            const float s = data[frame_start + i];
            sum_sq += s * s;
        }
        const float rms = std::sqrt(sum_sq / static_cast<float>(frame_len));

        const float delta = std::max(0.0f, rms - prev_rms);
        prev_rms = rms;

        if (frame_start > 0 && delta > threshold && frame_start - last_onset_sample >= min_spacing) {
            onsets.push_back(frame_start);
            last_onset_sample = frame_start;
            if (static_cast<int>(onsets.size()) >= PadBank::k_num_pads)
                break;
        }
    }

    return onsets;
}

void Slicer::chop_to_pads(
    PadBank& bank, VoicePool& pool, const float* data, int num_samples, const std::vector<int>& onsets) {
    if (data == nullptr || num_samples <= 0) {
        pool.reset_all();
        return;
    }

    pool.reset_all();

    jassert(std::is_sorted(onsets.begin(), onsets.end()));

    const int n = std::min(static_cast<int>(onsets.size()), PadBank::k_num_pads);

    for (int i = n; i < PadBank::k_num_pads; ++i)
        bank.pad(i).sample_buffer().setSize(0, 0);

    for (int i = 0; i < n; ++i) {
        const int start = onsets[i];
        const int end = (i + 1 < static_cast<int>(onsets.size())) ? onsets[i + 1] : num_samples;
        const int length = end - start;

        if (length <= 0)
            continue;

        auto& buf = bank.pad(i).sample_buffer();
        buf.setSize(1, length, false, true, false);
        juce::FloatVectorOperations::copy(buf.getWritePointer(0), data + start, length);
    }
}
