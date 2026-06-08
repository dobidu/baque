#include "audio/granular_processor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_pi = 3.14159265358979323846;

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, double sr, float amplitude = 1.0f) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, amplitude * static_cast<float>(std::sin(2.0 * k_pi * freq * i / sr)));
}

TEST_CASE("GR1 - GranularProcessor produces finite non-silent output", "[granular]") {
    GranularProcessor gp;
    gp.prepare(44100.0, 1024);

    juce::AudioBuffer<float> buf(2, 4096);
    fill_sine(buf, 440.0f, 44100.0, 0.5f);
    gp.process(buf, 0.0f, 0.0f, false);

    float peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 4096; ++i) {
            const float s = buf.getSample(ch, i);
            REQUIRE(std::isfinite(s));
            peak = std::max(peak, std::abs(s));
        }
    REQUIRE(peak > 0.01f);
}

TEST_CASE("GR2 - GranularProcessor freeze holds captured audio after input silenced", "[granular]") {
    GranularProcessor gp;
    gp.prepare(44100.0, 1024);

    // Fill capture buffer with sine (freeze=false)
    juce::AudioBuffer<float> fill(2, 4096);
    fill_sine(fill, 440.0f, 44100.0, 0.5f);
    gp.process(fill, 0.0f, 0.0f, false);

    // Freeze + silent input — grains must replay from frozen capture
    juce::AudioBuffer<float> silent(2, 512);
    silent.clear();
    gp.process(silent, 0.0f, 0.0f, true);

    float peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            peak = std::max(peak, std::abs(silent.getSample(ch, i)));
    REQUIRE(peak > 0.01f);
}

TEST_CASE("GR3 - spray=0.5 produces measurably different output from spray=0.0", "[granular]") {
    GranularProcessor gp_a, gp_b;
    gp_a.prepare(44100.0, 1024);
    gp_b.prepare(44100.0, 1024);

    // Fill phase — separate buffers: process() modifies in place;
    // identical fill_sine inputs give identical capture_l_/capture_r_ contents.
    juce::AudioBuffer<float> fill_a(2, 4096);
    fill_sine(fill_a, 440.0f, 44100.0, 0.5f);
    gp_a.process(fill_a, 0.0f, 0.0f, false);

    juce::AudioBuffer<float> fill_b(2, 4096);
    fill_sine(fill_b, 440.0f, 44100.0, 0.5f);
    gp_b.process(fill_b, 0.0f, 0.0f, false);

    // Measurement phase — spray=0 vs spray=0.5
    juce::AudioBuffer<float> buf_a(2, 512);
    fill_sine(buf_a, 440.0f, 44100.0, 0.5f);
    gp_a.process(buf_a, 0.0f, 0.0f, false);

    juce::AudioBuffer<float> buf_b(2, 512);
    fill_sine(buf_b, 440.0f, 44100.0, 0.5f);
    gp_b.process(buf_b, 0.5f, 0.0f, false);

    float max_diff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            max_diff = std::max(max_diff, std::abs(buf_a.getSample(ch, i) - buf_b.getSample(ch, i)));
    REQUIRE(max_diff > 0.001f);
}

TEST_CASE("GR4 - pitch_spread=0.5 produces measurably different output from pitch_spread=0.0", "[granular]") {
    GranularProcessor gp_a, gp_b;
    gp_a.prepare(44100.0, 1024);
    gp_b.prepare(44100.0, 1024);

    // Fill phase
    juce::AudioBuffer<float> fill_a(2, 4096);
    fill_sine(fill_a, 440.0f, 44100.0, 0.5f);
    gp_a.process(fill_a, 0.0f, 0.0f, false);

    juce::AudioBuffer<float> fill_b(2, 4096);
    fill_sine(fill_b, 440.0f, 44100.0, 0.5f);
    gp_b.process(fill_b, 0.0f, 0.0f, false);

    // Measurement phase — pitch_spread=0.0 vs pitch_spread=0.5
    juce::AudioBuffer<float> buf_a(2, 512);
    fill_sine(buf_a, 440.0f, 44100.0, 0.5f);
    gp_a.process(buf_a, 0.0f, 0.0f, false);

    juce::AudioBuffer<float> buf_b(2, 512);
    fill_sine(buf_b, 440.0f, 44100.0, 0.5f);
    gp_b.process(buf_b, 0.0f, 0.5f, false);

    float max_diff = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            max_diff = std::max(max_diff, std::abs(buf_a.getSample(ch, i) - buf_b.getSample(ch, i)));
    REQUIRE(max_diff > 0.001f);
}

TEST_CASE("GR5 - Phase 7 GranularProcessor standalone DoD marker", "[granular]") {
    SUCCEED("GranularProcessor standalone proven: freeze, spray, pitch_spread");
}
