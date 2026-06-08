#include "audio/fx_params.h"
#include "audio/plock_pattern.h"
#include "audio/sequencer.h"
#include "audio/step_pattern.h"
#include "audio/transport_state.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

struct PLFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

static TransportState pl_transport(double ppq = 0.0, double bpm = 120.0) {
    TransportState t{};
    t.is_playing = true;
    t.ppq_position = ppq;
    t.bpm = bpm;
    return t;
}

static StepPattern pl_all_steps() {
    StepPattern p{};
    for (int s = 0; s < StepPattern::k_num_steps; ++s) {
        p.set_active(0, s, true);
        p.set_note(0, s, 36);
    }
    return p;
}

// PL1: step with filter_cutoff p-lock emits correct event
TEST_CASE("PL1 - step with filter_cutoff plock emits event", "[plock]") {
    PLFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    PLockPattern pp{};
    pp.enabled = true;
    const int ci = static_cast<int>(PLockParam::filter_cutoff);
    pp.steps[0].active[ci] = true;
    pp.steps[0].values[ci] = 800.0f;

    Sequencer seq;
    seq.set_pattern(pl_all_steps());
    juce::MidiBuffer buf;
    PLockBatch batch;
    seq.generate(pl_transport(0.0), buf, block, sr, nullptr, nullptr, 0, &pp, &batch);

    REQUIRE(batch.count >= 1);
    bool found = false;
    for (int i = 0; i < batch.count; ++i) {
        if (batch.events[i].param == PLockParam::filter_cutoff && batch.events[i].value == Catch::Approx(800.0f))
            found = true;
    }
    REQUIRE(found);
}

// PL2: step with no active p-locks emits nothing
TEST_CASE("PL2 - step with no active plocks emits nothing", "[plock]") {
    PLFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    PLockPattern pp{};
    pp.enabled = true;
    // all active[i] default false

    Sequencer seq;
    seq.set_pattern(pl_all_steps());
    juce::MidiBuffer buf;
    PLockBatch batch;
    seq.generate(pl_transport(0.0), buf, block, sr, nullptr, nullptr, 0, &pp, &batch);

    REQUIRE(batch.count == 0);
}

// PL3: step with 2 p-locks emits both events
TEST_CASE("PL3 - step with two plocks emits both events", "[plock]") {
    PLFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    PLockPattern pp{};
    pp.enabled = true;
    const int ci = static_cast<int>(PLockParam::filter_cutoff);
    const int ri = static_cast<int>(PLockParam::reverb_mix);
    pp.steps[0].active[ci] = true;
    pp.steps[0].values[ci] = 400.0f;
    pp.steps[0].active[ri] = true;
    pp.steps[0].values[ri] = 0.5f;

    Sequencer seq;
    seq.set_pattern(pl_all_steps());
    juce::MidiBuffer buf;
    PLockBatch batch;
    seq.generate(pl_transport(0.0), buf, block, sr, nullptr, nullptr, 0, &pp, &batch);

    REQUIRE(batch.count == 2);
}

// PL4: plock_pattern=nullptr — backward compatible, empty batch
TEST_CASE("PL4 - plock_pattern nullptr backward compatible", "[plock]") {
    PLFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    Sequencer seq;
    seq.set_pattern(pl_all_steps());
    juce::MidiBuffer buf;
    PLockBatch batch;
    seq.generate(pl_transport(0.0), buf, block, sr, nullptr, nullptr, 0, nullptr, &batch);

    REQUIRE(batch.count == 0);
}

// PL5: PLockPattern.enabled=false — no events even if steps have active p-locks
TEST_CASE("PL5 - disabled PLockPattern emits no events", "[plock]") {
    PLFixture f;
    const double sr = 44100.0;
    const int block = 8192;

    PLockPattern pp{};
    pp.enabled = false;
    pp.steps[0].active[static_cast<int>(PLockParam::filter_cutoff)] = true;
    pp.steps[0].values[static_cast<int>(PLockParam::filter_cutoff)] = 500.0f;

    Sequencer seq;
    seq.set_pattern(pl_all_steps());
    juce::MidiBuffer buf;
    PLockBatch batch;
    seq.generate(pl_transport(0.0), buf, block, sr, nullptr, nullptr, 0, &pp, &batch);

    REQUIRE(batch.count == 0);
}

// PL6: PLockParam enum count is 6; APVTS param presence covered by smoke test crash behavior.
// audit-SR1: BaqueProcessor cannot be instantiated in unit tests (requires full JUCE plugin
// infrastructure). AC-6 is implicitly verified by test_smoke.cpp processBlock: if any
// APVTS param ID is missing from create_parameter_layout(), getRawParameterValue() returns
// nullptr and the dereference crashes. Static check here confirms enum sanity.
TEST_CASE("PL6 - PLockParam enum count correct", "[plock]") {
    REQUIRE(k_plock_param_count == static_cast<int>(PLockParam::count));
    REQUIRE(k_plock_param_count == 15);
}

// PL7: multiple steps firing in same block accumulate all p-lock events
TEST_CASE("PL7 - multiple steps firing accumulate all plock events", "[plock]") {
    PLFixture f;
    const double sr = 44100.0;
    const int block = 98304; // ~2.2 bars at 120bpm → fires multiple steps

    PLockPattern pp{};
    pp.enabled = true;
    const int ci = static_cast<int>(PLockParam::filter_cutoff);
    pp.steps[0].active[ci] = true;
    pp.steps[0].values[ci] = 200.0f;
    pp.steps[4].active[ci] = true;
    pp.steps[4].values[ci] = 400.0f;

    Sequencer seq;
    seq.set_pattern(pl_all_steps());
    juce::MidiBuffer buf;
    PLockBatch batch;
    seq.generate(pl_transport(0.0), buf, block, sr, nullptr, nullptr, 0, &pp, &batch);

    REQUIRE(batch.count >= 2);
}

// PL8: PLockBatch overflow guard — never exceeds k_max even with 16×6=96 potential events
TEST_CASE("PL8 - PLockBatch overflow guard prevents buffer overrun", "[plock]") {
    PLFixture f;
    const double sr = 44100.0;
    const int block = 98304; // fires all 16 steps

    PLockPattern pp{};
    pp.enabled = true;
    for (int s = 0; s < PLockPattern::k_steps; ++s) {
        for (int pi = 0; pi < k_plock_param_count; ++pi) {
            pp.steps[s].active[pi] = true;
            pp.steps[s].values[pi] = static_cast<float>(s * 10 + pi);
        }
    }

    Sequencer seq;
    seq.set_pattern(pl_all_steps());
    juce::MidiBuffer buf;
    PLockBatch batch;
    seq.generate(pl_transport(0.0), buf, block, sr, nullptr, nullptr, 0, &pp, &batch);

    REQUIRE(batch.count <= PLockBatch::k_max);
}
