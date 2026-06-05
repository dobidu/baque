#pragma once
#include "pad_bank.h"
#include "voice_pool.h"

// Offline tempo-only time-stretch via SoundTouch (WSOLA).
// NOT RT-safe -- allocates. Call from background/message thread only.
class TimeStretch {
  public:
    // Stretch pad.sample_buffer() in place (tempo unchanged, duration changes).
    // tempo_ratio: 1.0 = no change; 0.5 = half speed (2x longer); 2.0 = double speed (2x shorter).
    // Calls pool.reset_all() first (single-writer contract -- see pad_bank.h).
    // No-op if: !pad.has_sample() || tempo_ratio <= 0 || |tempo_ratio - 1.0| < 0.001 || num_samples < 256.
    static void apply(SamplePad& pad, VoicePool& pool, double sample_rate, double tempo_ratio);
};
