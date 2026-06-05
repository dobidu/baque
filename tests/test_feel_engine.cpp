#include "audio/feel_engine.h"
#include "audio/feel_pattern.h"
#include "audio/sequencer.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

struct FeelJuceFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

static TransportState make_transport(double ppq = 0.0, double bpm = 120.0) {
    TransportState t{};
    t.is_playing = true;
    t.ppq_position = ppq;
    t.bpm = bpm;
    return t;
}

static StepPattern single_step_pattern() {
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36);
    return p;
}

static StepPattern all_steps_pattern() {
    StepPattern p{};
    for (int s = 0; s < StepPattern::k_num_steps; ++s) {
        p.set_active(0, s, true);
        p.set_note(0, s, 36);
    }
    return p;
}

TEST_CASE("FE1 - positive timing offset shifts note forward", "[feel]") {
    FeelJuceFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    FeelPattern feel{};
    feel.enabled = true;
    feel.steps[0].timing_ms = 25.0f;

    FeelEngine engine;
    Sequencer seq;
    seq.set_pattern(single_step_pattern());

    juce::MidiBuffer buf;
    seq.generate(make_transport(0.0), buf, block, sr, &feel, &engine, 0);

    int note_on_pos = -1;
    for (const auto meta : buf) {
        if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == 36)
            note_on_pos = meta.samplePosition;
    }
    REQUIRE(note_on_pos >= 0);
    const int expected = static_cast<int>(std::round(25.0 * sr / 1000.0));
    REQUIRE(note_on_pos >= expected - 2);
    REQUIRE(note_on_pos <= expected + 2);
}

TEST_CASE("FE2 - negative offset clamps to 0", "[feel]") {
    FeelJuceFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    FeelPattern feel{};
    feel.enabled = true;
    feel.steps[0].timing_ms = -50.0f;

    FeelEngine engine;
    Sequencer seq;
    seq.set_pattern(single_step_pattern());

    juce::MidiBuffer buf;
    seq.generate(make_transport(0.0), buf, block, sr, &feel, &engine, 0);

    for (const auto meta : buf) {
        if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == 36)
            REQUIRE(meta.samplePosition == 0);
    }
}

TEST_CASE("FE3 - beyond-block offset defers note", "[feel]") {
    FeelJuceFixture f;
    const double sr = 44100.0;
    const int block = 4096;

    FeelPattern feel{};
    feel.enabled = true;
    feel.steps[0].timing_ms = 150.0f; // 6615 samples at 44100 — beyond 4096

    FeelEngine engine;
    Sequencer seq;
    seq.set_pattern(single_step_pattern());

    juce::MidiBuffer buf;
    seq.generate(make_transport(0.0), buf, block, sr, &feel, &engine, 0);

    bool found = false;
    for (const auto meta : buf) {
        if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == 36)
            found = true;
    }
    REQUIRE_FALSE(found);
    REQUIRE(engine.deferred_count() >= 1);

    // Verify deferred entry is the note-on for note 36 — flush into next block.
    // target = 0 + 0 + 6615 = 6615; block [4096, 12288) covers it.
    juce::MidiBuffer next_buf;
    engine.flush_deferred(next_buf, 4096, 8192);
    bool deferred_note_found = false;
    for (const auto meta : next_buf) {
        if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == 36)
            deferred_note_found = true;
    }
    REQUIRE(deferred_note_found);
}

TEST_CASE("FE4 - deferred note fires in next block at correct position", "[feel]") {
    FeelJuceFixture f;
    FeelEngine engine;

    juce::MidiMessage test_msg = juce::MidiMessage::noteOn(1, 60, static_cast<uint8_t>(100));
    const int64_t target = 10000;
    engine.defer(test_msg, target);

    // Block [8192, 12288) should contain target=10000 at pos 10000-8192=1808
    juce::MidiBuffer buf;
    engine.flush_deferred(buf, 8192, 4096);

    REQUIRE(engine.deferred_count() == 0);
    int found_pos = -1;
    for (const auto meta : buf) {
        if (meta.getMessage().isNoteOn())
            found_pos = meta.samplePosition;
    }
    REQUIRE(found_pos == 1808);
}

