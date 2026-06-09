#include "audio/lane_routing.h"
#include "audio/perf_state.h"
#include "audio/sequencer.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>

namespace {
constexpr double k_sr = 48000.0;
constexpr int k_block = 98304; // ~2.2 bars @120bpm — cobre os steps

TransportState mr_transport() {
    TransportState t{};
    t.is_playing = true;
    t.ppq_position = 0.0;
    t.bpm = 120.0;
    return t;
}

int count_on(const juce::MidiBuffer& m, int note, int channel) {
    int n = 0;
    for (const auto meta : m) {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getNoteNumber() == note && msg.getChannel() == channel)
            ++n;
    }
    return n;
}
int count_on_any(const juce::MidiBuffer& m, int note) {
    int n = 0;
    for (const auto meta : m)
        if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == note)
            ++n;
    return n;
}
int count_off(const juce::MidiBuffer& m, int note) {
    int n = 0;
    for (const auto meta : m)
        if (meta.getMessage().isNoteOff() && meta.getMessage().getNoteNumber() == note)
            ++n;
    return n;
}

StepPattern one_lane(int lane, int step, int note) {
    StepPattern p{};
    // limpa default kick na lane 0 se necessário
    for (int s : {0, 4, 8, 12})
        p.set_active(0, s, false);
    p.set_active(lane, step, true);
    p.set_note(lane, step, static_cast<uint8_t>(note));
    return p;
}

void run(Sequencer& seq, juce::MidiBuffer& internal, juce::MidiBuffer& ext, const LaneRouting* routing) {
    internal.clear();
    ext.clear();
    PLockBatch batch;
    seq.generate(mr_transport(), internal, k_block, k_sr, nullptr, nullptr, 0, nullptr, &batch, nullptr, routing, &ext);
}
} // namespace

// MR1 (AC-1/AC-6): routing=nullptr → tudo interno, EXT vazio (legado).
TEST_CASE("MR1 - null routing is legacy internal-only", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(one_lane(0, 0, 36));
    juce::MidiBuffer internal, ext;
    run(seq, internal, ext, nullptr);
    REQUIRE(count_on_any(internal, 36) > 0);
    REQUIRE(ext.getNumEvents() == 0);
}

// MR2 (AC-1): lane external → note-on no EXT, nada no interno p/ essa lane.
TEST_CASE("MR2 - external lane emits to EXT only", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(one_lane(0, 0, 40));
    LaneRouting r;
    r.mode[0] = LaneMode::external;
    r.channel[0] = 5;
    juce::MidiBuffer internal, ext;
    run(seq, internal, ext, &r);
    REQUIRE(count_on_any(ext, 40) > 0);
    REQUIRE(count_on_any(internal, 40) == 0);
}

// MR3 (AC-2): canal por lane.
TEST_CASE("MR3 - external lane uses its MIDI channel", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(one_lane(0, 0, 41));
    LaneRouting r;
    r.mode[0] = LaneMode::external;
    r.channel[0] = 10;
    juce::MidiBuffer internal, ext;
    run(seq, internal, ext, &r);
    REQUIRE(count_on(ext, 41, 10) > 0); // canal 10
    REQUIRE(count_on(ext, 41, 1) == 0);
}

// MR4 (AC-1): both → eventos nos dois buffers.
TEST_CASE("MR4 - both routes to internal and EXT", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    Sequencer seq;
    seq.set_pattern(one_lane(0, 0, 42));
    LaneRouting r;
    r.mode[0] = LaneMode::both;
    r.channel[0] = 3;
    juce::MidiBuffer internal, ext;
    run(seq, internal, ext, &r);
    REQUIRE(count_on_any(internal, 42) > 0);
    REQUIRE(count_on(ext, 42, 3) > 0);
}

