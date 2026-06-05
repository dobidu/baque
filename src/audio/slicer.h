#pragma once

#include "pad_bank.h"
#include "voice_pool.h"

#include <vector>

// Offline auto-slicer. NOT RT-safe -- allocates. Call off audio thread only.
class Slicer {
  public:
    // Energy-based onset detection. Always includes position 0.
    // Returns up to PadBank::k_num_pads onset sample positions.
    // threshold: HWR delta 0..1 (default 0.1). min_slice_ms: min gap between onsets.
    [[nodiscard]] static std::vector<int> detect_onsets(
        const float* data, int num_samples, float sample_rate, float threshold = 0.1f, float min_slice_ms = 100.0f);

    // Chops buffer into slices at onset positions and loads into consecutive pads.
    // Calls pool.reset_all() first (UAF prevention -- single-writer contract).
    // Clears pads beyond n_slices to prevent stale data from prior chop calls.
    // onsets must be sorted ascending -- jassert enforced in debug builds.
    static void
    chop_to_pads(PadBank& bank, VoicePool& pool, const float* data, int num_samples, const std::vector<int>& onsets);
};
