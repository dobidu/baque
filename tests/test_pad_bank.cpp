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

// 6. Choke: VoicePool::choke_group() desativa vozes do mesmo grupo
TEST_CASE_METHOD(PadBankJuceFixture, "choke group silences voice of matching pad group", "[padbank][choke]") {
    PadBank bank;
    load_dc_pad(bank, 0, 8000, 1.0f); // pad 0 — grupo 1
    bank.pad(0).choke_group = 1;

    VoicePool pool;
    constexpr int block_size = 256;

    // Dispara pad 0 diretamente e verifica que produz sinal
    auto* v = pool.trigger_at(0, bank.pad(0).data(), bank.pad(0).num_samples(), 1.0f, 1.0, false, 0.0f, 0);
    {
        std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
        pool.process_all(l.data(), r.data(), block_size);
        REQUIRE(std::abs(l[100]) > 1e-4f); // voz ativa
    }

    // Choke: inicia fade-out na voz do pad 0
    pool.choke_group(1, bank);
    REQUIRE(v->is_active()); // ainda ativa — fade em curso

    // Após fade-out de 32 frames, voz deve estar silenciosa e inativa
    {
        std::vector<float> l(40, 0.0f), r(40, 0.0f);
        pool.process_all(l.data(), r.data(), 40);
        REQUIRE(!v->is_active()); // fade-out completo (k_fade_frames = 32)
    }
}

// 7. Choke: grupos distintos nao interferem entre si
TEST_CASE_METHOD(PadBankJuceFixture, "choke group does not affect different group", "[padbank][choke]") {
    PadBank bank;
    load_dc_pad(bank, 0, 8000, 1.0f); // pad 0 — grupo 1
    load_dc_pad(bank, 2, 8000, 1.0f); // pad 2 — grupo 2
    bank.pad(0).choke_group = 1;
    bank.pad(2).choke_group = 2;

    VoicePool pool;
    constexpr int block_size = 256;

    // Dispara pad 0 (grupo 1)
    static_cast<void>(pool.trigger_at(0, bank.pad(0).data(), bank.pad(0).num_samples(), 1.0f, 1.0, false, 0.0f, 0));

    // Choke do grupo 2 — nao deve afetar pad 0
    pool.choke_group(2, bank);

    // Processa: pad 0 ainda deve estar ativo e produzindo sinal
    std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
    pool.process_all(l.data(), r.data(), block_size);
    REQUIRE(std::abs(l[100]) > 1e-4f); // voz intacta
}

// 8. Choke via scheduler: re-trigger do mesmo pad choca a voz anterior
TEST_CASE_METHOD(PadBankJuceFixture,
                 "scheduler choke on self retrigger keeps single voice amplitude",
                 "[padbank][choke][scheduler]") {
    PadBank bank;
    load_dc_pad(bank, 0, 16000, 1.0f); // pad longo (nota 36)
    bank.pad(0).choke_group = 1;

    Scheduler sched;
    VoicePool pool;
    constexpr int block_size = 256;
    constexpr float k_cpan = 0.70710678f;

    // Bloco 1: primeiro trigger
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);
        std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(l.data(), r.data(), block_size);
    }

    // Bloco 2: re-trigger — scheduler deve chocar a primeira voz antes de disparar segunda
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);
        std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(l.data(), r.data(), block_size);

        // Frame 50 (apos fade-out de 32 da primeira): apenas segunda voz ativa.
        // Com duas vozes somadas (sem choke) seria ~2x; com choke deve ser ~1x.
        const float expected = 1.0f * k_cpan; // velocidade 1.0, DC 1.0, pan central
        REQUIRE(l[50] == Catch::Approx(expected).margin(0.05f));
    }
}

// Auxiliar: carrega buffer de uma camada com valor DC constante
static void load_dc_layer(SamplePad& pad, int layer_idx, int num_samples, float value, uint8_t vel_lo, uint8_t vel_hi) {
    auto& lyr = pad.layer(layer_idx);
    lyr.buffer.setSize(1, num_samples);
    for (int i = 0; i < num_samples; ++i)
        lyr.buffer.setSample(0, i, value);
    lyr.vel_lo = vel_lo;
    lyr.vel_hi = vel_hi;
}

