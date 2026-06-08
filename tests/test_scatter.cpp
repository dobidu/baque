#include "audio/scatter_engine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_pi = 3.14159265358979323846;
static constexpr double k_sr = 48000.0;
static constexpr double k_bpm = 120.0;
static constexpr int k_slice = 6000; // 1/16 nota a 120 BPM em 48 kHz

// Preenche todos os canais com uma rampa crescente [0, amp].
static void fill_ramp(juce::AudioBuffer<float>& buf, float amp = 1.0f) {
    const int n = buf.getNumSamples();
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < n; ++i)
            buf.setSample(ch, i, amp * static_cast<float>(i) / static_cast<float>(n));
}

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, float amp = 0.5f) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, amp * static_cast<float>(std::sin(2.0 * k_pi * freq * i / k_sr)));
}

static void fill_const(juce::AudioBuffer<float>& buf, float vL, float vR) {
    for (int i = 0; i < buf.getNumSamples(); ++i) {
        buf.setSample(0, i, vL);
        if (buf.getNumChannels() > 1)
            buf.setSample(1, i, vR);
    }
}

// SC1 (AC-1): type=0 → saída idêntica à entrada.
TEST_CASE("SC1 - type 0 is exact bypass passthrough", "[scatter]") {
    constexpr int n = 4096;
    juce::AudioBuffer<float> buf(2, n);
    fill_sine(buf, 440.0f);
    juce::AudioBuffer<float> orig(2, n);
    orig.makeCopyOf(buf);

    ScatterEngine eng;
    eng.prepare(k_sr, n);
    eng.process(buf, 0, 1.0f, k_bpm);

    float max_err = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < n; ++i)
            max_err = std::max(max_err, std::abs(buf.getSample(ch, i) - orig.getSample(ch, i)));
    REQUIRE(max_err < 1e-6f);
}

// SC2 (AC-2): repeat — a slice congelada faz loop; conteúdo recorre a cada slice_len.
TEST_CASE("SC2 - repeat replays frozen slice periodically", "[scatter]") {
    ScatterEngine eng;
    eng.prepare(k_sr, k_slice);

    // Fill phase: enche o ring com sinal real ANTES de ativar (evita captura zerada — vacuoso).
    juce::AudioBuffer<float> warm(2, k_slice * 2);
    fill_sine(warm, 220.0f);
    eng.process(warm, 0, 1.0f, k_bpm); // type 0 → captura, sem latch

    // Bloco ativo de 2 slices: repeat 1/16 (type 2), depth 1.
    juce::AudioBuffer<float> act(2, k_slice * 2);
    fill_sine(act, 220.0f);
    eng.process(act, 2, 1.0f, k_bpm);

    // out[j] deve recorrer em out[j+slice] (mesma slice congelada tocada 2x).
    float max_diff = 0.0f;
    float max_abs = 0.0f;
    for (int j = 0; j < k_slice; j += 17) {
        const float a = act.getSample(0, j);
        const float b = act.getSample(0, j + k_slice);
        max_diff = std::max(max_diff, std::abs(a - b));
        max_abs = std::max(max_abs, std::abs(a));
    }
    REQUIRE(max_diff < 1e-6f); // recorrência exata
    REQUIRE(max_abs > 0.01f);  // não-vacuoso: a slice tem conteúdo
}

// SC3 (AC-3): reverse — dentro da slice a saída decresce sobre rampa crescente.
TEST_CASE("SC3 - reverse plays the slice backwards", "[scatter]") {
    ScatterEngine eng;
    eng.prepare(k_sr, k_slice);

    juce::AudioBuffer<float> warm(2, k_slice * 2);
    fill_ramp(warm, 1.0f); // rampa crescente
    eng.process(warm, 0, 1.0f, k_bpm);

    juce::AudioBuffer<float> act(2, k_slice);
    fill_ramp(act, 1.0f);
    eng.process(act, 5, 1.0f, k_bpm); // reverse 1/16

    // Slice congelada = fim da rampa (crescente). Reverso → saída deve cair ao longo da slice.
    REQUIRE(act.getSample(0, 100) > act.getSample(0, k_slice - 100));
    REQUIRE(act.getSample(0, 1000) > act.getSample(0, 5000));
}

