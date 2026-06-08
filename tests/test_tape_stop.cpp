#include "audio/tape_stop_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_pi = 3.14159265358979323846;
static constexpr double k_sr = 48000.0;

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, float amp = 0.5f) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, amp * static_cast<float>(std::sin(2.0 * k_pi * freq * i / k_sr)));
}

// TS1 (AC-4): tape_stop=0 → bypass exato.
TEST_CASE("TS1 - tape_stop 0 is exact bypass", "[tape_stop]") {
    constexpr int n = 2048;
    juce::AudioBuffer<float> buf(2, n);
    fill_sine(buf, 440.0f);
    juce::AudioBuffer<float> orig(2, n);
    orig.makeCopyOf(buf);

    TapeStopProcessor proc;
    proc.prepare(k_sr, n);
    proc.process(buf, 0.0f);

    float max_err = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < n; ++i)
            max_err = std::max(max_err, std::abs(buf.getSample(ch, i) - orig.getSample(ch, i)));
    REQUIRE(max_err < 1e-6f);
}

// TS2 (AC-4, M1): tape_stop=1.0 → halt vai a SILÊNCIO (não DC congelado).
TEST_CASE("TS2 - full tape_stop halts to silence not DC", "[tape_stop]") {
    constexpr int blk = 512;
    TapeStopProcessor proc;
    proc.prepare(k_sr, blk);

    // Processa muitos blocos a tape_stop=1.0 p/ o rate suavizado assentar em 0.
    juce::AudioBuffer<float> buf(2, blk);
    float final_peak = 0.0f;
    for (int b = 0; b < 60; ++b) {
        fill_sine(buf, 440.0f);
        proc.process(buf, 1.0f);
        if (b >= 55) { // janela final, após settle do ramp de 30 ms
            for (int i = 0; i < blk; ++i)
                final_peak = std::max(final_peak, std::abs(buf.getSample(0, i)));
        }
    }
    REQUIRE(final_peak < 0.01f); // silêncio (gain→0), não DC sustentado
}

// TS3 (AC-4): tape_stop=0.5 → saída diverge do dry (pitch/tempo rebaixado).
TEST_CASE("TS3 - partial tape_stop diverges from dry", "[tape_stop]") {
    constexpr int blk = 512;
    TapeStopProcessor proc;
    proc.prepare(k_sr, blk);

    // Warm um pouco + medir num bloco com sine conhecido.
    juce::AudioBuffer<float> warm(2, blk);
    for (int b = 0; b < 4; ++b) {
        fill_sine(warm, 440.0f);
        proc.process(warm, 0.5f);
    }
    juce::AudioBuffer<float> buf(2, blk);
    fill_sine(buf, 440.0f);
    juce::AudioBuffer<float> orig(2, blk);
    orig.makeCopyOf(buf);
    proc.process(buf, 0.5f);

    float max_diff = 0.0f;
    for (int i = 0; i < blk; ++i)
        max_diff = std::max(max_diff, std::abs(buf.getSample(0, i) - orig.getSample(0, i)));
    REQUIRE(max_diff > 0.01f);
}
