#pragma once
#include <cstdint>

struct FeelStep {
    float timing_ms = 0.0f; // per-step offset [≈ −200, +200] ms; negative = early, positive = late
    float vel_scale = 1.0f; // velocity multiplier; clamped to [1, 127] at application site
};

struct FeelPattern {
    static constexpr int k_steps = 16;
    FeelStep steps[k_steps];
    bool enabled = false;
    float humanize_timing_ms = 0.0f; // Gaussian stddev for per-note timing jitter (0 = disabled)
    float humanize_vel_pct = 0.0f;   // Gaussian stddev as % of base velocity (0 = disabled)
    uint32_t seed = 1;               // PRNG seed; 0 treated as 1 (xorshift32 cannot have state=0)
};
