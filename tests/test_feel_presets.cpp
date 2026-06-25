#include "audio/feel_engine.h"
#include "audio/feel_pattern.h"
#include "audio/feel_presets.h"
#include "audio/sequencer.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <array> // audit-added M1: std::array used in FP7; missing = compile error on MSVC
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

struct FPFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

static TransportState fp_transport(double ppq = 0.0, double bpm = 120.0) {
    TransportState t{};
    t.is_playing = true;
    t.ppq_position = ppq;
    t.bpm = bpm;
    return t;
}

static StepPattern fp_all_steps() {
    StepPattern p{};
    for (int s = 0; s < StepPattern::k_num_steps; ++s) {
        p.set_active(0, s, true);
        p.set_note(0, s, 36);
    }
    return p;
}

static std::vector<int> collect_on_positions(const juce::MidiBuffer& buf) {
    std::vector<int> v;
    for (const auto meta : buf)
        if (meta.getMessage().isNoteOn())
            v.push_back(meta.samplePosition);
    return v;
}

static std::vector<int> collect_velocities(const juce::MidiBuffer& buf) {
    std::vector<int> v;
    for (const auto meta : buf)
        if (meta.getMessage().isNoteOn())
            v.push_back(meta.getMessage().getVelocity());
    return v;
}

// FP1: Straight — positions identical to no-feel baseline
TEST_CASE("FP1 - straight preset positions match no-feel baseline", "[feel][presets][dod]") {
    FPFixture f;
    const double sr = 44100.0;
    const int block = 98304;

    // Baseline: no feel params
    Sequencer seq_base;
    seq_base.set_pattern(fp_all_steps());
    juce::MidiBuffer buf_base;
    seq_base.generate(fp_transport(), buf_base, block, sr);
    const auto baseline = collect_on_positions(buf_base);

    // Straight preset
    const auto feel = FeelPresets::straight();
    FeelEngine engine;
    engine.set_seed(feel.seed);
    Sequencer seq;
    seq.set_pattern(fp_all_steps());
    juce::MidiBuffer buf;
    seq.generate(fp_transport(), buf, block, sr, &feel, &engine, 0);
    const auto positions = collect_on_positions(buf);

    REQUIRE(baseline == positions);
}

// FP2: Straight — all velocities 100
TEST_CASE("FP2 - straight preset velocities all 100", "[feel][presets]") {
    FPFixture f;
    const double sr = 44100.0;
    const int block = 98304;

    const auto feel = FeelPresets::straight();
    FeelEngine engine;
    engine.set_seed(feel.seed);
    Sequencer seq;
    seq.set_pattern(fp_all_steps());
    juce::MidiBuffer buf;
    seq.generate(fp_transport(), buf, block, sr, &feel, &engine, 0);

    // audit-added SR1: verify at least one note-on was found — vacuous pass prevention
    bool found = false;
    for (const auto meta : buf) {
        const auto& msg = meta.getMessage();
        if (msg.isNoteOn()) {
            found = true;
            REQUIRE(msg.getVelocity() == 100);
        }
    }
    REQUIRE(found);
}

// FP3: DL Drunk — perceptible timing (Phase 5 DoD part 1)
TEST_CASE("FP3 - dl_drunk perceptible timing deviation (Phase 5 DoD)", "[feel][presets][dod]") {
    const auto feel = FeelPresets::dl_drunk();
    REQUIRE(feel.enabled);
    REQUIRE(feel.humanize_timing_ms >= 20.0f);

    float sum_abs = 0.0f;
    for (int i = 0; i < FeelPattern::k_steps; ++i)
        sum_abs += std::abs(feel.steps[i].timing_ms);
    const float avg_abs = sum_abs / static_cast<float>(FeelPattern::k_steps);

    REQUIRE(avg_abs > 20.0f);
}

