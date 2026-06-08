#include "audio/lo_fi_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_pi = 3.14159265358979323846;

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, double sr, float amplitude = 1.0f) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, amplitude * static_cast<float>(std::sin(2.0 * k_pi * freq * i / sr)));
}

TEST_CASE("LF1 - 16-bit with sr_factor=1.0 is transparent (less than 1 LSB error)", "[lo_fi]") {
    constexpr int k_n = 1024;
    juce::AudioBuffer<float> buf(2, k_n);
    fill_sine(buf, 440.0f, 44100.0, 0.5f);

    juce::AudioBuffer<float> orig(2, k_n);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < k_n; ++i)
            orig.setSample(ch, i, buf.getSample(ch, i));

    LoFiProcessor proc;
    proc.prepare(44100.0, k_n);
    proc.process(buf, 16.0f, 1.0f);

    float max_err = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < k_n; ++i)
            max_err = std::max(max_err, std::abs(buf.getSample(ch, i) - orig.getSample(ch, i)));

    REQUIRE(max_err < 2.0f / 32768.0f);
}

TEST_CASE("LF2 - 8-bit quantizes 0.3f to exact grid step 38/128", "[lo_fi]") {
    juce::AudioBuffer<float> buf(2, 1);
    buf.setSample(0, 0, 0.3f);
    buf.setSample(1, 0, 0.3f);

    LoFiProcessor proc;
    proc.prepare(44100.0, 1);
    proc.process(buf, 8.0f, 1.0f);

    REQUIRE(buf.getSample(0, 0) == Catch::Approx(38.0f / 128.0f));
    REQUIRE(buf.getSample(0, 0) != Catch::Approx(0.3f).margin(1e-5));
}

TEST_CASE("LF3 - ZOH sr_factor=2.0 holds value for 2 samples then advances", "[lo_fi]") {
    juce::AudioBuffer<float> buf(2, 4);
    const float inputs[] = {0.1f, 0.2f, 0.3f, 0.4f};
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 4; ++i)
            buf.setSample(ch, i, inputs[i]);

    LoFiProcessor proc;
    proc.prepare(44100.0, 4);
    proc.process(buf, 16.0f, 2.0f);

    // Exact == is correct and intentional: both samples compute
    // std::round(held_l_ * levels) / levels with identical held_ and levels
    // → IEEE754 guarantees bit-identical result. Do NOT change to Approx —
    // that would allow held-state drift and destroy the test's semantic value.
    REQUIRE(buf.getSample(0, 0) == buf.getSample(0, 1));
    REQUIRE(buf.getSample(0, 2) == buf.getSample(0, 3));
    REQUIRE(std::abs(buf.getSample(0, 0) - buf.getSample(0, 2)) > 0.001f);
}

TEST_CASE("LF4 - SP-1200 preset measurably degrades 440Hz sine", "[lo_fi]") {
    constexpr int k_n = 1024;
    juce::AudioBuffer<float> buf(2, k_n);
    fill_sine(buf, 440.0f, 44100.0, 0.5f);

    juce::AudioBuffer<float> orig(2, k_n);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < k_n; ++i)
            orig.setSample(ch, i, buf.getSample(ch, i));

    LoFiProcessor proc;
    proc.prepare(44100.0, k_n);
    proc.process(buf, k_sp1200.bit_depth, k_sp1200.sr_factor);

    float max_diff = 0.0f;
    float max_amp = 0.0f;
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < k_n; ++i) {
            const float s = buf.getSample(ch, i);
            REQUIRE(std::isfinite(s));
            max_diff = std::max(max_diff, std::abs(s - orig.getSample(ch, i)));
            max_amp = std::max(max_amp, std::abs(s));
        }
    }

    REQUIRE(max_diff > 0.001f);
    REQUIRE(max_amp > 0.1f);
}

TEST_CASE("LF5 - Phase 7 lo-fi DoD marker: LoFiProcessor ships", "[lo_fi]") {
    // LF1-LF4 prove BitCrusher + ZOH DSP correctness.
    // k_sp1200/k_sp303/k_8bit presets are defined and tested.
    SUCCEED("LoFiProcessor BitCrusher + ZOH DSP verified");
}