TEST_CASE("FE5 - feel disabled leaves output unchanged", "[feel]") {
    FeelJuceFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    Sequencer seq_baseline;
    seq_baseline.set_pattern(single_step_pattern());
    juce::MidiBuffer buf_baseline;
    seq_baseline.generate(make_transport(0.0), buf_baseline, block, sr);

    FeelPattern feel{};
    feel.enabled = false;
    feel.steps[0].timing_ms = 99.0f; // would shift if enabled

    FeelEngine engine;
    Sequencer seq_feel;
    seq_feel.set_pattern(single_step_pattern());
    juce::MidiBuffer buf_feel;
    seq_feel.generate(make_transport(0.0), buf_feel, block, sr, &feel, &engine, 0);

    auto collect_positions = [](const juce::MidiBuffer& b) {
        std::vector<int> positions;
        for (const auto meta : b)
            positions.push_back(meta.samplePosition);
        return positions;
    };
    REQUIRE(collect_positions(buf_baseline) == collect_positions(buf_feel));
}

TEST_CASE("FE6 - vel_scale applied and clamped", "[feel]") {
    FeelJuceFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    auto check_vel = [&](float scale, uint8_t expected) {
        FeelPattern feel{};
        feel.enabled = true;
        feel.steps[0].vel_scale = scale;

        FeelEngine engine;
        Sequencer seq;
        seq.set_pattern(single_step_pattern());
        juce::MidiBuffer buf;
        seq.generate(make_transport(0.0), buf, block, sr, &feel, &engine, 0);

        for (const auto meta : buf) {
            const auto& msg = meta.getMessage();
            if (msg.isNoteOn() && msg.getNoteNumber() == 36)
                REQUIRE(msg.getVelocity() == expected);
        }
    };

    check_vel(0.5f, 50);  // 100 * 0.5 = 50
    check_vel(2.0f, 127); // 100 * 2.0 = 200 -> clamped 127
    check_vel(0.0f, 1);   // 100 * 0.0 = 0 -> clamped 1
}

TEST_CASE("FE7 - all 16 steps with distinct offsets fire at correct positions", "[feel]") {
    FeelJuceFixture f;
    const double sr = 44100.0;
    const double bpm = 120.0;
    // At 120 BPM/44100Hz: 1 step = sr*60/bpm/4 = 5512.5 samples. Full pattern ~88200; use 98304.
    const int block = 98304;

    FeelPattern feel{};
    feel.enabled = true;
    for (int i = 0; i < 16; ++i)
        feel.steps[i].timing_ms = static_cast<float>((i - 8) * 3); // -24ms to +21ms

    FeelEngine engine;
    Sequencer seq;
    seq.set_pattern(all_steps_pattern());

    juce::MidiBuffer buf;
    seq.generate(make_transport(0.0, bpm), buf, block, sr, &feel, &engine, 0);

    std::vector<int> actual_positions;
    for (const auto meta : buf) {
        if (meta.getMessage().isNoteOn() && meta.getMessage().getNoteNumber() == 36)
            actual_positions.push_back(meta.samplePosition);
    }

    REQUIRE(actual_positions.size() == 16);

    // Verify each note fires at expected position ±3 samples
    const double step_samples = sr * 60.0 / bpm / 4.0; // 5512.5 at 120BPM/44100Hz
    for (int i = 0; i < 16; ++i) {
        const double base = std::round(static_cast<double>(i) * step_samples);
        const int offset = FeelEngine::timing_ms_to_samples(feel.steps[i].timing_ms, sr);
        const int expected = static_cast<int>(base) + offset;
        const int clamped_expected = std::max(0, expected);
        REQUIRE(actual_positions[i] >= clamped_expected - 3);
        REQUIRE(actual_positions[i] <= clamped_expected + 3);
    }
}

TEST_CASE("FE8 - deferred queue overflow drops note", "[feel]") {
    FeelJuceFixture f;
    FeelEngine engine;

    juce::MidiMessage msg = juce::MidiMessage::noteOn(1, 60, static_cast<uint8_t>(100));

    for (int i = 0; i < FeelEngine::k_max_deferred; ++i)
        engine.defer(msg, static_cast<int64_t>(i) * 100);

    REQUIRE(engine.deferred_count() == FeelEngine::k_max_deferred);

    // One more — should be dropped; count stays at cap (no allocation, no exception)
    engine.defer(msg, 999999);
    REQUIRE(engine.deferred_count() == FeelEngine::k_max_deferred);
}
