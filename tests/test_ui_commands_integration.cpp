#include "audio/hardware_templates.h"
#include "audio/lane_routing.h"
#include "audio/step_pattern.h"
#include "audio/ui_command_queue.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {
constexpr double k_sr = 48000.0;
constexpr int k_block_s = 512;
constexpr int k_block_full = 98304; // covers all 16 steps at 120 bpm

struct UITestPlayHead : juce::AudioPlayHead {
    bool playing = true;
    double ppq = 0.0;
    double bpm = 120.0;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo info;
        info.setIsPlaying(playing);
        info.setBpm(bpm);
        info.setPpqPosition(ppq);
        return info;
    }
};

// Default pattern: lane 0, step 0 active, note 36 (everything else cleared).
StepPattern ui_pattern_lane0() {
    StepPattern p{};
    for (int s : {0, 4, 8, 12})
        p.set_active(0, s, false); // clear default kicks
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36);
    return p;
}

// Pattern with all steps on lane 0 inactive.
StepPattern all_inactive_pattern() {
    StepPattern p{};
    for (int s : {0, 4, 8, 12})
        p.set_active(0, s, false);
    return p;
}

// Full block with fixed PlayHead; returns midi output.
juce::MidiBuffer run_full(BaqueProcessor& proc) {
    juce::AudioBuffer<float> buf(2, k_block_full);
    buf.clear();
    juce::MidiBuffer midi;
    proc.processBlock(buf, midi);
    return midi;
}

int count_note_on(const juce::MidiBuffer& m, int note, int ch) {
    int n = 0;
    for (const auto meta : m) {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getNoteNumber() == note && msg.getChannel() == ch)
            ++n;
    }
    return n;
}
} // namespace

// UI1: set_mute dispatched before sequencer drain → no note-on for muted lane.
TEST_CASE("UI1 set_mute suppresses lane note-on in next processBlock", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_full);
    proc.set_pattern(ui_pattern_lane0());

    UITestPlayHead ph;
    proc.setPlayHead(&ph);
    proc.lane_routing_.mode[0] = LaneMode::external;
    proc.lane_routing_.channel[0] = 1;

    // Unmuted: note-on ch1 note36 expected
    ph.ppq = 0.0;
    REQUIRE(count_note_on(run_full(proc), 36, 1) > 0);

    // Push mute → same drain as next processBlock
    proc.push_ui_command({UiCommandType::set_mute, 0, 0, 1, 0.0f});
    ph.ppq = 0.0;
    REQUIRE(count_note_on(run_full(proc), 36, 1) == 0); // muted

    // Push unmute → note-on returns
    proc.push_ui_command({UiCommandType::set_mute, 0, 0, 0, 0.0f});
    ph.ppq = 0.0;
    REQUIRE(count_note_on(run_full(proc), 36, 1) > 0); // unmuted
}

// UI2: set_lane_mode + set_lane_channel dispatch → notes routed to EXT MIDI output.
TEST_CASE("UI2 set_lane_mode and set_lane_channel redirect to MIDI out", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_full);
    proc.set_pattern(ui_pattern_lane0());

    UITestPlayHead ph;
    proc.setPlayHead(&ph);

    proc.push_ui_command({UiCommandType::set_lane_mode, 0, 0, static_cast<int32_t>(LaneMode::external), 0.0f});
    proc.push_ui_command({UiCommandType::set_lane_channel, 0, 0, 10, 0.0f});

    ph.ppq = 0.0;
    const auto midi = run_full(proc);
    REQUIRE(count_note_on(midi, 36, 10) > 0); // note on ch10
    REQUIRE(count_note_on(midi, 36, 1) == 0); // not on default ch1
}

// UI3: apply_template TR-8 rewires routing+cc and rewrites mapped lane notes;
//      trig activations preserved (SR1 — only set_note, never set_active).
TEST_CASE("UI3 apply_template TR-8 sets routing and preserves trigs", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_full);

    // User pattern: lane 0 step 0 active with custom note 60
    StepPattern p{};
    p.set_active(0, 0, true);
    p.set_note(0, 0, 60);
    proc.set_pattern(p);

    UITestPlayHead ph;
    proc.setPlayHead(&ph);

    proc.push_ui_command({UiCommandType::apply_template, 0, 0, 0, 0.0f}); // id=0 → TR-8
    ph.ppq = 0.0;
    const auto midi = run_full(proc); // dispatch occurs in drain

    // Routing updated to TR-8 layout
    REQUIRE(proc.lane_routing_.mode[0] == LaneMode::external);
    REQUIRE(proc.cc_out_.enabled == true);
    // TR-8 BD (lane 0): note rewritten to 36, fires on ch10
    REQUIRE(count_note_on(midi, 36, 10) > 0);
    // Original activation (step 0 active) preserved — proven by note-on firing
}

