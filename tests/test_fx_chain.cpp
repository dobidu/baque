#include "audio/fx_chain.h"
#include "audio/fx_params.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_pi = 3.14159265358979323846;

static FxParams default_params() {
    return FxParams{};
}

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, double sr) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, static_cast<float>(std::sin(2.0 * k_pi * freq * i / sr)));
}

static float peak_amplitude(const juce::AudioBuffer<float>& buf) {
    float peak = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            peak = std::max(peak, std::abs(buf.getSample(ch, i)));
    return peak;
}

TEST_CASE("FC1 - zero input with default params stays zero", "[fx_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    FxChain fx;
    fx.prepare(44100.0, 512);
    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    fx.process(buf, default_params());
    REQUIRE(peak_amplitude(buf) < 1e-6f);
}

TEST_CASE("FC2 - 1kHz tone near-transparent with default LP at 20kHz", "[fx_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    FxChain fx;
    fx.prepare(44100.0, 512);
    juce::AudioBuffer<float> buf(2, 512);
    fill_sine(buf, 1000.0f, 44100.0);
    const float pre_peak = peak_amplitude(buf);
    fx.process(buf, default_params());
    REQUIRE(peak_amplitude(buf) > pre_peak * 0.99f);
}

TEST_CASE("FC3 - LP at 200Hz attenuates 10kHz by more than 20dB", "[fx_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    FxChain fx;
    fx.prepare(44100.0, 4096);
    FxParams p{};
    p.filter_cutoff = 200.0f;
    p.filter_res = 0.1f;
    juce::AudioBuffer<float> buf(2, 4096);
    fill_sine(buf, 10000.0f, 44100.0);
    const float pre_peak = peak_amplitude(buf);
    fx.process(buf, p);
    float tail_peak = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 512; i < 4096; ++i)
            tail_peak = std::max(tail_peak, std::abs(buf.getSample(ch, i)));
    REQUIRE(tail_peak < pre_peak * 0.1f);
}

TEST_CASE("FC4 - reverb mix 0 vs 0.5 after impulse: tail energy check", "[fx_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    // Multi-block approach: block0=impulse, block1=silence (filter decays), block2=silence
    // (comb echo arrives at global sample 1116 = block2 position 92).
    // With mix=0 (dry only, wetLevel=0): block2 output stays silent.
    // With mix=0.5: reverb comb echo visible in block2.
    auto measure_reverb_tail = [](float rev_mix) -> float {
        FxChain fx;
        fx.prepare(44100.0, 512);
        FxParams p{};
        p.reverb_mix = rev_mix;
        juce::AudioBuffer<float> buf(2, 512);
        // Block 0: impulse — primes reverb comb filters (audit M1 fix: clear ONCE outside loop)
        buf.clear();
        for (int ch = 0; ch < 2; ++ch)
            buf.setSample(ch, 0, 1.0f);
        fx.process(buf, p);
        // Block 1: silence — filter ringing decays (<20 samples at Q≈1.26, 20kHz)
        buf.clear();
        fx.process(buf, p);
        // Block 2: silence — first reverb comb echo at position ~92 (global sample 1116)
        buf.clear();
        fx.process(buf, p);
        float peak = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                peak = std::max(peak, std::abs(buf.getSample(ch, i)));
        return peak;
    };
    REQUIRE(measure_reverb_tail(0.0f) < 1e-4f);
    REQUIRE(measure_reverb_tail(0.5f) > 1e-3f);
}

TEST_CASE("FC5 - delay mix 0 is dry-only; positive mix replays signal", "[fx_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    {
        // delay_mix=0: delay contributes nothing. After sine block, silence block stays silent.
        // (Filter ringing decays in <20 samples; reverb mix=0; delay mix=0 → wet muted.)
        FxChain fx;
        fx.prepare(44100.0, 512);
        FxParams p{};
        p.delay_mix = 0.0f;
        p.delay_time = 0.01f; // 441 samples — within block
        juce::AudioBuffer<float> buf(2, 512);
        fill_sine(buf, 440.0f, 44100.0);
        fx.process(buf, p); // block 0: fills delay buffer, but mix=0 → no echo in output
        buf.clear();
        fx.process(buf, p); // block 1: silence — no delay echo (mix=0 mutes wet signal)
        float tail = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 200; i < 512; ++i) // skip filter settling (SVTPT discrete τ≈14 samples)
                tail = std::max(tail, std::abs(buf.getSample(ch, i)));
        REQUIRE(tail < 1e-5f);
    }
    {
        FxChain fx;
        fx.prepare(44100.0, 512);
        FxParams p{};
        p.delay_mix = 0.5f;
        p.delay_time = 0.001f;
        juce::AudioBuffer<float> buf(2, 512);
        fill_sine(buf, 440.0f, 44100.0);
        fx.process(buf, p);
        fill_sine(buf, 440.0f, 44100.0);
        const float before = buf.getSample(0, 100);
        fx.process(buf, p);
        const float after = buf.getSample(0, 100);
        REQUIRE(std::abs(after - before) > 1e-4f);
    }
}

TEST_CASE("FC6 - prepare/reset/prepare cycle does not crash or corrupt state", "[fx_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    FxChain fx;
    fx.prepare(44100.0, 512);
    fx.reset();
    fx.prepare(48000.0, 256);
    juce::AudioBuffer<float> buf(2, 256);
    fill_sine(buf, 1000.0f, 48000.0);
    REQUIRE_NOTHROW(fx.process(buf, default_params()));
    REQUIRE(peak_amplitude(buf) > 0.5f);
}