// SC4 (AC-4): gate — existe região silenciada E região preservando o nível.
TEST_CASE("SC4 - gate chops the slice on and off", "[scatter]") {
    ScatterEngine eng;
    eng.prepare(k_sr, k_slice);

    juce::AudioBuffer<float> warm(2, k_slice * 2);
    fill_const(warm, 0.5f, 0.5f);
    eng.process(warm, 0, 1.0f, k_bpm);

    juce::AudioBuffer<float> act(2, k_slice);
    fill_const(act, 0.5f, 0.5f);
    eng.process(act, 6, 1.0f, k_bpm); // gate 1/16, duty 50%

    bool found_gated = false;
    bool found_open = false;
    for (int i = 0; i < k_slice; ++i) {
        const float v = std::abs(act.getSample(0, i));
        if (v < 0.01f)
            found_gated = true;
        if (std::abs(v - 0.5f) < 0.01f)
            found_open = true;
    }
    REQUIRE(found_gated); // anti-vacuoso: existe chop silenciado
    REQUIRE(found_open);  // nível preservado fora do gate
}

// SC5 (AC-5): decimate — sample-and-hold produz degraus (amostras consecutivas iguais).
TEST_CASE("SC5 - decimate holds samples across the stride", "[scatter]") {
    ScatterEngine eng;
    eng.prepare(k_sr, k_slice);

    juce::AudioBuffer<float> warm(2, k_slice * 2);
    fill_sine(warm, 330.0f); // suave: sem amostras consecutivas iguais
    eng.process(warm, 0, 1.0f, k_bpm);

    juce::AudioBuffer<float> act(2, k_slice);
    fill_sine(act, 330.0f);
    eng.process(act, 9, 1.0f, k_bpm); // decimate stride forte (16)

    // Dentro de um stride os 16 primeiros samples seguram o mesmo valor; o próximo bucket difere.
    REQUIRE(act.getSample(0, 0) == Catch::Approx(act.getSample(0, 15)).margin(1e-6));
    REQUIRE(act.getSample(0, 0) != Catch::Approx(act.getSample(0, 16)).margin(1e-6));
}

// SC6 (AC-6): depth=0 → seco exato; depth=1 → diverge do seco.
TEST_CASE("SC6 - depth 0 is exact dry, depth 1 diverges", "[scatter]") {
    constexpr int n = k_slice * 2;

    // depth = 0 → bypass exato
    {
        ScatterEngine eng;
        eng.prepare(k_sr, n);
        juce::AudioBuffer<float> buf(2, n);
        fill_sine(buf, 440.0f);
        juce::AudioBuffer<float> orig(2, n);
        orig.makeCopyOf(buf);
        eng.process(buf, 2, 0.0f, k_bpm);
        float max_err = 0.0f;
        for (int i = 0; i < n; ++i)
            max_err = std::max(max_err, std::abs(buf.getSample(0, i) - orig.getSample(0, i)));
        REQUIRE(max_err < 1e-6f);
    }
    // depth = 1 → diverge do seco
    {
        ScatterEngine eng;
        eng.prepare(k_sr, n);
        juce::AudioBuffer<float> warm(2, n);
        fill_sine(warm, 440.0f);
        eng.process(warm, 0, 1.0f, k_bpm);
        juce::AudioBuffer<float> buf(2, n);
        fill_sine(buf, 440.0f);
        juce::AudioBuffer<float> orig(2, n);
        orig.makeCopyOf(buf);
        eng.process(buf, 5, 1.0f, k_bpm); // reverse → claramente != seco
        float max_diff = 0.0f;
        for (int i = 0; i < n; ++i)
            max_diff = std::max(max_diff, std::abs(buf.getSample(0, i) - orig.getSample(0, i)));
        REQUIRE(max_diff > 0.01f);
    }
}

