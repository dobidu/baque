#include "feel_presets.h"

FeelPattern FeelPresets::straight() noexcept {
    FeelPattern p{};
    p.enabled = true;
    p.humanize_timing_ms = 0.0f;
    p.humanize_vel_pct = 0.0f;
    p.seed = 1;
    // All steps: timing=0, vel=1.0 (grid-locked)
    return p;
}

FeelPattern FeelPresets::boom_bap() noexcept {
    FeelPattern p{};
    p.enabled = true;
    p.humanize_timing_ms = 5.0f;
    p.humanize_vel_pct = 5.0f;
    p.seed = 808;

    // Classic hip-hop: kicks/snares on grid or slightly behind; off-beats ahead
    static constexpr float timing[16] = {0, -5, -8, 0, 15, -5, -8, 0, 0, -5, -8, 0, 15, -5, -8, 0};
    static constexpr float vel[16] = {
        1.0f, 0.7f, 0.8f, 0.7f, 1.0f, 0.8f, 0.7f, 0.7f, 0.9f, 0.7f, 0.8f, 0.7f, 1.0f, 0.8f, 0.7f, 0.7f};
    for (int i = 0; i < FeelPattern::k_steps; ++i) {
        p.steps[i].timing_ms = timing[i];
        p.steps[i].vel_scale = vel[i];
    }
    return p;
}

FeelPattern FeelPresets::dl_drunk() noexcept {
    FeelPattern p{};
    p.enabled = true;
    p.humanize_timing_ms = 25.0f;
    p.humanize_vel_pct = 15.0f;
    p.seed = 313;

    // Notes consistently behind grid, 16ths very drunk
    static constexpr float timing[16] = {0, 20, 35, 15, 25, 40, 20, 10, 0, 30, 45, 20, 30, 50, 25, 15};
    static constexpr float vel[16] = {
        1.0f, 0.9f, 0.75f, 0.85f, 0.95f, 0.8f, 0.75f, 0.9f, 1.0f, 0.85f, 0.75f, 0.85f, 0.95f, 0.8f, 0.75f, 0.9f};
    for (int i = 0; i < FeelPattern::k_steps; ++i) {
        p.steps[i].timing_ms = timing[i];
        p.steps[i].vel_scale = vel[i];
    }
    return p;
}

FeelPattern FeelPresets::brl_broken() noexcept {
    FeelPattern p{};
    p.enabled = true;
    p.humanize_timing_ms = 50.0f;
    p.humanize_vel_pct = 20.0f;
    p.seed = 666;

    // Extreme irregular scatter, near-broken
    static constexpr float timing[16] = {0, -30, 75, -15, 50, 100, -45, 20, 0, 60, -20, 80, 35, -60, 90, -10};
    static constexpr float vel[16] = {
        1.0f, 0.6f, 0.85f, 0.7f, 0.9f, 0.65f, 0.75f, 0.8f, 1.0f, 0.7f, 0.85f, 0.6f, 0.9f, 0.75f, 0.65f, 0.8f};
    for (int i = 0; i < FeelPattern::k_steps; ++i) {
        p.steps[i].timing_ms = timing[i];
        p.steps[i].vel_scale = vel[i];
    }
    return p;
}

FeelPattern FeelPresets::fly_wonk() noexcept {
    FeelPattern p{};
    p.enabled = true;
    p.humanize_timing_ms = 15.0f;
    p.humanize_vel_pct = 10.0f;
    p.seed = 42;

    // Polyrhythmic drift, creative irregularity
    static constexpr float timing[16] = {0, 10, -20, 30, -10, 25, -15, 5, 0, 15, -25, 20, -5, 35, -10, 10};
    static constexpr float vel[16] = {
        1.0f, 0.85f, 0.9f, 0.75f, 0.95f, 0.8f, 0.85f, 0.9f, 0.95f, 0.8f, 0.85f, 0.75f, 0.9f, 0.8f, 0.85f, 0.9f};
    for (int i = 0; i < FeelPattern::k_steps; ++i) {
        p.steps[i].timing_ms = timing[i];
        p.steps[i].vel_scale = vel[i];
    }
    return p;
}

FeelPattern FeelPresets::bnb_loose() noexcept {
    FeelPattern p{};
    p.enabled = true;
    p.humanize_timing_ms = 8.0f;
    p.humanize_vel_pct = 8.0f;
    p.seed = 2024;

    // Live drummer feel, light humanize, subtle off-grid
    static constexpr float timing[16] = {0, 5, -5, 5, 10, 5, -5, 5, 0, 5, -5, 5, 12, 5, -5, 5};
    static constexpr float vel[16] = {
        1.0f, 0.85f, 0.9f, 0.8f, 0.9f, 0.85f, 0.8f, 0.85f, 0.95f, 0.8f, 0.9f, 0.8f, 0.9f, 0.85f, 0.8f, 0.85f};
    for (int i = 0; i < FeelPattern::k_steps; ++i) {
        p.steps[i].timing_ms = timing[i];
        p.steps[i].vel_scale = vel[i];
    }
    return p;
}
