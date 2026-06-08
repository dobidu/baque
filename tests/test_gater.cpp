#include "audio/gater_processor.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_sr = 48000.0;
static constexpr double k_bpm = 120.0;
static constexpr int k_period = 6000; // 1/16 @ 120bpm/48k

static void fill_const(juce::AudioBuffer<float>& buf, float v) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, v);
}

// GT1 (AC-5): gate_depth=0 → bypass exato.
TEST_CASE("GT1 - gate_depth 0 is exact bypass", "[gater]") {
    constexpr int n = 2048;
    juce::AudioBuffer<float> buf(2, n);
    fill_const(buf, 0.5f);
    GaterProcessor proc;
    proc.prepare(k_sr, n);
    proc.process(buf, 0.0f, k_bpm);
    for (int i = 0; i < n; ++i)
        REQUIRE(buf.getSample(0, i) == 0.5f);
}

// GT2 (AC-5): gate_depth=1 → existe região silenciada E região preservando o nível.
TEST_CASE("GT2 - full gate chops on and off", "[gater]") {
    juce::AudioBuffer<float> buf(2, k_period * 2);
    fill_const(buf, 0.5f);
    GaterProcessor proc;
    proc.prepare(k_sr, k_period * 2);
    proc.process(buf, 1.0f, k_bpm);

    bool found_gated = false;
    bool found_open = false;
    for (int i = 0; i < buf.getNumSamples(); ++i) {
        const float v = std::abs(buf.getSample(0, i));
        if (v < 0.01f)
            found_gated = true;
        if (std::abs(v - 0.5f) < 0.01f)
            found_open = true;
    }
    REQUIRE(found_gated);
    REQUIRE(found_open);
}

// GT3 (AC-5): periodicidade — regiões gated recorrem a cada gate_period (1/16).
TEST_CASE("GT3 - gate is beat-synced to the 1/16 period", "[gater]") {
    REQUIRE(GaterProcessor::gate_period_samples(48000.0, 120.0) == 6000);

    juce::AudioBuffer<float> buf(2, k_period * 3);
    fill_const(buf, 0.5f);
    GaterProcessor proc;
    proc.prepare(k_sr, k_period * 3);
    proc.process(buf, 1.0f, k_bpm);

    // Coleta inícios de regiões gated SUSTENTADAS (plateau OFF), ignorando o sliver de ~2 amostras
    // do fade-up de reabertura no phase 0 (gain rampa 0→1) — esse não é uma região gated real.
    auto sustained_gated = [&](int i) {
        return std::abs(buf.getSample(0, i)) < 0.01f && i + 256 < buf.getNumSamples() &&
               std::abs(buf.getSample(0, i + 256)) < 0.01f;
    };
    int first_start = -1, second_start = -1;
    bool prev_gated = false;
    for (int i = 0; i < buf.getNumSamples(); ++i) {
        const bool gated = sustained_gated(i);
        if (gated && !prev_gated) {
            if (first_start < 0)
                first_start = i;
            else if (second_start < 0) {
                second_start = i;
                break;
            }
        }
        prev_gated = gated;
    }
    REQUIRE(first_start >= 0);
    REQUIRE(second_start >= 0);
    REQUIRE(std::abs((second_start - first_start) - k_period) < 128); // ≈ 1 período (1/16)
}

// GT4 (AC-5 anti-click): sem salto brusco entre amostras vizinhas (fade presente).
TEST_CASE("GT4 - gate edges fade, no click", "[gater]") {
    juce::AudioBuffer<float> buf(2, k_period * 2);
    fill_const(buf, 0.5f);
    GaterProcessor proc;
    proc.prepare(k_sr, k_period * 2);
    proc.process(buf, 1.0f, k_bpm);

    float max_step = 0.0f;
    for (int i = 1; i < buf.getNumSamples(); ++i)
        max_step = std::max(max_step, std::abs(buf.getSample(0, i) - buf.getSample(0, i - 1)));
    REQUIRE(max_step < 0.5f); // fade ~2ms → degraus pequenos, sem click de borda dura
}

// GT5 (AC-2 sanity): enum + count.
TEST_CASE("GT5 - PLockParam count 15 with tape_stop 13 and gate_depth 14", "[gater]") {
    REQUIRE(k_plock_param_count == 15);
    REQUIRE(static_cast<int>(PLockParam::tape_stop) == 13);
    REQUIRE(static_cast<int>(PLockParam::gate_depth) == 14);
}

// GT6 (AC-6, SR2): wiring real — processBlock com tape_stop+gater ativos fica finito.
TEST_CASE("GT6 - processBlock with tape_stop and gater active stays finite", "[gater]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 512);

    processor.apvts_.getParameter("tape_stop")->setValueNotifyingHost(1.0f);
    processor.apvts_.getParameter("gate_depth")->setValueNotifyingHost(1.0f);

    for (int b = 0; b < 64; ++b) {
        juce::AudioBuffer<float> buf(2, 512);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                REQUIRE(std::isfinite(buf.getSample(ch, i)));
    }
}
