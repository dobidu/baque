#include "audio/perf_state.h"
#include "audio/sequencer.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>

namespace {
constexpr double k_sr = 44100.0;
constexpr int k_block = 98304; // ~2.2 bars @120bpm — cobre todos os 16 steps

TransportState tf_transport() {
    TransportState t{};
    t.is_playing = true;
    t.ppq_position = 0.0;
    t.bpm = 120.0;
    return t;
}

int count_note_ons_of(const juce::MidiBuffer& midi, int note) {
    int n = 0;
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isNoteOn() && m.getNoteNumber() == note)
            ++n;
    }
    return n;
}

int count_note_offs_of(const juce::MidiBuffer& midi, int note) {
    int n = 0;
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isNoteOff() && m.getNoteNumber() == note)
            ++n;
    }
    return n;
}

// Gera num bloco e retorna o MidiBuffer (Sequencer fresco por chamada).
void run(Sequencer& seq, juce::MidiBuffer& out, const PerfState* perf) {
    out.clear();
    PLockBatch batch;
    seq.generate(tf_transport(), out, k_block, k_sr, nullptr, nullptr, 0, nullptr, &batch, perf);
}
} // namespace

// TF1 (AC-1): step always dispara com fill on E off.
TEST_CASE("TF1 - always trig fires regardless of fill", "[trig]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36);
    p.set_trig(0, 0, StepPattern::TrigCondition::always);

    Sequencer seq;
    seq.set_pattern(p);
    juce::MidiBuffer midi;

    PerfState perf_on;
    perf_on.fill_active = true;
    run(seq, midi, &perf_on);
    REQUIRE(count_note_ons_of(midi, 36) > 0);

    Sequencer seq2;
    seq2.set_pattern(p);
    PerfState perf_off;
    perf_off.fill_active = false;
    run(seq2, midi, &perf_off);
    REQUIRE(count_note_ons_of(midi, 36) > 0);
}

// TF2 (AC-1): step fill dispara só com fill_active=true.
TEST_CASE("TF2 - fill trig fires only when fill active", "[trig]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 40);
    p.set_trig(0, 0, StepPattern::TrigCondition::fill);

    Sequencer seq;
    seq.set_pattern(p);
    juce::MidiBuffer midi;
    PerfState perf_off;
    run(seq, midi, &perf_off);
    REQUIRE(count_note_ons_of(midi, 40) == 0); // sem fill → suprimido

    Sequencer seq2;
    seq2.set_pattern(p);
    PerfState perf_on;
    perf_on.fill_active = true;
    run(seq2, midi, &perf_on);
    REQUIRE(count_note_ons_of(midi, 40) > 0); // com fill → dispara
}

// TF3 (AC-1): step not_fill dispara só com fill_active=false.
TEST_CASE("TF3 - not_fill trig fires only when fill inactive", "[trig]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 41);
    p.set_trig(0, 0, StepPattern::TrigCondition::not_fill);

    Sequencer seq;
    seq.set_pattern(p);
    juce::MidiBuffer midi;
    PerfState perf_off;
    run(seq, midi, &perf_off);
    REQUIRE(count_note_ons_of(midi, 41) > 0); // sem fill → dispara

    Sequencer seq2;
    seq2.set_pattern(p);
    PerfState perf_on;
    perf_on.fill_active = true;
    run(seq2, midi, &perf_on);
    REQUIRE(count_note_ons_of(midi, 41) == 0); // com fill → suprimido
}

// TF4 (AC-1 compat): perf=nullptr → todos os steps ativos (always) disparam (legado).
TEST_CASE("TF4 - perf nullptr is legacy behavior", "[trig]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36); // default trig = always

    Sequencer seq;
    seq.set_pattern(p);
    juce::MidiBuffer midi;
    run(seq, midi, nullptr);
    REQUIRE(count_note_ons_of(midi, 36) > 0);
}

// TF5 (AC-7, audit M1/SR2): step suprimido NÃO corrompe o NoteTracker.
TEST_CASE("TF5 - suppressed step does not poison NoteTracker", "[trig]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36); // always (nota A)
    p.set_active(0, 1, true);
    p.set_note(0, 1, 38);
    p.set_trig(0, 1, StepPattern::TrigCondition::fill); // fill (nota B), suprimido sem fill

    Sequencer seq;
    seq.set_pattern(p);
    juce::MidiBuffer midi;
    PerfState perf_off; // fill_active = false → step 1 suprimido
    run(seq, midi, &perf_off);

    REQUIRE(count_note_ons_of(midi, 38) == 0);  // nota B nunca dispara
    REQUIRE(count_note_offs_of(midi, 38) == 0); // e nunca gera note-off (não entrou no tracker)
    REQUIRE(count_note_ons_of(midi, 36) > 0);   // nota A dispara
    REQUIRE(count_note_offs_of(midi, 36) > 0);  // e tem note-off (sem nota presa)
}
