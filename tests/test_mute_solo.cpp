#include "audio/perf_state.h"
#include "audio/sequencer.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>

namespace {
constexpr double k_sr = 44100.0;
constexpr int k_block = 98304;

TransportState ms_transport() {
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

// Padrão de 2 lanes: lane 0 nota 36, lane 1 nota 38, ambas no step 0.
StepPattern two_lane_pattern() {
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36);
    p.set_active(1, 0, true);
    p.set_note(1, 0, 38);
    return p;
}

void run(Sequencer& seq, juce::MidiBuffer& out, const PerfState* perf) {
    out.clear();
    PLockBatch batch;
    seq.generate(ms_transport(), out, k_block, k_sr, nullptr, nullptr, 0, nullptr, &batch, perf);
}
} // namespace

// MS1 (AC-2): lane muted → 0 note-ons; note-off ainda emitido (sem nota presa).
TEST_CASE("MS1 - muted lane emits no note-on but still note-off", "[mute_solo]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(two_lane_pattern());
    juce::MidiBuffer midi;
    PerfState perf;
    perf.mute[1] = true; // muta lane 1 (nota 38)
    run(seq, midi, &perf);

    REQUIRE(count_note_ons_of(midi, 38) == 0); // muted → sem note-on
    REQUIRE(count_note_offs_of(midi, 38) > 0); // note-off ainda emitido (evita nota presa)
    REQUIRE(count_note_ons_of(midi, 36) > 0);  // lane 0 inalterada
}

// MS2 (AC-3): solo isola — só lanes soloed disparam.
TEST_CASE("MS2 - solo isolates soloed lanes", "[mute_solo]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(two_lane_pattern());
    juce::MidiBuffer midi;
    PerfState perf;
    perf.solo[1] = true; // solo lane 1
    run(seq, midi, &perf);

    REQUIRE(count_note_ons_of(midi, 38) > 0);  // soloed dispara
    REQUIRE(count_note_ons_of(midi, 36) == 0); // não-soloed suprimido
}

// MS3 (AC-3): lane soloed E muted → suprimida (mute vence).
TEST_CASE("MS3 - soloed and muted lane stays suppressed", "[mute_solo]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(two_lane_pattern());
    juce::MidiBuffer midi;
    PerfState perf;
    perf.solo[1] = true;
    perf.mute[1] = true; // soloed mas muted
    run(seq, midi, &perf);

    REQUIRE(count_note_ons_of(midi, 38) == 0); // mute vence
    REQUIRE(count_note_ons_of(midi, 36) == 0); // lane 0 não-soloed → suprimida
}

// MS4 (AC-4 compat): perf=nullptr → todas as lanes ativas disparam (legado).
TEST_CASE("MS4 - perf nullptr fires all active lanes", "[mute_solo]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(two_lane_pattern());
    juce::MidiBuffer midi;
    run(seq, midi, nullptr);

    REQUIRE(count_note_ons_of(midi, 36) > 0);
    REQUIRE(count_note_ons_of(midi, 38) > 0);
}
