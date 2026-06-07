#include "audio/sidechain_comp.h"

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

static float peak_amplitude(const juce::AudioBuffer<float>& buf) {
    float peak = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            peak = std::max(peak, std::abs(buf.getSample(ch, i)));
    return peak;
}

TEST_CASE("SC1 - zero input stays zero", "[sidechain]") {
    juce::ScopedJuceInitialiser_GUI init;
    SidechainCompressor comp;
    comp.prepare(44100.0, 512);
    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    comp.process(buf, -12.0f);
    REQUIRE(peak_amplitude(buf) < 1e-6f);
}

TEST_CASE("SC2 - signal below threshold passes through", "[sidechain]") {
    juce::ScopedJuceInitialiser_GUI init;
    SidechainCompressor comp;
    comp.prepare(44100.0, 512);
    juce::AudioBuffer<float> buf(2, 512);
    // 0.1 amplitude ~= -20 dBFS; threshold -12 dBFS (threshold_linear ~= 0.251)
    // 0.1 < 0.251 -> envelope never exceeds threshold -> no compression
    fill_sine(buf, 440.0f, 44100.0, 0.1f);
    comp.process(buf, -12.0f);
    REQUIRE(peak_amplitude(buf) > 0.1f * 0.9f);
}

TEST_CASE("SC3 - signal above threshold is compressed", "[sidechain]") {
    juce::ScopedJuceInitialiser_GUI init;
    SidechainCompressor comp;
    // 2s buffer: IIR envelope (tau=220 samples) needs ~37+ tau to converge with rectified sine.
    // At steady state (A=1.0, threshold=-12dBFS): gain_db=-10.5dB, gain~=0.299 (audit M1).
    comp.prepare(44100.0, 88200);
    juce::AudioBuffer<float> buf(2, 88200);
    fill_sine(buf, 440.0f, 44100.0, 1.0f); // 0 dBFS -- 12 dB above threshold
    comp.process(buf, -12.0f);
    float tail_peak = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 8192; i < 88200; ++i) // skip first 8192 samples (37 tau)
            tail_peak = std::max(tail_peak, std::abs(buf.getSample(ch, i)));
    REQUIRE(tail_peak < 0.4f); // >8 dB reduction; distinguishes 8:1 from <=2:1 ratio
}

TEST_CASE("SC4 - pump effect: loud block charges envelope, moderate signal is compressed", "[sidechain]") {
    juce::ScopedJuceInitialiser_GUI init;
    SidechainCompressor comp;
    comp.prepare(44100.0, 512);
    juce::AudioBuffer<float> buf(2, 512);

    // Block 0: loud burst -- charges envelope above threshold_linear (0.251) to ~0.9
    fill_sine(buf, 440.0f, 44100.0, 1.0f);
    comp.process(buf, -12.0f);

    // Block 1: moderate signal (0.2) -- env_ ~= 0.9 >> 0.251; release_time 200ms >> 11ms (512/44100)
    // Compression still active: output should be < 0.2 * 0.8 = 0.16
    fill_sine(buf, 440.0f, 44100.0, 0.2f);
    comp.process(buf, -12.0f);
    REQUIRE(peak_amplitude(buf) < 0.2f * 0.8f);
}

TEST_CASE("SC5 - threshold controls compression: low threshold compresses, 0 dBFS transparent", "[sidechain]") {
    juce::ScopedJuceInitialiser_GUI init;
    // Contrast test: amplitude=0.5 (-6 dBFS) is ABOVE -12 dBFS threshold but BELOW 0 dBFS.
    // audit SR1: original single-case test was vacuous (IIR env_ <=1.0 for unit-amplitude signal,
    // so threshold_linear=1.0 branch never entered). This contrast version cannot vacuously pass.
    // 2s buffer: give IIR envelope (tau=220 samples) time to converge with rectified sine.
    // At steady state (A=0.5, threshold=-12dBFS): gain~=0.546, out~=0.273 < 0.35 ✓
    // At threshold=0dBFS: threshold_linear=1.0, env~=0.5 < 1.0 -> no compression -> out~=0.5 > 0.45 ✓
    auto measure_tail = [](float threshold_db) -> float {
        SidechainCompressor comp;
        comp.prepare(44100.0, 88200);
        juce::AudioBuffer<float> buf(2, 88200);
        fill_sine(buf, 440.0f, 44100.0, 0.5f); // -6 dBFS: above -12dBFS threshold, below 0dBFS
        comp.process(buf, threshold_db);
        float peak = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 8192; i < 88200; ++i) // skip first 8192 samples (37 tau)
                peak = std::max(peak, std::abs(buf.getSample(ch, i)));
        return peak;
    };
    // threshold=-12 dBFS: 0.5 is 6 dB above -> compression engaged
    // env~=0.5; env_db=-6; excess=6; gain_db=6*(-0.875)=-5.25dB; gain~=0.546; out~=0.273
    REQUIRE(measure_tail(-12.0f) < 0.5f * 0.7f);
    // threshold=0 dBFS: threshold_linear=1.0; 0.5 amplitude -> envelope~=0.5 < 1.0 -> no compression
    REQUIRE(measure_tail(0.0f) > 0.5f * 0.9f);
}

TEST_CASE("SC6 - prepare/reset/prepare cycle does not crash or corrupt state", "[sidechain]") {
    juce::ScopedJuceInitialiser_GUI init;
    SidechainCompressor comp;
    comp.prepare(44100.0, 512);
    comp.reset();
    comp.prepare(48000.0, 256);
    juce::AudioBuffer<float> buf(2, 256);
    fill_sine(buf, 440.0f, 48000.0, 0.1f); // below threshold -> near-transparent
    REQUIRE_NOTHROW(comp.process(buf, -12.0f));
    REQUIRE(peak_amplitude(buf) > 0.05f);
}