// UI4: set_pad_gain 0 → audio peak 0; restore gain 1 → peak > 0.
TEST_CASE("UI4 set_pad_gain to zero silences audio output", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_s);
    proc.set_pattern(ui_pattern_lane0());

    UITestPlayHead ph;
    proc.setPlayHead(&ph);

    // Gain = 0: note fires but output silent
    proc.push_ui_command({UiCommandType::set_pad_gain, 0, 0, 0, 0.0f});
    ph.ppq = 0.0;
    {
        juce::AudioBuffer<float> buf(2, k_block_s);
        buf.clear();
        juce::MidiBuffer midi;
        proc.processBlock(buf, midi);
    }
    REQUIRE(proc.ui_snapshot().master_peak_l.load() == 0.0f);

    // Reset voice pool; restore gain → output audible
    proc.prepareToPlay(k_sr, k_block_s);
    proc.push_ui_command({UiCommandType::set_pad_gain, 0, 0, 0, 1.0f});
    ph.ppq = 0.0;
    {
        juce::AudioBuffer<float> buf(2, k_block_s);
        buf.clear();
        juce::MidiBuffer midi;
        proc.processBlock(buf, midi);
    }
    REQUIRE(proc.ui_snapshot().master_peak_l.load() > 0.0f);
}

// UI5: set_step activates a previously inactive step → note fires in next processBlock.
TEST_CASE("UI5 set_step activates step and note fires in next processBlock", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_full);
    proc.set_pattern(all_inactive_pattern());

    UITestPlayHead ph;
    proc.setPlayHead(&ph);
    proc.lane_routing_.mode[0] = LaneMode::external;
    proc.lane_routing_.channel[0] = 1;

    // No steps active → no note-on
    ph.ppq = 0.0;
    REQUIRE(count_note_on(run_full(proc), 36, 1) == 0);

    // Activate step 0 lane 0 via command queue
    proc.push_ui_command({UiCommandType::set_step, 0, 0, 1, 0.0f}); // lane=0, step=0, on=1
    ph.ppq = 0.0;
    REQUIRE(count_note_on(run_full(proc), 36, 1) > 0); // step now fires
}

// UI6: snapshot coherent after playing processBlock (AC-4).
TEST_CASE("UI6 snapshot fields coherent after playing processBlock", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_s);
    proc.set_pattern(ui_pattern_lane0()); // lane 0 internal (default): fires into midi_buffer_seq_

    UITestPlayHead ph;
    proc.setPlayHead(&ph);
    ph.ppq = 0.0;

    {
        juce::AudioBuffer<float> buf(2, k_block_s);
        buf.clear();
        juce::MidiBuffer midi;
        proc.processBlock(buf, midi);
    }

    const auto& snap = proc.ui_snapshot();
    REQUIRE(snap.is_playing.load() == true);
    const int step = snap.current_step.load();
    REQUIRE(step >= 0);
    REQUIRE(step < 16);
    REQUIRE(snap.bpm.load() == Catch::Approx(120.0f));
    REQUIRE(snap.master_peak_l.load() > 0.0f);         // test_kick_wav fires → audio > 0
    REQUIRE(snap.lane_last_velocity[0].load() == 100); // default sequencer velocity
}

// UI7: command FIFO order preserved — last command in queue wins.
TEST_CASE("UI7 command FIFO order set_solo last command wins", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_full);

    // 2-lane pattern: both external ch1
    StepPattern p{};
    for (int s : {0, 4, 8, 12})
        p.set_active(0, s, false);
    p.set_active(0, 0, true);
    p.set_note(0, 0, 36);
    p.set_active(1, 0, true);
    p.set_note(1, 0, 38);
    proc.set_pattern(p);
    proc.lane_routing_.mode[0] = LaneMode::external;
    proc.lane_routing_.channel[0] = 1;
    proc.lane_routing_.mode[1] = LaneMode::external;
    proc.lane_routing_.channel[1] = 1;

    UITestPlayHead ph;
    proc.setPlayHead(&ph);

    // Solo(lane1=true) then solo(lane1=false) — second must win
    proc.push_ui_command({UiCommandType::set_solo, 1, 0, 1, 0.0f}); // solo lane1 = true
    proc.push_ui_command({UiCommandType::set_solo, 1, 0, 0, 0.0f}); // solo lane1 = false

    ph.ppq = 0.0;
    const auto midi = run_full(proc);
    // Final state solo[1]=false → all lanes fire → lane 0 fires (note 36 on ch1 visible)
    REQUIRE(count_note_on(midi, 36, 1) > 0);
}

// UI8: getStateInformation drains command queue before serialising (M1 — audit 10-01).
TEST_CASE("UI8 getStateInformation drains queue before serialising lane_routing", "[ui_commands]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    proc.prepareToPlay(k_sr, k_block_s);

    // Push routing change — no processBlock in between
    proc.push_ui_command({UiCommandType::set_lane_mode, 3, 0, static_cast<int32_t>(LaneMode::external), 0.0f});
    proc.push_ui_command({UiCommandType::set_lane_channel, 3, 0, 7, 0.0f});

    // getStateInformation must drain queue in suspend bracket
    juce::MemoryBlock state;
    proc.getStateInformation(state);

    // Source processor must reflect dispatched commands
    REQUIRE(proc.lane_routing_.mode[3] == LaneMode::external);
    REQUIRE(proc.lane_routing_.channel[3] == 7);

    // Restore into fresh processor
    BaqueProcessor proc2;
    proc2.prepareToPlay(k_sr, k_block_s);
    proc2.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    REQUIRE(proc2.lane_routing_.mode[3] == LaneMode::external);
    REQUIRE(proc2.lane_routing_.channel[3] == 7);
}
