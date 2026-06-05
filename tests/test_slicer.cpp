// Fase 4 (04-03): Slicer -- deteccao de onsets por energia + chop-to-pads.
// Auditoria 04-03: null guard, stale-pad clear, min_slice_ms enforcement.

#include "audio/slicer.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

using Catch::Approx;

// Fixture do JUCE
struct SlicerJuceFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

// Gera buffer com burst de amplitude constante em [burst_start, burst_end)
static std::vector<float> make_burst_buffer(int total, int burst_start, int burst_end, float value = 0.5f) {
    std::vector<float> buf(total, 0.0f);
    for (int i = burst_start; i < burst_end && i < total; ++i)
        buf[i] = value;
    return buf;
}

// S1 -- detect_onsets always returns at least position 0 (silence)
TEST_CASE("slicer detect_onsets silence returns {0}", "[slicer]") {
    SlicerJuceFixture f;
    std::vector<float> buf(1000, 0.0f);
    auto onsets = Slicer::detect_onsets(buf.data(), 1000, 44100.0f, 0.1f, 1.0f);
    REQUIRE(onsets.size() == 1);
    REQUIRE(onsets[0] == 0);
}

// S2 -- detect_onsets finds second burst onset
TEST_CASE("slicer detect_onsets two bursts returns two positions", "[slicer]") {
    SlicerJuceFixture f;
    // burst 0..511, silence 512..1023, burst 1024..1535
    auto buf = make_burst_buffer(1536, 0, 512, 0.5f);
    for (int i = 1024; i < 1536; ++i)
        buf[i] = 0.5f;
    auto onsets = Slicer::detect_onsets(buf.data(), 1536, 44100.0f, 0.05f, 1.0f);
    REQUIRE(onsets.size() >= 2);
    REQUIRE(onsets[0] == 0);
    // onset[1] should be near sample 1024 (within one hop = 256)
    REQUIRE(onsets[1] >= 768);
    REQUIRE(onsets[1] <= 1280);
}

// S3 -- detect_onsets null/zero guard
TEST_CASE("slicer detect_onsets null data returns {0}", "[slicer]") {
    SlicerJuceFixture f;
    auto onsets = Slicer::detect_onsets(nullptr, 0, 44100.0f);
    REQUIRE(onsets.size() == 1);
    REQUIRE(onsets[0] == 0);
}

// S4 -- detect_onsets clamps to 16 pads
TEST_CASE("slicer detect_onsets clamps to 16 pads", "[slicer]") {
    SlicerJuceFixture f;
    // 20 bursts separated by silence, each 256 samples, gap 256
    const int burst = 256, gap = 256, n_bursts = 20;
    std::vector<float> buf((burst + gap) * n_bursts, 0.0f);
    for (int i = 0; i < n_bursts; ++i)
        for (int s = i * (burst + gap); s < i * (burst + gap) + burst; ++s)
            buf[s] = 0.5f;
    auto onsets = Slicer::detect_onsets(buf.data(), static_cast<int>(buf.size()), 44100.0f, 0.05f, 1.0f);
    REQUIRE(static_cast<int>(onsets.size()) <= 16);
}

// S5 -- chop_to_pads fills two pads with correct data
TEST_CASE("slicer chop_to_pads two slices fills pads correctly", "[slicer]") {
    SlicerJuceFixture f;
    std::vector<float> buf(2048);
    std::fill(buf.begin(), buf.begin() + 1024, 1.0f);
    std::fill(buf.begin() + 1024, buf.end(), 2.0f);

    PadBank bank;
    VoicePool pool;
    pool.prepare(44100.0);

    Slicer::chop_to_pads(bank, pool, buf.data(), 2048, {0, 1024});

    REQUIRE(bank.pad(0).num_samples() == 1024);
    REQUIRE(bank.pad(1).num_samples() == 1024);
    REQUIRE(bank.pad(0).data()[0] == Approx(1.0f));
    REQUIRE(bank.pad(1).data()[0] == Approx(2.0f));
}

// S6 -- chop_to_pads single onset loads whole buffer
TEST_CASE("slicer chop_to_pads single onset loads whole buffer", "[slicer]") {
    SlicerJuceFixture f;
    std::vector<float> buf(512, 0.7f);
    PadBank bank;
    VoicePool pool;
    Slicer::chop_to_pads(bank, pool, buf.data(), 512, {0});
    REQUIRE(bank.pad(0).num_samples() == 512);
    REQUIRE(bank.pad(0).data()[0] == Approx(0.7f));
}

// S7 -- chop_to_pads empty onset list writes nothing
TEST_CASE("slicer chop_to_pads empty onsets writes nothing", "[slicer]") {
    SlicerJuceFixture f;
    std::vector<float> buf(512, 1.0f);
    PadBank bank;
    VoicePool pool;
    Slicer::chop_to_pads(bank, pool, buf.data(), 512, {});
    REQUIRE(!bank.pad(0).has_sample());
}

// S8 -- chop_to_pads 20 onsets clamps to 16 without out-of-bounds
TEST_CASE("slicer chop_to_pads 20 onsets clamps to 16", "[slicer]") {
    SlicerJuceFixture f;
    const int total = 20 * 100;
    std::vector<float> buf(total, 1.0f);
    std::vector<int> onsets;
    for (int i = 0; i < 20; ++i)
        onsets.push_back(i * 100);

    PadBank bank;
    VoicePool pool;
    Slicer::chop_to_pads(bank, pool, buf.data(), total, onsets);
    REQUIRE(bank.pad(15).has_sample());
}

// S9 -- detect_onsets min_slice_ms suppresses close onsets (AC-5)
TEST_CASE("slicer detect_onsets min_slice_ms suppresses close onsets", "[slicer]") {
    SlicerJuceFixture f;
    // Three bursts at 0, 100, 200 samples; sr=44100 -> each ~2.3ms apart.
    // min_slice_ms=500ms -> min_spacing=22050 samples; only onset 0 survives.
    const int total = 512;
    std::vector<float> buf(total, 0.0f);
    for (int i = 0; i < 100; ++i)
        buf[i] = 0.8f;
    for (int i = 200; i < 300; ++i)
        buf[i] = 0.8f;
    auto onsets = Slicer::detect_onsets(buf.data(), total, 44100.0f, 0.05f, 500.0f);
    REQUIRE(onsets.size() == 1);
    REQUIRE(onsets[0] == 0);
}

// S10 -- chop_to_pads clears stale pads on re-chop (AC-4)
TEST_CASE("slicer chop_to_pads clears stale pads on re-chop", "[slicer]") {
    SlicerJuceFixture f;
    // First chop: 8 pads loaded
    std::vector<float> buf8(800, 1.0f);
    std::vector<int> onsets8;
    for (int i = 0; i < 8; ++i)
        onsets8.push_back(i * 100);

    PadBank bank;
    VoicePool pool;
    Slicer::chop_to_pads(bank, pool, buf8.data(), 800, onsets8);
    REQUIRE(bank.pad(7).has_sample()); // sanity: 8 pads loaded

    // Second chop: only 3 pads
    std::vector<float> buf3(300, 2.0f);
    Slicer::chop_to_pads(bank, pool, buf3.data(), 300, {0, 100, 200});
    REQUIRE(bank.pad(0).has_sample());
    REQUIRE(bank.pad(2).has_sample());
    REQUIRE(!bank.pad(3).has_sample()); // stale data cleared
    REQUIRE(!bank.pad(7).has_sample()); // stale data cleared
}
