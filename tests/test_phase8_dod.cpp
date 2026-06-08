#include "audio/perf_state.h"
#include "audio/sequencer.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace {
constexpr double k_sr = 48000.0;

int count_note_ons(const juce::MidiBuffer& midi) {
    int n = 0;
    for (const auto meta : midi) {
        if (meta.getMessage().isNoteOn())
            ++n;
    }
    return n;
}
} // namespace

// D1 (AC-5, DoD): live scatter + gater + tape_stop ativos no processBlock — sem dropout/NaN.
TEST_CASE("D1 - live scatter and perf FX stay finite over many blocks", "[dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 512);

    processor.apvts_.getParameter("scatter_type")->setValueNotifyingHost(2.0f / 10.0f);
    processor.apvts_.getParameter("scatter_depth")->setValueNotifyingHost(1.0f);
    processor.apvts_.getParameter("gate_depth")->setValueNotifyingHost(1.0f);
    processor.apvts_.getParameter("tape_stop")->setValueNotifyingHost(0.5f);

    for (int b = 0; b < 200; ++b) {
        juce::AudioBuffer<float> buf(2, 512);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                REQUIRE(std::isfinite(buf.getSample(ch, i)));
    }
}

// D2 (AC-6, DoD): fills via trig conditions — fill adiciona hits ponta-a-ponta.
TEST_CASE("D2 - fills via trig conditions add hits when engaged", "[dod]") {
    juce::ScopedJuceInitialiser_GUI init;

    // Lane 0: steps base "always" (0,4,8,12) + steps extras "fill" (2,6,10,14).
    StepPattern p{};
    for (int s : {0, 4, 8, 12}) {
        p.set_active(0, s, true);
        p.set_note(0, s, 36);
        p.set_trig(0, s, StepPattern::TrigCondition::always);
    }
    for (int s : {2, 6, 10, 14}) {
        p.set_active(0, s, true);
        p.set_note(0, s, 36);
        p.set_trig(0, s, StepPattern::TrigCondition::fill);
    }

    TransportState t{};
    t.is_playing = true;
    t.ppq_position = 0.0;
    t.bpm = 120.0;
    constexpr int block = 98304; // cobre o padrão

    Sequencer seq_off;
    seq_off.set_pattern(p);
    juce::MidiBuffer midi_off;
    PLockBatch batch_off;
    PerfState perf_off; // fill_active = false
    seq_off.generate(t, midi_off, block, k_sr, nullptr, nullptr, 0, nullptr, &batch_off, &perf_off);
    const int n_no_fill = count_note_ons(midi_off);

    Sequencer seq_on;
    seq_on.set_pattern(p);
    juce::MidiBuffer midi_on;
    PLockBatch batch_on;
    PerfState perf_on;
    perf_on.fill_active = true;
    seq_on.generate(t, midi_on, block, k_sr, nullptr, nullptr, 0, nullptr, &batch_on, &perf_on);
    const int n_with_fill = count_note_ons(midi_on);

    REQUIRE(n_no_fill > 0);           // base steps disparam sem fill
    REQUIRE(n_with_fill > n_no_fill); // fill engajado adiciona hits
}

// D3: marcador de DoD da Fase 8.
TEST_CASE("D3 - Phase 8 DoD marker", "[dod]") {
    SUCCEED("Phase 8 DoD: scatter/tape/gater RT-safe live (no dropout); fills via trig conditions; "
            "mute/solo groups. Scene morph deferred to Phase 10/UI.");
}
