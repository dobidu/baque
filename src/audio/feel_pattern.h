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
    // Phase 05-02 will add: float humanize_timing_ms, humanize_vel_pct, uint32_t seed
};