// MR5 (AC-3): híbrido — lane0 INT (36) + lane1 EXT ch10 (38) no mesmo padrão.
TEST_CASE("MR5 - hybrid internal and external lanes in one pattern", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    for (int s : {0, 4, 8, 12})
        p.set_active(0, s, false);
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36); // lane 0 INT
    p.set_active(1, 0, true);
    p.set_note(1, 0, 38); // lane 1 EXT
    Sequencer seq;
    seq.set_pattern(p);
    LaneRouting r;
    r.mode[1] = LaneMode::external;
    r.channel[1] = 10;
    juce::MidiBuffer internal, ext;
    run(seq, internal, ext, &r);
    REQUIRE(count_on_any(internal, 36) > 0); // lane 0 interno
    REQUIRE(count_on(ext, 38, 10) > 0);      // lane 1 externo ch10
    REQUIRE(count_on_any(internal, 38) == 0);
}

// MR6 (AC-4): lane EXT mutada → 0 note-on EXT; note-off EXT ainda emitido.
TEST_CASE("MR6 - muted external lane suppresses note-on but emits note-off", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    for (int s : {0, 4, 8, 12})
        p.set_active(0, s, false);
    p.set_active(0, 0, true);
    p.set_note(0, 0, 44);
    p.set_active(0, 1, true);
    p.set_note(0, 1, 44);
    Sequencer seq;
    seq.set_pattern(p);
    LaneRouting r;
    r.mode[0] = LaneMode::external;
    r.channel[0] = 7;
    PerfState perf;
    perf.mute[0] = true;
    juce::MidiBuffer internal, ext;
    ext.clear();
    internal.clear();
    PLockBatch batch;
    seq.generate(mr_transport(), internal, k_block, k_sr, nullptr, nullptr, 0, nullptr, &batch, &perf, &r, &ext);
    REQUIRE(count_on_any(ext, 44) == 0); // muted → sem note-on EXT
    REQUIRE(count_off(ext, 44) > 0);     // note-off EXT ainda emitido (sem nota presa no hardware)
}

// MR9 (AC-1/SR1): note-off EXT usa a nota do padrão no prev_step (não o tracker INT vazio).
TEST_CASE("MR9 - external note-off uses pattern note not empty INT tracker", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    StepPattern p{};
    for (int s : {0, 4, 8, 12})
        p.set_active(0, s, false);
    p.set_active(0, 0, true);
    p.set_note(0, 0, 50); // nota A
    p.set_active(0, 1, true);
    p.set_note(0, 1, 52); // nota B
    Sequencer seq;
    seq.set_pattern(p);
    LaneRouting r;
    r.mode[0] = LaneMode::external; // external-only → tracker INT vazio
    r.channel[0] = 1;
    juce::MidiBuffer internal, ext;
    run(seq, internal, ext, &r);
    // step 1 dispara → note-off do prev_step (0) usa a nota 50 do padrão; não 0.
    REQUIRE(count_off(ext, 50) > 0);
    REQUIRE(count_off(ext, 0) == 0);
}

// MR8 (AC-7, M1): stop-flush — borda toca→para emite All Notes Off no canal EXT.
namespace {
struct StopPlayHead : juce::AudioPlayHead {
    bool playing = true;
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override {
        juce::AudioPlayHead::PositionInfo info;
        info.setIsPlaying(playing);
        info.setBpm(120.0);
        info.setPpqPosition(0.0);
        return info;
    }
};
} // namespace

TEST_CASE("MR8 - transport stop flushes all-notes-off on EXT channel", "[midi_routing]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 512);
    processor.lane_routing_.mode[0] = LaneMode::external;
    processor.lane_routing_.channel[0] = 9;

    StopPlayHead ph;
    processor.setPlayHead(&ph);

    juce::MidiBuffer midi;
    // Bloco 1: tocando → was_playing_ = true.
    ph.playing = true;
    {
        juce::AudioBuffer<float> buf(2, 512);
        midi.clear();
        processor.processBlock(buf, midi);
    }
    // Bloco 2: parado → borda toca→para → All Notes Off (CC123) no canal 9.
    ph.playing = false;
    juce::AudioBuffer<float> buf(2, 512);
    midi.clear();
    processor.processBlock(buf, midi);

    bool found_panic = false;
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isControllerOfType(123) && m.getChannel() == 9)
            found_panic = true;
    }
    REQUIRE(found_panic); // sem nota presa no hardware ao parar
}
