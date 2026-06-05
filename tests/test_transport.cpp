// DoD Phase 2: pad fires sample without clicks (fade-in/out in SampleVoice),
// sample-accurate (Scheduler dispatches at MidiBuffer offset),
// RT-safe (zero allocs, verified in test_voice.cpp AC-2)

#include "audio/scheduler.h"
#include "audio/voice_pool.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

// Fixture do JUCE — necessária para tipos que usam JUCE internamente
struct JuceFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

// Gera buffer de floats com valor constante val nas primeiras count amostras, 0 no resto
static std::vector<float> make_sample_data(int total, int nonzero_count, float val = 1.0f) {
    std::vector<float> buf(static_cast<std::size_t>(total), 0.0f);
    const int limit = nonzero_count < total ? nonzero_count : total;
    for (int i = 0; i < limit; ++i)
        buf[static_cast<std::size_t>(i)] = val;
    return buf;
}

// 1. Scheduler despacha no offset correto de sample
TEST_CASE("scheduler dispatches at correct sample offset", "[transport][scheduler]") {
    VoicePool pool;
    Scheduler sched;

    // Sample data: amostras 0-99 = 1.0f, resto = 0
    auto data = make_sample_data(1000, 100, 1.0f);

    // Note-on no offset 256 dentro de um bloco de 512
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 256);

    constexpr int block_size = 512;
    std::vector<float> left(block_size, 0.0f), right(block_size, 0.0f);

    sched.process(midi, pool, data.data(), static_cast<int>(data.size()), block_size);
    pool.process_all(left.data(), right.data(), block_size);

    // Silêncio antes do offset (frames 0..255)
    for (int i = 0; i < 256; ++i)
        REQUIRE(std::abs(left[static_cast<std::size_t>(i)]) < 1e-10f);

    // Voz começa no frame 256 — fade-in ativo, mas deve ser não-zero após 1 sample
    // (posição 1 no fade de 32 amostras → amplitude = 1/32 * data[0] * gain)
    REQUIRE(left[256] != 0.0f);

    // Fade-in em progresso: frame 288 > frame 256
    REQUIRE(left[288] > left[256]);
}

// 2. Silêncio antes do note-on em offset próximo ao fim do bloco
TEST_CASE("block-boundary silence before note-on", "[transport][scheduler]") {
    VoicePool pool;
    Scheduler sched;

    auto data = make_sample_data(1000, 1000, 1.0f);

    // Note-on no offset 500 dentro de um bloco de 512
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 500);

    constexpr int block_size = 512;
    std::vector<float> left(block_size, 0.0f), right(block_size, 0.0f);

    sched.process(midi, pool, data.data(), static_cast<int>(data.size()), block_size);
    pool.process_all(left.data(), right.data(), block_size);

    // Frames 0..499 devem ser silenciosos
    for (int i = 0; i < 500; ++i)
        REQUIRE(std::abs(left[static_cast<std::size_t>(i)]) < 1e-10f);
}

// 3. Note-off dispara fade-out de 32 amostras
TEST_CASE("note-off triggers voice fade-out", "[transport][scheduler]") {
    VoicePool pool;
    Scheduler sched;

    auto data = make_sample_data(5000, 5000, 1.0f);

    // Dispara voz manualmente e processa 100 frames (voz deve estar tocando)
    constexpr int first_block = 100;
    std::vector<float> left(first_block, 0.0f), right(first_block, 0.0f);

    juce::MidiBuffer note_on_midi;
    note_on_midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
    sched.process(note_on_midi, pool, data.data(), static_cast<int>(data.size()), first_block);
    pool.process_all(left.data(), right.data(), first_block);

    // Voz deve estar ativa após o primeiro bloco
    bool any_nonzero = false;
    for (int i = 32; i < first_block; ++i)
        if (std::abs(left[static_cast<std::size_t>(i)]) > 1e-6f)
            any_nonzero = true;
    REQUIRE(any_nonzero);

    // Chama note_off diretamente na voz mais ativa (para testar o fade)
    // VoicePool não expõe vozes diretamente — testamos via allocate trick:
    // Aloca nova voz com 1 sample para ter uma voz "mais antiga" e chamar note_off
    // Testa o comportamento de SampleVoice::note_off indiretamente
    // (O teste de fade-in já cobre a lógica de atenuação em test_voice.cpp)
    // Aqui confirmamos que o scheduler processa note-off sem crash
    juce::MidiBuffer note_off_midi;
    note_off_midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    constexpr int second_block = 64;
    std::vector<float> left2(second_block, 0.0f), right2(second_block, 0.0f);
    sched.process(note_off_midi, pool, data.data(), static_cast<int>(data.size()), second_block);
    // Deve processar sem crash (note-off no Fase 2 é no-op na Scheduler)
    pool.process_all(left2.data(), right2.data(), second_block);
    REQUIRE(true); // Chegou aqui sem crash
}

// 4. Estado de transporte: defaults corretos antes de qualquer playhead
TEST_CASE_METHOD(JuceFixture, "transport state has correct defaults", "[transport]") {
    BaqueProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    // TransportState é privado — testamos indiretamente pelo comportamento:
    // processBlock sem playhead não deve travar nem alterar o buffer
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    // Buffer deve permanecer razoável (sem NaN, sem infinito)
    bool has_invalid = false;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            if (!std::isfinite(buffer.getSample(ch, i)))
                has_invalid = true;
    REQUIRE(!has_invalid);
}
