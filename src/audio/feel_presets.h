#pragma once
#include "feel_pattern.h"

// Named feel presets. Each returns a fully populated FeelPattern (enabled=true).
// Preset data encodes per-step timing_ms, vel_scale, humanize params, and seed.
// Call feel_engine.set_seed(preset.seed) before generate() for reproducible grooves.
struct FeelPresets {
    [[nodiscard]] static FeelPattern straight() noexcept;
    [[nodiscard]] static FeelPattern boom_bap() noexcept;
    [[nodiscard]] static FeelPattern dilla_drunk() noexcept;
    [[nodiscard]] static FeelPattern burial_broken() noexcept;
    [[nodiscard]] static FeelPattern flylo_wonk() noexcept;
    [[nodiscard]] static FeelPattern bonobo_loose() noexcept;
};
