#include "audio/time_stretch.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using Catch::Approx;

struct TimeStretchJuceFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

static void load_sine_pad(SamplePad& pad, int num_samples, float sample_rate = 44100.0f) {
    auto& buf = pad.sample_buffer();
    buf.setSize(1, num_samples, false, true, false);
    float* data = buf.getWritePointer(0);
    for (int i = 0; i < num_samples; ++i)
        data[i] = std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / sample_rate);
}

// T1 -- tempo_ratio 0.5 roughly doubles output length (AC-1)
TEST_CASE("time_stretch ratio 0.5 roughly doubles length", "[stretch]") {
    TimeStretchJuceFixture f;
    PadBank bank;
    VoicePool pool;
    pool.prepare(44100.0);
    load_sine_pad(bank.pad(0), 8192);
    TimeStretch::apply(bank.pad(0), pool, 44100.0, 0.5);
    const int out = bank.pad(0).num_samples();
    REQUIRE(out > static_cast<int>(8192 * 1.7));
    REQUIRE(out < static_cast<int>(8192 * 2.3));
}

// T2 -- tempo_ratio 2.0 roughly halves output length (AC-2)
TEST_CASE("time_stretch ratio 2.0 roughly halves length", "[stretch]") {
    TimeStretchJuceFixture f;
    PadBank bank;
    VoicePool pool;
    pool.prepare(44100.0);
    load_sine_pad(bank.pad(0), 8192);
    TimeStretch::apply(bank.pad(0), pool, 44100.0, 2.0);
    const int out = bank.pad(0).num_samples();
    REQUIRE(out > static_cast<int>(8192 * 0.35));
    REQUIRE(out < static_cast<int>(8192 * 0.65));
}

// T3 -- pool.reset_all() called on apply (AC-3)
TEST_CASE("time_stretch resets pool voices on apply", "[stretch]") {
    TimeStretchJuceFixture f;
    PadBank bank;
    VoicePool pool;
    pool.prepare(44100.0);
    load_sine_pad(bank.pad(0), 8192);
    (void)pool.trigger_at(0, bank.pad(0).data(), bank.pad(0).num_samples(), 1.0f, 1.0, false, 0.0f, 0);
    TimeStretch::apply(bank.pad(0), pool, 44100.0, 0.5);
    float left[128] = {}, right[128] = {};
    pool.process_all(left, right, 128);
    float sum = 0.0f;
    for (int i = 0; i < 128; ++i)
        sum += std::fabs(left[i]);
    REQUIRE(sum == Approx(0.0f));
}

// T4 -- no-op on empty pad (AC-4a)
TEST_CASE("time_stretch no-op on empty pad", "[stretch]") {
    TimeStretchJuceFixture f;
    PadBank bank;
    VoicePool pool;
    TimeStretch::apply(bank.pad(0), pool, 44100.0, 0.5);
    REQUIRE(!bank.pad(0).has_sample());
}

// T5 -- no-op on ratio 1.0, ratio <= 0, and short input (AC-4b/c/d)
TEST_CASE("time_stretch no-op on identity ratio and invalid inputs", "[stretch]") {
    TimeStretchJuceFixture f;
    PadBank bank;
    VoicePool pool;
    pool.prepare(44100.0);
    load_sine_pad(bank.pad(0), 2048);
    const int orig_len = bank.pad(0).num_samples();

    TimeStretch::apply(bank.pad(0), pool, 44100.0, 1.0);
    REQUIRE(bank.pad(0).num_samples() == orig_len);

    TimeStretch::apply(bank.pad(0), pool, 44100.0, 0.0);
    REQUIRE(bank.pad(0).num_samples() == orig_len);

    TimeStretch::apply(bank.pad(0), pool, 44100.0, -1.0);
    REQUIRE(bank.pad(0).num_samples() == orig_len);
}