// 9. AC-2: camada de velocity seleciona buffer correto quando vel corresponde
TEST_CASE_METHOD(PadBankJuceFixture, "velocity layer selects layer buffer when velocity matches", "[padbank][layers]") {
    PadBank bank;
    load_dc_pad(bank, 0, 2000, 0.5f);                  // base buffer DC=0.5 (fallback)
    load_dc_layer(bank.pad(0), 0, 2000, 1.0f, 0, 127); // camada 0 DC=1.0 vel 0-127
    bank.pad(0).set_num_layers(1);

    Scheduler sched;
    constexpr int block_size = 256;
    constexpr float k_cpan = 0.70710678f;

    VoicePool pool;
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, 64.0f / 127.0f), 0); // velocity 64
    std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
    sched.process(midi, pool, bank, block_size);
    pool.process_all(l.data(), r.data(), block_size);

    // Camada DC=1.0 selecionada (não base DC=0.5)
    const float vel_gain = 64.0f / 127.0f;
    REQUIRE(l[100] == Catch::Approx(1.0f * vel_gain * k_cpan).margin(1e-3f));
}

// 10. AC-2: sem camada correspondente → fallback para buffer_ base
TEST_CASE_METHOD(PadBankJuceFixture, "velocity layer falls back to base when no layer matches", "[padbank][layers]") {
    PadBank bank;
    load_dc_pad(bank, 0, 2000, 0.5f);                 // base buffer DC=0.5
    load_dc_layer(bank.pad(0), 0, 2000, 1.0f, 0, 63); // camada 0 DC=1.0 vel 0-63 apenas
    bank.pad(0).set_num_layers(1);

    Scheduler sched;
    constexpr int block_size = 256;
    constexpr float k_cpan = 0.70710678f;

    VoicePool pool;
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, 80.0f / 127.0f), 0); // velocity 80 (>63 = sem match)
    std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
    sched.process(midi, pool, bank, block_size);
    pool.process_all(l.data(), r.data(), block_size);

    // Usa base DC=0.5 (camada vel 0-63 nao cobre velocity 80)
    const float vel_gain = 80.0f / 127.0f;
    REQUIRE(l[100] == Catch::Approx(0.5f * vel_gain * k_cpan).margin(1e-3f));
}

// 11. AC-2: round-robin alterna entre camadas correspondentes a cada trigger
TEST_CASE_METHOD(PadBankJuceFixture, "round robin alternates between matching layers", "[padbank][layers][rr]") {
    PadBank bank;
    load_dc_pad(bank, 0, 2000, 0.5f);                  // base buffer (necessário para pad_for_note)
    load_dc_layer(bank.pad(0), 0, 2000, 1.0f, 0, 127); // camada 0 DC=1.0
    load_dc_layer(bank.pad(0), 1, 2000, 2.0f, 0, 127); // camada 1 DC=2.0
    bank.pad(0).set_num_layers(2);

    Scheduler sched; // mesmo scheduler — rr_index persiste entre triggers
    constexpr int block_size = 256;
    constexpr float k_cpan = 0.70710678f;

    float amp1 = 0.0f, amp2 = 0.0f;

    // Trigger 1 → rr_index=0 → camada 0 (DC=1.0)
    {
        VoicePool pool;
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);
        std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(l.data(), r.data(), block_size);
        amp1 = l[100];
    }

    // Trigger 2 → rr_index=1 → camada 1 (DC=2.0)
    {
        VoicePool pool;
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);
        std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
        sched.process(midi, pool, bank, block_size);
        pool.process_all(l.data(), r.data(), block_size);
        amp2 = l[100];
    }

    REQUIRE(amp1 == Catch::Approx(1.0f * k_cpan).margin(1e-3f)); // camada 0
    REQUIRE(amp2 == Catch::Approx(2.0f * k_cpan).margin(1e-3f)); // camada 1
    REQUIRE(amp2 / amp1 == Catch::Approx(2.0f).margin(1e-3f));   // ratio 2:1
}

// 12. AC-2: num_layers=0 → usa buffer_ base sem modificação (retrocompatibilidade)
TEST_CASE_METHOD(PadBankJuceFixture, "zero layers uses base buffer unchanged", "[padbank][layers]") {
    PadBank bank;
    load_dc_pad(bank, 0, 2000, 0.75f); // base buffer DC=0.75, sem camadas

    Scheduler sched;
    constexpr int block_size = 256;
    constexpr float k_cpan = 0.70710678f;

    VoicePool pool;
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, 1.0f), 0);
    std::vector<float> l(block_size, 0.0f), r(block_size, 0.0f);
    sched.process(midi, pool, bank, block_size);
    pool.process_all(l.data(), r.data(), block_size);

    REQUIRE(l[100] == Catch::Approx(0.75f * k_cpan).margin(1e-3f));
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
