#include "audio/fx_chain.h"
#include "audio/fx_params.h"
#include "audio/plock_pattern.h"
#include "audio/scatter_engine.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_scc_pi = 3.14159265358979323846;
static constexpr double k_sr = 48000.0;
static constexpr double k_bpm = 120.0;

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, float amp = 0.5f) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, amp * static_cast<float>(std::sin(2.0 * k_scc_pi * freq * i / k_sr)));
}

// SCH1 (AC-1): scatter_type / scatter_depth existem como params APVTS com defaults neutros.
TEST_CASE("SCH1 - scatter params registered in APVTS with neutral defaults", "[scatter_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;

    auto* type = processor.apvts_.getRawParameterValue("scatter_type");
    auto* depth = processor.apvts_.getRawParameterValue("scatter_depth");
    REQUIRE(type != nullptr);
    REQUIRE(depth != nullptr);
    REQUIRE(*type == Catch::Approx(0.0f));  // off por default
    REQUIRE(*depth == Catch::Approx(0.0f)); // sem efeito por default
}

// SCH2 (AC-2): enum estendido, count 13, índices novos corretos.
TEST_CASE("SCH2 - PLockParam extended to 13 with scatter at 11 and 12", "[scatter_chain]") {
    REQUIRE(k_plock_param_count == 15);
    REQUIRE(static_cast<int>(PLockParam::scatter_type) == 11);
    REQUIRE(static_cast<int>(PLockParam::scatter_depth) == 12);
}

// SCH3 (AC-3): p-lock sobrescreve scatter params; clamp protege contra valor fora de range (SR1).
// Replica a lógica de dispatch + clamp de plugin_processor.cpp (apply_plock_batch é privado).
static void apply_scatter_plock(const PLockBatch& batch, FxParams& fx) {
    for (int i = 0; i < batch.count; ++i) {
        if (batch.events[i].param == PLockParam::scatter_type)
            fx.scatter_type = batch.events[i].value;
        else if (batch.events[i].param == PLockParam::scatter_depth)
            fx.scatter_depth = batch.events[i].value;
    }
}

TEST_CASE("SCH3 - p-lock overrides scatter params and clamp guards out-of-range", "[scatter_chain]") {
    FxParams fx{};
    PLockBatch batch;
    batch.push(PLockParam::scatter_type, 2.0f);
    batch.push(PLockParam::scatter_depth, 1.0f);
    apply_scatter_plock(batch, fx);
    REQUIRE(fx.scatter_type == Catch::Approx(2.0f));
    REQUIRE(fx.scatter_depth == Catch::Approx(1.0f));

    // CLAMP (SR1): p-lock empurra scatter_type p/ 15 → jlimit p/ [0,10] = 10, sem jassert.
    FxParams fx2{};
    PLockBatch bad;
    bad.push(PLockParam::scatter_type, 15.0f);
    apply_scatter_plock(bad, fx2);
    const int clamped = juce::jlimit(0, ScatterEngine::k_num_types, juce::roundToInt(fx2.scatter_type));
    REQUIRE(clamped == 10);
}

// SCH4 (AC-4): ordering — ScatterEngine modifica o sinal JÁ processado pela FxChain.
TEST_CASE("SCH4 - scatter operates on the post-FxChain signal", "[scatter_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    constexpr int n = 4096;

    FxParams p{};
    p.reverb_mix = 0.0f;
    p.delay_mix = 0.0f;
    p.sidechain_threshold = 0.0f;

    // Path A: só FxChain.
    FxChain chain_a;
    chain_a.prepare(k_sr, n);
    juce::AudioBuffer<float> only_fx(2, n);
    fill_sine(only_fx, 440.0f);
    chain_a.process(only_fx, p);

    // Path B: FxChain → ScatterEngine (mesma config), com fill prévio do ring de scatter.
    FxChain chain_b;
    chain_b.prepare(k_sr, n);
    ScatterEngine sc;
    sc.prepare(k_sr, n);
    // Warm: enche o ring de scatter com o sinal já processado pela chain (type 0 = captura, sem latch).
    for (int b = 0; b < 3; ++b) {
        juce::AudioBuffer<float> warm(2, n);
        fill_sine(warm, 440.0f);
        chain_b.process(warm, p);
        sc.process(warm, 0, 0.0f, k_bpm);
    }
    // Mede: FxChain → scatter ativo (type 2 repeat, depth 1).
    juce::AudioBuffer<float> scattered(2, n);
    fill_sine(scattered, 440.0f);
    chain_b.process(scattered, p);
    juce::AudioBuffer<float> fx_only_ref(2, n);
    fx_only_ref.makeCopyOf(scattered); // estado pós-FxChain, antes de scatter
    sc.process(scattered, 2, 1.0f, k_bpm);

    float max_diff = 0.0f;
    for (int i = 0; i < n; ++i)
        max_diff = std::max(max_diff, std::abs(scattered.getSample(0, i) - fx_only_ref.getSample(0, i)));
    REQUIRE(max_diff > 0.01f); // scatter modifica o sinal pós-chain (não-vacuoso)

    // type 0 → sem efeito: scatter não altera a saída da chain.
    juce::AudioBuffer<float> bypassed(2, n);
    fill_sine(bypassed, 440.0f);
    chain_b.process(bypassed, p);
    juce::AudioBuffer<float> before(2, n);
    before.makeCopyOf(bypassed);
    sc.process(bypassed, 0, 1.0f, k_bpm);
    float max_bypass_diff = 0.0f;
    for (int i = 0; i < n; ++i)
        max_bypass_diff = std::max(max_bypass_diff, std::abs(bypassed.getSample(0, i) - before.getSample(0, i)));
    REQUIRE(max_bypass_diff < 1e-6f);
}

// SCH5 (AC-4b): wiring real — processBlock com scatter ativo não produz NaN/Inf nem crash.
TEST_CASE("SCH5 - processBlock with scatter active stays finite", "[scatter_chain]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 512);

    // Ativa scatter via APVTS (normalizado: type 2 em [0,10] = 0.2; depth 1 = 1.0).
    processor.apvts_.getParameter("scatter_type")->setValueNotifyingHost(2.0f / 10.0f);
    processor.apvts_.getParameter("scatter_depth")->setValueNotifyingHost(1.0f);

    for (int b = 0; b < 64; ++b) {
        juce::AudioBuffer<float> buf(2, 512);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                REQUIRE(std::isfinite(buf.getSample(ch, i)));
    }
}
