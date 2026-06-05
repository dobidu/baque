// DoD Phase 3: pattern loops (03-01), global swing (03-02), pattern switch (03-02), no stuck notes

#include "audio/note_tracker.h"
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

// ─── Testes 03-02: swing, troca de padrão, NoteTracker ───────────────────────

// 7. Swing atrasa steps ímpares
TEST_CASE("swing delays even steps", "[sequencer][swing]") {
    StepClock clock;
    clock.set_swing(0.67f); // MPC default

    const double bpm = 120.0;
    const double sr = 44100.0;
    const double ppq = 0.0;

    // Step 0 (ímpar por índice, on-beat): sem swing → offset do grid
    const int offset_step0 = clock.sample_offset_of_step_start(ppq, bpm, sr, 0);

    // Step 1 (par por índice, off-beat): com swing
    const int offset_step1 = clock.sample_offset_of_step_start(ppq, bpm, sr, 1);

    // Step 1 deve ser step_duration + swing_offset
    // step_duration = 60*44100/(120*4) = 5512 samples; swing_offset = (0.67-0.5)*2*5512 = 1874
    REQUIRE(offset_step1 > offset_step0);
    const int expected_step1 = 5512 + 1874; // grade + swing ≈ 7386
    REQUIRE(offset_step1 == Catch::Approx(expected_step1).margin(5));
}

// 8. Troca de padrão ocorre na transição 15→0
TEST_CASE("pattern switch happens at step 0", "[sequencer][pattern-switch]") {
    Sequencer seq;

    // Padrão A: step 0 ativo na lane 0
    StepPattern pattern_a;
    for (int s = 0; s < 16; ++s)
        pattern_a.set_active(0, s, false);
    pattern_a.set_active(0, 0, true);
    seq.pattern() = pattern_a;

    // Padrão B: step 8 ativo na lane 0
    StepPattern pattern_b;
    for (int s = 0; s < 16; ++s)
        pattern_b.set_active(0, s, false);
    pattern_b.set_active(0, 8, true);

    // Agenda troca
    seq.set_next_pattern(pattern_b);

    // Simula: avança até step 15 (sem gerar step 0 ainda)
    TransportState t;
    t.is_playing = true;
    t.bpm = 120.0;
    // ppq = 3.75 → step 15; ppq_block_end = 3.75 + delta (1 bloco de 512 amostras)
    t.ppq_position = 3.74; // gera step 15 dentro do bloco
    juce::MidiBuffer midi;
    seq.generate(t, midi, 512, 44100.0);
    midi.clear();

    // Agora step 0 da próxima volta — deve ativar padrão B
    t.ppq_position = 3.99; // logo antes de 4.0 = step 0 do próximo ciclo
    seq.generate(t, midi, 512, 44100.0);

    // Padrão B: step 8 ativo → step 0 inativo → nenhum note-on de step 0 no próximo ciclo
    // Mas a troca ocorre na transição 15→0, então o step 0 deste bloco usa padrão B
    // (step 0 inativo em B → nenhum note-on aqui)
    bool has_note_on = false;
    for (const auto& meta : midi) {
        if (meta.getMessage().isNoteOn())
            has_note_on = true;
    }
    // Padrão B tem step 0 inativo → nenhum note-on no step 0
    REQUIRE(!has_note_on);
}

// 9. NoteTracker registra e retorna nota correta
TEST_CASE("note tracker records and returns correct note", "[sequencer][notetracker]") {
    NoteTracker tracker;
    tracker.note_triggered(3, 42);
    REQUIRE(tracker.get_active_note(3) == 42);
    tracker.note_triggered(3, 57);
    REQUIRE(tracker.get_active_note(3) == 57);
    tracker.note_triggered(0, 36);
    REQUIRE(tracker.get_active_note(0) == 36);
    REQUIRE(tracker.get_active_note(3) == 57); // lane 3 inalterada
}

// 10. Sequencer: note-off usa NoteTracker, nao nota hardcoded
TEST_CASE("sequencer note-off uses tracker note not fixed fallback", "[sequencer][notetracker]") {
    Sequencer seq;

    // Padrão: step 0 ativo na lane 0, nota 55
    StepPattern p;
    for (int s = 0; s < 16; ++s)
        p.set_active(0, s, false);
    p.set_active(0, 0, true);
    p.set_note(0, 0, 55);
    seq.pattern() = p;

    // Bloco 1: ppq logo antes do step 0 → dispara note-on 55
    TransportState t;
    t.is_playing = true;
    t.bpm = 120.0;
    t.ppq_position = 3.99; // gera step 0 (próximo ciclo) dentro do bloco
    juce::MidiBuffer midi1;
    seq.generate(t, midi1, 512, 44100.0);

    bool note_on_55 = false;
    for (const auto& meta : midi1) {
        if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == 55)
            note_on_55 = true;
    }
    REQUIRE(note_on_55);

    // Bloco 2: ppq logo antes do step 1 → dispara note-off 55 (via NoteTracker)
    t.ppq_position = 0.249; // step 1 boundary próximo
    juce::MidiBuffer midi2;
    seq.generate(t, midi2, 512, 44100.0);

    bool note_off_55 = false;
    for (const auto& meta : midi2) {
        if (meta.getMessage().isNoteOff() && meta.getMessage().getNoteNumber() == 55)
            note_off_55 = true;
    }
    REQUIRE(note_off_55);
}
