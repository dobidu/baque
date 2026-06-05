#include "audio/sequencer.h"
#include "audio/step_clock.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// 1. StepClock: step correto a partir do ppq
TEST_CASE("step clock returns correct step from ppq", "[sequencer][clock]") {
    StepClock clock;
    REQUIRE(clock.current_step(0.0) == 0);
    REQUIRE(clock.current_step(0.25) == 1);
    REQUIRE(clock.current_step(0.5) == 2);
    REQUIRE(clock.current_step(3.75) == 15);
    REQUIRE(clock.current_step(4.0) == 0);     // loop-around
    REQUIRE(clock.current_step(0.24999) == 0); // abaixo da fronteira
    REQUIRE(clock.current_step(8.0) == 0);     // dois ciclos completos
}

// 2. StepClock: offset de sample dentro do bloco
TEST_CASE("step clock sample offset within block", "[sequencer][clock]") {
    StepClock clock;

    // Caso A: ppq=0.0, step 1 fica 5512 samples longe → fora do bloco de 512
    const int offset_a = clock.sample_offset_of_step_start(0.0, 120.0, 44100.0, 1);
    REQUIRE(offset_a >= 512); // fora do bloco

    // Caso B: ppq=0.249, step 1 (ppq 0.25) fica a ~22 samples
    // delta_ppq = 0.001; delta_seconds = 0.001 * (60/120) = 0.0005s; delta_samples ≈ 22
    const int offset_b = clock.sample_offset_of_step_start(0.249, 120.0, 44100.0, 1);
    REQUIRE(offset_b < 100); // dentro das primeiras 100 amostras
    REQUIRE(offset_b >= 0);
}

// 3. StepPattern: padrão padrão tem bumbo nos tempos
TEST_CASE("step pattern default has kick on beats", "[sequencer][pattern]") {
    StepPattern p;
    REQUIRE(p.is_active(0, 0));
    REQUIRE(p.is_active(0, 4));
    REQUIRE(p.is_active(0, 8));
    REQUIRE(p.is_active(0, 12));
    REQUIRE(!p.is_active(0, 1));
    REQUIRE(!p.is_active(0, 2));
    REQUIRE(!p.is_active(0, 3));
}

// 4. StepPattern: set e get
TEST_CASE("step pattern set and get", "[sequencer][pattern]") {
    StepPattern p;
    p.set_active(0, 7, true);
    REQUIRE(p.is_active(0, 7));
    p.set_active(0, 7, false);
    REQUIRE(!p.is_active(0, 7));

    p.set_note(2, 5, 42);
    REQUIRE(p.get_note(2, 5) == 42);
}

// 5. Sequencer: nenhum evento quando não está tocando
TEST_CASE("sequencer generates no events when not playing", "[sequencer]") {
    Sequencer seq;
    TransportState transport;
    transport.is_playing = false;

    juce::MidiBuffer midi;
    seq.generate(transport, midi, 512, 44100.0);
    REQUIRE(midi.isEmpty());
}

// 6. Sequencer: gera note-on na fronteira do step
TEST_CASE("sequencer generates note-on at step boundary", "[sequencer]") {
    Sequencer seq;
    TransportState transport;
    transport.is_playing = true;
    transport.bpm = 120.0;
    transport.ppq_position = 0.249; // logo antes do step 1 (0.25 ppq)

    juce::MidiBuffer midi;
    seq.generate(transport, midi, 512, 44100.0);

    // Deve ter pelo menos um evento (note-on do step 1, se step 1 ativo, ou note-off do step 0)
    // Padrão padrão: step 0 ativo na lane 0, step 1 inativo
    // → note-off do step 0 deve aparecer no step 1
    bool has_event = false;
    int first_sample_pos = 512;
    for (const auto& meta : midi) {
        has_event = true;
        first_sample_pos = std::min(first_sample_pos, meta.samplePosition);
    }
    REQUIRE(has_event);
    REQUIRE(first_sample_pos < 100); // samplePosition < 100 (≈22 amostras após ppq 0.249)
}
