// Fase 4 (04-01): PadBank — roteamento nota→pad, varispeed, mapeamento de
// velocity fixado e protocolo de load seguro (auditoria 04-01).

#include "audio/pad_bank.h"
#include "audio/scheduler.h"
#include "audio/voice_pool.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

// Fixture do JUCE — necessária para tipos que usam JUCE internamente
struct PadBankJuceFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

// Preenche o pad com valor DC constante (facilita medir amplitude)
static void load_dc_pad(PadBank& bank, int pad_index, int num_samples, float value) {
    auto& buf = bank.pad(pad_index).sample_buffer();
    buf.setSize(1, num_samples);
    for (int i = 0; i < num_samples; ++i)
        buf.setSample(0, i, value);
}

// 1. AC-1: nota mapeia para o pad correto; fora do banco ou vazio = nullptr
TEST_CASE_METHOD(PadBankJuceFixture, "pad bank note to pad mapping", "[padbank]") {
    PadBank bank;
    load_dc_pad(bank, 0, 100, 1.0f);

    // Nota 36 (k_base_note) → pad 0
    REQUIRE(bank.pad_for_note(36) == &bank.pad(0));

    // Nota fora do intervalo [36, 52) → nullptr
    REQUIRE(bank.pad_for_note(35) == nullptr);
    REQUIRE(bank.pad_for_note(52) == nullptr);
    REQUIRE(bank.pad_for_note(0) == nullptr);

    // Nota válida mas pad vazio → nullptr
    REQUIRE(bank.pad_for_note(40) == nullptr);

    // Carrega pad 4 (nota 40) → agora retorna o pad
    load_dc_pad(bank, 4, 100, 0.5f);
    REQUIRE(bank.pad_for_note(40) == &bank.pad(4));
}

// 2. AC-2: taxa varispeed = 2^((semitons + cents/100)/12)
TEST_CASE("pad playback rate follows varispeed formula", "[padbank]") {
    SamplePad pad;

    pad.pitch_semitones = 0;
    pad.pitch_cents = 0;
    REQUIRE(pad.playback_rate() == Catch::Approx(1.0));

    pad.pitch_semitones = 12;
    REQUIRE(pad.playback_rate() == Catch::Approx(2.0));

    pad.pitch_semitones = -12;
    REQUIRE(pad.playback_rate() == Catch::Approx(0.5));

    // 100 cents = 1 semitom
    pad.pitch_semitones = 0;
    pad.pitch_cents = 100;
    REQUIRE(pad.playback_rate() == Catch::Approx(std::exp2(1.0 / 12.0)));
}

// 3. AC-1: roteamento ponta a ponta — cada nota dispara o sample do seu pad
TEST_CASE_METHOD(PadBankJuceFixture, "scheduler routes notes to distinct pads", "[padbank][scheduler]") {
    PadBank bank;
    load_dc_pad(bank, 0, 1000, 0.25f); // nota 36
    load_dc_pad(bank, 1, 1000, 0.5f);  // nota 37

    Scheduler sched;
    constexpr int block_size = 256;
    constexpr float k_center_pan = 0.70710678f; // equal-power no centro

    // Nota 36 → amplitude do pad 0 (0.25 × pan central)
    {
        VoicePool pool;
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);
        std::vector<float> left(block_size, 0.0f), right(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(left.data(), right.data(), block_size);
        // Frame 100 (após fade-in de 32): DC 0.25 × 0.7071
        REQUIRE(left[100] == Catch::Approx(0.25f * k_center_pan).margin(1e-3));
    }

    // Nota 37 → amplitude do pad 1 (0.5 × pan central)
    {
        VoicePool pool;
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 37, 1.0f), 0);
        std::vector<float> left(block_size, 0.0f), right(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(left.data(), right.data(), block_size);
        REQUIRE(left[100] == Catch::Approx(0.5f * k_center_pan).margin(1e-3));
    }

    // Nota sem pad (45 vazio) → silêncio, sem erro
    {
        VoicePool pool;
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 45, 1.0f), 0);
        std::vector<float> left(block_size, 0.0f), right(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(left.data(), right.data(), block_size);
        for (int i = 0; i < block_size; ++i)
            REQUIRE(std::abs(left[static_cast<std::size_t>(i)]) < 1e-10f);
    }
}

// 4. Mapeamento de velocity fixado: linear (velocity/127) × pad.gain
TEST_CASE_METHOD(PadBankJuceFixture, "velocity mapping is pinned linear", "[padbank][scheduler]") {
    PadBank bank;
    load_dc_pad(bank, 0, 1000, 1.0f);

    Scheduler sched;
    constexpr int block_size = 256;

    auto peak_at_frame_100 = [&](float velocity) {
        VoicePool pool;
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, velocity), 0);
        std::vector<float> left(block_size, 0.0f), right(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(left.data(), right.data(), block_size);
        return left[100];
    };

    const float peak_full = peak_at_frame_100(1.0f);           // velocity 127
    const float peak_half = peak_at_frame_100(64.0f / 127.0f); // velocity 64

    // v127 → pad.gain (1.0) × pan central
    REQUIRE(peak_full == Catch::Approx(0.70710678f).margin(1e-3));
    // v64/v127 = 64/127 ≈ 0.5039 (linear, sem curva)
    REQUIRE(peak_half / peak_full == Catch::Approx(64.0f / 127.0f).margin(1e-3));
}

// 5. AC-5: protocolo de load seguro — reset_all antes da mutação do buffer
TEST_CASE_METHOD(PadBankJuceFixture, "safe load protocol no dangling voice pointers", "[padbank][rt-safety]") {
    PadBank bank;
    load_dc_pad(bank, 0, 4000, 1.0f);

    Scheduler sched;
    VoicePool pool;
    constexpr int block_size = 256;

    // Dispara o pad 0 e processa um bloco (voz ativa lendo o buffer)
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);
    std::vector<float> left(block_size, 0.0f), right(block_size, 0.0f);
    sched.process(midi, pool, bank, block_size);
    pool.process_all(left.data(), right.data(), block_size);
    REQUIRE(std::abs(left[100]) > 1e-6f); // Voz tocando

    // PROTOCOLO: invalida as vozes ANTES de mutar o buffer (auditoria 04-01)
    pool.reset_all();
    auto& buf = bank.pad(0).sample_buffer();
    buf.setSize(1, 50); // Realocação — ponteiro antigo inválido
    for (int i = 0; i < 50; ++i)
        buf.setSample(0, i, 0.0f);

    // Processa após a mutação: nenhuma voz ativa referencia o buffer antigo
    std::vector<float> left2(block_size, 0.0f), right2(block_size, 0.0f);
    pool.process_all(left2.data(), right2.data(), block_size);
    for (int i = 0; i < block_size; ++i)
        REQUIRE(std::abs(left2[static_cast<std::size_t>(i)]) < 1e-10f); // Silêncio — sem use-after-free
}
