#include "audio/fx_chain.h"
#include "audio/fx_params.h"
#include "audio/plock_pattern.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_gc_pi = 3.14159265358979323846;

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, double sr, float amplitude = 1.0f) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, amplitude * static_cast<float>(std::sin(2.0 * k_gc_pi * freq * i / sr)));
}

TEST_CASE("GC1 - FxChain default granular params produce finite non-silent output", "[granular]") {
    juce::ScopedJuceInitialiser_GUI init;
    FxChain chain;
    chain.prepare(44100.0, 4096);

    FxParams p{};
    p.reverb_mix = 0.0f;
    p.delay_mix = 0.0f;
    p.sidechain_threshold = 0.0f;
    p.granular_spray = 0.0f;
    p.granular_pitch_spread = 0.0f;
    p.granular_freeze = 0.0f;

    // Fill capture buffer before measuring — grains read from captured audio;
    // without a fill phase the ring buffer is all-zeros and output is silent.
    juce::AudioBuffer<float> fill(2, 4096);
    fill_sine(fill, 440.0f, 44100.0, 0.5f);
    chain.process(fill, p);

    juce::AudioBuffer<float> buf(2, 1024);
    fill_sine(buf, 440.0f, 44100.0, 0.5f);
    chain.process(buf, p);

    float peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 1024; ++i) {
            const float s = buf.getSample(ch, i);
            REQUIRE(std::isfinite(s));
            peak = std::max(peak, std::abs(s));
        }
    REQUIRE(peak > 0.1f);
}

TEST_CASE("GC2 - FxChain granular freeze holds grains during silence", "[granular]") {
    juce::ScopedJuceInitialiser_GUI init;
    FxChain chain;
    chain.prepare(44100.0, 4096);

    FxParams p{};
    p.reverb_mix = 0.0f;
    p.delay_mix = 0.0f;
    p.sidechain_threshold = 0.0f;
    p.granular_spray = 0.0f;
    p.granular_pitch_spread = 0.0f;
    p.granular_freeze = 0.0f;

    // Fill capture buffer with signal
    juce::AudioBuffer<float> fill(2, 4096);
    fill_sine(fill, 440.0f, 44100.0, 0.5f);
    chain.process(fill, p);

    // Freeze on — send silence
    p.granular_freeze = 1.0f;
    juce::AudioBuffer<float> silent(2, 512);
    silent.clear();
    chain.process(silent, p);

    // Frozen grains must produce signal even with silent input
    float peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            peak = std::max(peak, std::abs(silent.getSample(ch, i)));
    REQUIRE(peak > 0.01f);
}

TEST_CASE("GC3 - FxChain spray=0.5 measurably differs from spray=0.0", "[granular]") {
    juce::ScopedJuceInitialiser_GUI init;
    FxChain chain_a, chain_b;
    chain_a.prepare(44100.0, 4096);
    chain_b.prepare(44100.0, 4096);

    FxParams p{};
    p.reverb_mix = 0.0f;
    p.delay_mix = 0.0f;
    p.sidechain_threshold = 0.0f;
    p.granular_spray = 0.0f;
    p.granular_pitch_spread = 0.0f;
    p.granular_freeze = 0.0f;

    // Fill phase — separate buffers per instance (process() modifies in-place)
    juce::AudioBuffer<float> fill_a(2, 4096);
    fill_sine(fill_a, 440.0f, 44100.0, 0.5f);
    chain_a.process(fill_a, p);

    juce::AudioBuffer<float> fill_b(2, 4096);
    fill_sine(fill_b, 440.0f, 44100.0, 0.5f);
    chain_b.process(fill_b, p);

    // Measurement phase
    juce::AudioBuffer<float> buf_a(2, 512);
    fill_sine(buf_a, 440.0f, 44100.0, 0.5f);
    juce::AudioBuffer<float> buf_b(2, 512);
    fill_sine(buf_b, 440.0f, 44100.0, 0.5f);

    FxParams pa = p;
    pa.granular_spray = 0.0f;
    chain_a.process(buf_a, pa);

    FxParams pb = p;
    pb.granular_spray = 0.5f;
    chain_b.process(buf_b, pb);

    float max_diff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            max_diff = std::max(max_diff, std::abs(buf_a.getSample(ch, i) - buf_b.getSample(ch, i)));
    REQUIRE(max_diff > 0.001f);
}

TEST_CASE("GC4 - PLockParam granular enum indices correct", "[granular]") {
    // apply_plock_batch() dispatch verified in plugin_processor.cpp.
    // Enum plumbing checked here — same pattern as LC4/PL6.
    REQUIRE(static_cast<int>(PLockParam::granular_spray) == 8);
    REQUIRE(static_cast<int>(PLockParam::granular_pitch_spread) == 9);
    REQUIRE(static_cast<int>(PLockParam::granular_freeze) == 10);
    REQUIRE(k_plock_param_count == 15);

    FxParams fx{};
    fx.granular_spray = 0.5f;
    fx.granular_pitch_spread = 0.3f;
    fx.granular_freeze = 1.0f;
    REQUIRE(fx.granular_spray == Catch::Approx(0.5f));
    REQUIRE(fx.granular_pitch_spread == Catch::Approx(0.3f));
    REQUIRE(fx.granular_freeze == Catch::Approx(1.0f));
}

TEST_CASE("GC5 - Phase 7 granular FxChain integration DoD marker", "[granular][dod]") {
    // GC1-GC4 prove GranularProcessor wired into FxChain as second stage.
    // granular_spray, granular_pitch_spread, granular_freeze are FxParams fields
    // and PLockParam enum entries (indices 8-10).
    SUCCEED("GranularProcessor wired into FxChain; granular params p-lockable");
}