// FP4: DL Drunk — seed reproducibility (Phase 5 DoD part 2)
TEST_CASE("FP4 - dl_drunk seed-reproducible (Phase 5 DoD)", "[feel][presets][dod]") {
    FPFixture f;
    const double sr = 44100.0;
    const int block = 98304;
    const auto feel = FeelPresets::dl_drunk();

    auto run = [&]() {
        FeelEngine engine;
        engine.set_seed(feel.seed);
        Sequencer seq;
        seq.set_pattern(fp_all_steps());
        juce::MidiBuffer buf;
        seq.generate(fp_transport(), buf, block, sr, &feel, &engine, 0);
        return collect_on_positions(buf);
    };

    const auto p1 = run();
    const auto p2 = run();
    REQUIRE_FALSE(p1.empty());
    REQUIRE(p1 == p2);

    // audit-added SR2: AC-9 — verify humanize actually ran (not vacuous reproducibility).
    // Build deterministic-only baseline: same preset but humanize disabled.
    auto feel_det = feel;
    feel_det.humanize_timing_ms = 0.0f;
    FeelEngine engine_det;
    engine_det.set_seed(feel_det.seed);
    Sequencer seq_det;
    seq_det.set_pattern(fp_all_steps());
    juce::MidiBuffer buf_det;
    seq_det.generate(fp_transport(), buf_det, block, sr, &feel_det, &engine_det, 0);
    const auto p_det = collect_on_positions(buf_det);
    // With humanize_timing_ms=25ms, at least one position must differ from deterministic
    const bool any_differs = (p1 != p_det);
    REQUIRE(any_differs);
}

// FP5: BRL Broken — perceptible scatter (Phase 5 DoD part 3)
TEST_CASE("FP5 - brl_broken perceptible scatter (Phase 5 DoD)", "[feel][presets][dod]") {
    const auto feel = FeelPresets::brl_broken();
    REQUIRE(feel.enabled);
    REQUIRE(feel.humanize_timing_ms >= 40.0f);

    float min_t = feel.steps[0].timing_ms;
    float max_t = feel.steps[0].timing_ms;
    for (int i = 1; i < FeelPattern::k_steps; ++i) {
        min_t = std::min(min_t, feel.steps[i].timing_ms);
        max_t = std::max(max_t, feel.steps[i].timing_ms);
    }
    REQUIRE((max_t - min_t) > 100.0f);
}

// FP6: BRL Broken — seed reproducibility (Phase 5 DoD part 4)
TEST_CASE("FP6 - brl_broken seed-reproducible (Phase 5 DoD)", "[feel][presets][dod]") {
    FPFixture f;
    const double sr = 44100.0;
    const int block = 98304;
    const auto feel = FeelPresets::brl_broken();

    auto run = [&]() {
        FeelEngine engine;
        engine.set_seed(feel.seed);
        Sequencer seq;
        seq.set_pattern(fp_all_steps());
        juce::MidiBuffer buf;
        seq.generate(fp_transport(), buf, block, sr, &feel, &engine, 0);
        return collect_on_positions(buf);
    };

    const auto p1 = run();
    const auto p2 = run();
    REQUIRE_FALSE(p1.empty());
    REQUIRE(p1 == p2);

    // audit-added SR2: AC-9 — verify humanize actually ran (not vacuous reproducibility).
    auto feel_det = feel;
    feel_det.humanize_timing_ms = 0.0f;
    FeelEngine engine_det;
    engine_det.set_seed(feel_det.seed);
    Sequencer seq_det;
    seq_det.set_pattern(fp_all_steps());
    juce::MidiBuffer buf_det;
    seq_det.generate(fp_transport(), buf_det, block, sr, &feel_det, &engine_det, 0);
    const auto p_det = collect_on_positions(buf_det);
    // With humanize_timing_ms=50ms, at least one position must differ from deterministic
    const bool any_differs = (p1 != p_det);
    REQUIRE(any_differs);
}

// FP7: All 6 presets return enabled FeelPattern with correct structure
TEST_CASE("FP7 - all 6 presets return valid enabled FeelPattern", "[feel][presets]") {
    const std::array<FeelPattern, 6> presets = {
        FeelPresets::straight(),
        FeelPresets::boom_bap(),
        FeelPresets::dl_drunk(),
        FeelPresets::brl_broken(),
        FeelPresets::fly_wonk(),
        FeelPresets::bnb_loose(),
    };
    for (const auto& p : presets) {
        REQUIRE(p.enabled);
        REQUIRE(p.seed != 0);
        REQUIRE(FeelPattern::k_steps == 16);
    }
}

// FP8: BNB Loose < BRL Broken (humanize ordering)
TEST_CASE("FP8 - bnb humanize less than brl humanize", "[feel][presets]") {
    const auto bnb = FeelPresets::bnb_loose();
    const auto brl = FeelPresets::brl_broken();
    REQUIRE(bnb.humanize_timing_ms < brl.humanize_timing_ms);
    REQUIRE(bnb.humanize_vel_pct < brl.humanize_vel_pct);
}