// SC7 (AC-7): 256 blocos × todos os 11 types → sem NaN/Inf. (Alloc-freeness: ring pré-alocado em
// prepare(); process() só lê/escreve buffers fixos — verificado por inspeção do código.)
TEST_CASE("SC7 - no NaN or Inf across all types over many blocks", "[scatter]") {
    constexpr int block = 512;
    ScatterEngine eng;
    eng.prepare(k_sr, block);
    for (int b = 0; b < 256; ++b) {
        juce::AudioBuffer<float> buf(2, block);
        fill_sine(buf, 440.0f);
        const int type = b % 11; // 0..10
        eng.process(buf, type, 1.0f, k_bpm);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < block; ++i)
                REQUIRE(std::isfinite(buf.getSample(ch, i)));
    }
}

// SC8 (AC-8): slice beat-synced.
TEST_CASE("SC8 - slice length matches the beat subdivision", "[scatter]") {
    REQUIRE(ScatterEngine::slice_length_samples(48000.0, 120.0, 16) == 6000);
    REQUIRE(ScatterEngine::slice_length_samples(48000.0, 120.0, 8) == 12000);
    REQUIRE(ScatterEngine::slice_length_samples(48000.0, 120.0, 32) == 3000);
}

// SC9 (AC-9): reset() limpa o estado — sem resíduo da captura anterior.
TEST_CASE("SC9 - reset clears state, no stale slice replay", "[scatter]") {
    ScatterEngine eng;
    eng.prepare(k_sr, k_slice);

    // Captura sinal A (0.3), engaja repeat (mid-slice).
    juce::AudioBuffer<float> warmA(2, k_slice * 2);
    fill_const(warmA, 0.3f, 0.3f);
    eng.process(warmA, 0, 1.0f, k_bpm);
    juce::AudioBuffer<float> actA(2, k_slice / 2);
    fill_const(actA, 0.3f, 0.3f);
    eng.process(actA, 2, 1.0f, k_bpm); // latch em A

    eng.reset();

    // Alimenta sinal B (0.7), engaja repeat.
    juce::AudioBuffer<float> warmB(2, k_slice * 2);
    fill_const(warmB, 0.7f, 0.7f);
    eng.process(warmB, 0, 1.0f, k_bpm);
    juce::AudioBuffer<float> actB(2, k_slice);
    fill_const(actB, 0.7f, 0.7f);
    eng.process(actB, 2, 1.0f, k_bpm); // latch em B

    for (int i = 0; i < k_slice; ++i) {
        REQUIRE(actB.getSample(0, i) == Catch::Approx(0.7f).margin(1e-4)); // só B, nenhum resíduo de A
    }
}

// SC10 (SR1): coerência estéreo — gate silencia nas MESMAS posições em L e R.
TEST_CASE("SC10 - gate mask is stereo-coherent", "[scatter]") {
    ScatterEngine eng;
    eng.prepare(k_sr, k_slice);

    juce::AudioBuffer<float> warm(2, k_slice * 2);
    fill_const(warm, 0.4f, 0.6f); // L != R, ambos != 0
    eng.process(warm, 0, 1.0f, k_bpm);

    juce::AudioBuffer<float> act(2, k_slice);
    fill_const(act, 0.4f, 0.6f);
    eng.process(act, 6, 1.0f, k_bpm); // gate

    int gated = 0;
    int open = 0;
    for (int i = 0; i < k_slice; ++i) {
        const bool offL = std::abs(act.getSample(0, i)) < 0.01f;
        const bool offR = std::abs(act.getSample(1, i)) < 0.01f;
        REQUIRE(offL == offR); // mesma posição de gate nos dois canais
        if (offL)
            ++gated;
        else
            ++open;
    }
    REQUIRE(gated > 0); // não-vacuoso
    REQUIRE(open > 0);
}
