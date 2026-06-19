---
description: "BAQUE — current position and accumulated context"
type: ProjectState
about: "BAQUE"
---

# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-06-04)

**Core value:** Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, and controlled error as first-class features
**Current focus:** Phase 12 (Hardening) — Plan 12-03 created (64-voice polyphony + zero-alloc + Phase 12 DoD gate), awaiting AUDIT

## Current Position

Milestone: v1.0 Release
Phase: 12 of 13 (Hardening) — In Progress (2/3 plans done)
Plan: 12-03 audited — M1 (free() override), SR1 (4×note36 P12D4 setup), SR2 (finite check block0 not 499); ready for APPLY
Status: PLAN ✓ AUDIT ✓ APPLY ○ — ready for APPLY
Last activity: 2026-06-19 — Plan 12-03 audited; 3 findings applied, 1 deferred

Phase 11 complete ✅ (2-plan, 2026-06-18 → 2026-06-19):
- 11-01: Full engine state v5 + PresetManager (save/load/list *.bqpreset) + P11D1-P11D5 ✅
- 11-02: FactoryPresetLibrary (6 aesthetics) + BrowserScreen PRESETS tab + P11D6/P11D7 ✅

Phase 10 complete ✅ (7-plan, complex track, 2026-06-10 → 2026-06-18):
- 10-01: UI→engine command queue + atomicization of all single-writer structs + UiStateSnapshot ✅
- 10-02: LookAndFeel + design system (4 themes, knobs/arcs, faders, meters) + header/transport + NAV shell ✅
- 10-03: PERFORM screen (pads 4×4, sequencer grid TODAS/FOCO, sample editor) ✅
- 10-04: Feel Field radial visualizer (parallel candidate after 10-02 — read-only) ✅
- 10-05: FX + MIX screens ✅
- 10-06: PERF FX + MIDI screens ✅
- 10-07: BROWSER + undo/redo + Phase 10 DoD (drag-to-beat workflow) ✅

Phase 9 decomposition (4-plan, MIDI/Hardware) ✅:
- 09-01: Per-lane routing (INT/EXT/BOTH + channel) + Sequencer EXT MIDI out + processBlock merge + stop-flush ✅ 2026-06-08
- 09-02: MIDI clock OUT master (24ppqn + start/stop/continue + monotonic guard) ✅ 2026-06-08
- 09-03: CC out (p-locks→CC, drive TR-8 scatter) + MIDI in + MIDI learn ✅ 2026-06-09
- 09-04: TR-8/TR-8S mapping templates + Phase 9 DoD; AC-6 hardware sign-off resolved verified-by-spec (Tier-3: note/CC map machine-checked vs Roland's published MIDI Implementation Chart, 209/209 tests) ✅ 2026-06-10

Phase 8 decomposition (4-plan, mirrors Phase 6/7):
- 08-01: ScatterEngine standalone (ring + repeat/reverse/gate/decimate + type 1-10 + depth + beat-sync) ✅ 2026-06-08
- 08-02: ScatterEngine wired into processBlock (post-FxChain) + scatter_type/depth APVTS + p-lock (PLockParam 11/12) ✅ 2026-06-08
- 08-03: TapeStop + Gater perf FX, wired post-scatter, APVTS + p-lock (PLockParam 13/14, count 15) ✅ 2026-06-08
- 08-04: Fills via trig conditions + mute/solo groups + Phase 8 DoD (scene morph deferred) ✅ 2026-06-08

Progress:
- Milestone: [█████████░] 85% (11/13 phases complete)
- Phase 5: [██████████] 100% ✅
- Phase 6: [██████████] 100% ✅ (4/4 plans done)
- Phase 7: [██████████] 100% ✅ (4/4 plans done)
- Phase 8: [██████████] 100% ✅ (4/4 plans done)
- Phase 9: [██████████] 100% ✅ (4/4 plans done)
- Phase 10: [██████████] 100% ✅ (7/7 plans done)
- Phase 12: [██████████░] 67% 🔄 (2/3 plans done)

Phase 6 complete ✅:
- 06-01: P-lock system infrastructure (PLockPattern, FxParams, 92/92 tests) ✅ 2026-06-07
- 06-02: FxChain DSP (LP filter + reverb + delay + SmoothedValue, 98/98 tests) ✅ 2026-06-07
- 06-03: SidechainCompressor (IIR envelope follower + 8:1 gain computer, 104/104 tests) ✅ 2026-06-07
- 06-04: Phase 6 DoD — integration tests (p-lock→FxChain→output, 109/109 tests) ✅ 2026-06-07

Phase 7 complete ✅ (Lo-fi + Granular):
- 07-01: LoFiProcessor standalone (bitcrusher + ZOH SR reduction, 114/114 tests) ✅ 2026-06-08
- 07-02: LoFiProcessor wired into FxChain, bit_depth/sr_factor p-lockable (119/119 tests) ✅ 2026-06-08
- 07-03: GranularProcessor standalone (grain pool + freeze + spray + pitch_spread, 124/124 tests) ✅ 2026-06-08
- 07-04: GranularProcessor wired into FxChain, 3 granular PLockParam entries, Phase 7 DoD (129/129 tests) ✅ 2026-06-08

## Loop Position

Current loop state:
```
PLAN ──▶ AUDIT ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ○        ○     [Plan 12-03 audited — M1+SR1+SR2 applied]
```

Next action: /paul:apply .paul/phases/12-hardening/12-03-PLAN.md

## Accumulated Context

### Decisions

| Decision | Phase | Impact |
|----------|-------|--------|
| Feel Engine is product core | Init | Phase 5 is the keystone; everything orbits it |
| SoundTouch fork in separate repo (LGPL) | Init | R&D-TS parallel track from Phase 4; fork v1 blocks Phase 4 |
| v1 formats: VST3 + AU + Standalone | Init | CLAP/AAX/LV2 deferred |
| Enterprise plan audit enabled | Init | Architectural review between PLAN and APPLY (RT-safety focus) |
| Linux full v1 target (resolves §14.9) | Phase 1 | 3-OS CI matrix from start; Linux VST3 + Standalone in release |
| 2026-06-04: Enterprise audit on 01-01 (3 must-have + 3 strong applied, 2 deferred) and 01-02 (5 strong applied, 2 deferred). Verdict both: conditionally acceptable → upgraded | Phase 1 | Plugin identity locked (br.ufpb.lavid.baque, 'Lvd0'/'Bqe1'); JUCE tag pinned; AU + lipo gates in CI |
| Catch2 test names must be ASCII-only | Phase 1 | Windows ctest PRE_TEST filter mangles UTF-8; affects all future tests |
| Node.js 20 action deprecation deferred | Phase 1 | actions/checkout@v5 + cache@v5 upgrade; deadline June 16, 2026 |
| 2026-06-04: Enterprise audit on 02-01 (4 must-have + 2 strong applied, 1 deferred) and 02-02 (2 must-have + 3 strong applied, 2 deferred). Verdict both: conditionally acceptable → upgraded | Phase 2 | WAV header parsing via JUCE decoder; start_offset_ in 02-01 not 02-02; getPlayHead null-guard; steal never null |
| Note-off per-note tracking deferred to Phase 3 | Phase 2 Scheduler note-off is no-op; Phase 3 needs note→voice map for correct stuck-note prevention | Phase 2 | Phase 3 must add MIDI note tracking |
| MIDI channel ignored in Phase 2 | All channels trigger; Phase 3 needs channel routing for INT/EXT hybrid mode | Phase 2 | Phase 3 scope item |
| 2026-06-05: Enterprise audit on 03-01 (3 must-have + 2 strong applied, 2 deferred) and 03-02 (4 must-have + 1 strong applied, 2 deferred). Verdict both: conditionally acceptable → upgraded | Phase 3 | MidiBuffer two-call approach, generate() algorithm, double-trigger guard, atomic pattern switch, memory_order for swing |
| NoteTracker fallback to pattern note on first block | Tracker=0 at startup; note-off would be skipped without fallback | All consumers of NoteTracker must handle first-block case |
| MIDI channel routing still global in Phase 3 | All channels trigger; deferred to Phase 9 | Phase 9 must add per-lane channel filter |
| 2026-06-05: Phase 4 slices around fork blocker | Phase 4 | Varispeed in-house (cheap rate change, ESCOPO §4.11 realtime path); only offline stretch needs fork → isolated in 04-04; fork repo bootstrap runs parallel to 04-01..03 |
| Pad note mapping: pad index = note − 36 | Phase 4 | Matches StepPattern note convention; 16 pads = notes 36–51 |
| 2026-06-05: Enterprise audit on 04-01 (2 must-have + 4 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 4 | Safe-load protocol (reset_all before buffer mutation — UAF prevention); end-of-sample bounds termination + jassert; start_offset_ pinned block-relative; velocity curve pinned linear v/127; single-writer contract in pad_bank.h |
| 2026-06-05: Enterprise audit on 04-02 (3 must-have + 4 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 4 | runtime_.fill({}) in prepare() (stale pointer prevention); dec_frames_ field required; sustain=0 voice-leak guard in note_off(); stolen-voice note_off guard (pad_index check); no goto in process(); frames_rendered_ not reset on loop wrap; self-retrigger choke test |
| 2026-06-05: Enterprise audit on 04-03 (2 must-have + 3 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 4 | chop_to_pads null guard (UB prevention); stale pad clear on re-chop (silent-wrong-audio fix); jassert sorted onsets; min_slice_ms test; stale-pad regression test |
| 2026-06-05: Enterprise audit on 04-04 (2 must-have + 3 strong applied, 2 deferred). Verdict: conditionally acceptable → upgraded | Phase 4 | TimeStretchJuceFixture local struct (cross-file compile fix); (void) trigger_at nodiscard fix; jassert empty output; min_input < 256 guard; T3 buffer 4096→8192 |
| 2026-06-05: Enterprise audit on 05-01 (4 must-have + 2 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 5 | set_pattern() immediate setter (tests broken without it); flush_deferred stale-note discard (queue leak fix); ppq regression guard in Sequencer (ghost notes on loop); defer() RT-alloc jassert; FE7 position verification; FE3 deferred content verification |
| 2026-06-06: Enterprise audit on 05-02 (2 must-have + 2 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 5 | feel_engine null guard in vel section (crash fix); PRNG call order invariant documented; FE16 negative-jitter clamp test; FE17 combined humanize prepare() reproducibility test |
| 2026-06-06: Enterprise audit on 05-03 (1 must-have + 2 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 5 | #include <array> MSVC compile fix; FP2 vacuous-pass guard; FP4/FP6 humanize-actually-ran check (AC-9); DoD [dod] Catch2 tag for targeted CI |
| 2026-06-06: Enterprise audit on 06-01 (1 must-have + 2 strong applied, 1 deferred). Verdict: conditionally acceptable → upgraded | Phase 6 | double-generate() placement fix (M1); PL6 honest coverage doc (SR1); switch default warning comment (SR2) |
| 2026-06-07: Enterprise audit on 06-02 (1 must-have + 2 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 6 | FC4 buf.clear()-in-loop test bug fixed (M1); reverb setParameters every-block documented (SR1); pop-before-push delay invariant documented (SR2) |
| 2026-06-07: Enterprise audit on 06-03 (1 must-have + 2 strong applied, 4 deferred). Verdict: conditionally acceptable → upgraded | Phase 6 | SC3 threshold tightened < 0.5→< 0.4 (M1); SC5 vacuous-pass replaced with contrast test (SR1); jlimit guard against NaN in threshold_db (SR2) |
| 2026-06-07: Enterprise audit on 06-04 (1 must-have + 2 strong applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 6 | PD3 vacuous-pass replaced with delay-echo contrast test (M1); separate FxChain instances per run in PD1/PD2/PD3 (SR1); PD4 non-zero output check added (SR2) |
| 2026-06-07: 06-04 APPLY — PD2 plan assertion inverted (plan said peak_tight < peak_loose, but LOW threshold = MORE compression = LOWER output). Fixed: variables renamed hi_thresh/lo_thresh, assertion flipped. PD1 threshold -60→0dBFS (low threshold = max compression, not disabled). PD4 delay_time 2.0f→0.001f for non-zero SR2 check | Phase 6 | Plan spec error caught during APPLY; test semantics corrected; 109/109 pass |
| 2026-06-07: Enterprise audit on 07-01. Applied 1 must-have (std::pow hoisted from per-sample loop), 2 strongly-recommended (channel cap to 2, LF3 == comment). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 7 | Plan strengthened for enterprise standards |
| 2026-06-07: Enterprise audit on 07-02. Applied 1 must-have (T1 verify expanded to catch PL6 regression), 2 strongly-recommended (no-SmoothedValue comment for lo-fi, LC4 dispatch limitation documented). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 7 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 07-03. Applied 1 must-have (GR3/GR4 explicit fill phase — without it tests read zero capture → vacuous fail), 2 strongly-recommended (jassert(length>0) in hann(), spawn_grain skip documented). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 7 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 07-04. Applied 1 must-have (LC4 regression — test_lo_fi_chain.cpp asserts k_plock_param_count==8, becomes 11; boundary said DO NOT CHANGE but plan must update it), 2 strongly-recommended (granular always-on comment in fx_chain.cpp SR1; granular_freeze threshold >=0.5f in FxParams comment SR2). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 7 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 08-01. Applied 1 must-have (M1: ring read-anchor/freeze invariant — read_anchor_ frozen per slice trails write_pos_ by slice_len, slice_len < ring_size_, slice phase persists across blocks → no feedback/overlap; depth=0 exact dry), 2 strongly-recommended (SR1 stereo coherence — slice phase/gate/decimate shared L+R; SR2 reset() zeros ring+heads, SC9 test). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 8 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 08-02. Applied 1 must-have (M1: integration unverified — removed "degrade to ID check" hedge; require SCH4 post-FxChain ordering test + SCH5 real processBlock smoke; test_transport proves BaqueProcessor instantiable), 2 strongly-recommended (SR1 jlimit(0,k_num_types) clamp on scatter_type before dispatch — p-lock can push out-of-range → 08-01 jassert; SR2 document p-lock>APVTS precedence + grep guard for stray ==11 count asserts). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 8 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 08-03. Applied 1 must-have (M1: tape-stop halt must go to SILENCE via gain→0, not hold last sample — DC offset on master = speaker/pluginval hazard), 2 strongly-recommended (SR1 per-sample SmoothedValue for tape rate not per-block coef — zipper; SR2 mandatory GT6 processBlock wiring smoke — prepare-forgotten → silent no-op, 08-02 M1 lesson). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 8 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 08-04. Applied 1 must-have (M1: gate the whole fire as a unit — note_triggered + note-on; suppressed fill/muted step must NOT update NoteTracker else next note-off mis-targets; note-off stays unconditional; AC-7+TF5 guard), 2 strongly-recommended (SR1 PerfState single-writer contract doc — Phase 10 UI must use atomics/command queue, mirrors Phase-4 pad-params; SR2 TF5 suppression-interaction regression test). Deferred 3 (ratio trigs, fill/mute/solo UI, scene morph). Verdict: conditionally acceptable → upgraded | Phase 8 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 09-01. Applied 1 must-have (M1: transport-stop strands EXT notes on hardware — generate() early-returns when stopped → emit All-Notes-Off CC123 per EXT channel on play→stop edge), 2 strongly-recommended (SR1 EXT note-off note = pattern.get_note(lane,prev_step) not INT NoteTracker which is empty for external-only lanes; SR2 MR8 stop-flush + MR9 note-off-source tests). Deferred 3 (live mode-change orphan, EXT humanize, MIDI-thru). Verdict: conditionally acceptable → upgraded | Phase 9 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 09-02. Applied 1 must-have (M1: monotonic clock — last_tick_ absolute index, emit only n>last_tick_, resync on ppq regression; recomputing from ppq alone double/drops 0xF8 at block boundaries → TR-8 tempo glitch, fails DoD), 2 strongly-recommended (SR1 MC8 two-block-sum-24 boundary + MC9 regression tests; SR2 document start/stop at offset-0, clocks sample-accurate). Deferred 3 (slave/PLL, SPP, real-hardware jitter). Verdict: conditionally acceptable → upgraded | Phase 9 | Plan strengthened for enterprise standards |
| 2026-06-08: Enterprise audit on 09-03. Applied 1 must-have (M1: MIDI-learn is an audio-thread WRITER not single-writer — learn_arm must be std::atomic<int> release/acquire arm-disarm handshake; bindings audio-owned during capture; CcLearnMap non-copyable → mutate in place; Phase 10 UI snapshots, mirrors pad-params/PerfState), 2 strongly-recommended (SR1 single k_param_range[15] source-of-truth read by both norm+denorm — no drift; inbound CC covers all 15 params, removed arbitrary 10-field restriction; switch default jasserts; SR2 AC-6 controller-only capture + CC-out additive note/clock invariant + per-field state-load clamps cc[0,127]/channel[1,16]/target[0,15)). Deferred 3 (sub-block CC timing, log cutoff law, NRPN/14-bit+smoothing). Verdict: conditionally acceptable → upgraded | Phase 9 | Plan strengthened for enterprise standards |
| 2026-06-09: Enterprise audit on 09-04. Applied 1 must-have (M1: Phase-9 DoD INT-lane check was vacuous — pass on idle lane; now requires non-zero audio proving the lane fired before asserting no-INT-MIDI; DoD must drive processBlock not Sequencer::generate — 08-02 lesson), 2 strongly-recommended (SR1 apply_template non-destructive: set_note only, never set_active/set_trig/clear; added Sequencer::current_pattern() accessor; processor apply copies current pattern not fresh default — no data loss; routing+cc reset documented intentional; SR2 DoD asserts EXT note-off on gate close + EXACT 24ppqn clock count not >0). Deferred 3 (TR-8S assignable routing+CC banks, template import/export file format, automated clock-jitter measurement). Verdict: conditionally acceptable → upgraded | Phase 9 | Plan strengthened for enterprise standards |
| 2026-06-10: 09-04 Tier-3 — AC-6 hardware sign-off resolved VERIFIED-BY-SPEC instead of deferred. Built tr8_midi_spec.h from Roland's published MIDI Implementation Chart (TR-8 v1.11) + test_tr8_spec_conformance.cpp (TS1-TS5) machine-checking the hand-written template against it. Note map spec-verified (every note matches chart primary). Value-safe continuous CCs baked ON from chart (scatter_depth=69, reverb=91, delay_mix=16, delay_time=17); scatter_type=68 known but OFF (discrete value-curve needs physical unit). Fixed P9D5 cc 16→69 (16=DELAY LEVEL per chart). 209/209 tests | Phase 9 | Published implementation chart IS the firmware spec for note/CC numbers → byte-level mapping confirmed without hardware; reverses 09-04 audit "CC off until hardware" for continuous CCs |
| 2026-06-10: Enterprise audit on 10-01. Applied 1 must-have (M1: state save/load race — moving struct ownership to audio thread makes getState/setState a data race; both must bracket suspendProcessing + message-thread pre-drain of the command queue, UI8 test), 2 strongly-recommended (SR1 lane pulse derived from midi_buffer_seq_ note-ons not Sequencer internals — auto-respects 08-04 gating, EXT-only no-pulse documented; SR2 push() debug-jassert message thread + apply_template invalid id jassert+ignore not clamp). Deferred 3 (queue-full UI policy → 10-02, EXT-only pulse → 10-03, APVTS mute/solo automation → 10-05). Verdict: conditionally acceptable → upgraded | Phase 10 | Plan strengthened for enterprise standards |
| 2026-06-16: Enterprise audit on 10-03. Applied 4 must-have (M1 current_pattern by-value + correct doc; M2 PadGrid/Sequencer stopTimer() in dtor; M3 setFocusedPad/Lane/current_pad bounds guards; M4 toggleStep/setPlayMode return bool + non-vacuous tests), 4 strongly-recommended (SR1 no optimistic repaint; SR2 callback ownership; SR3 knob drag-target capture; SR4 forward-declare in perform_screen.h). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 10 | Plan strengthened for enterprise standards |
| 2026-06-16: Enterprise audit on 10-04. Applied 1 must-have (M1 explicit <cmath>+<algorithm> includes — MSVC build risk on 3-platform CI), 2 strongly-recommended (SR1 remove dead is_playing_ member — current_step_==-1 guard sufficient; SR2 FEEL2 paint smoke test — exercises node-drawing path, test count 230→231). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 10 | Plan strengthened for enterprise standards |
| 2026-06-17: Enterprise audit on 10-05. Applied 1 must-have (M1: missing #include "plugin_processor.h" in both fx_screen.cpp and mix_screen.cpp — BaqueProcessor forward-declared in headers; .cpp files call getAPVTS()/push_ui_command()/ui_snapshot() which require full type definition; compile-blocking on all 3 platforms), 2 strongly-recommended (SR1 corrected apvts_ rationale — it was already public at plugin_processor.h:64; getter is named accessor not capability unlock; SR2 documented mute/solo UI state drift after preset load — mute_state_[]/solo_state_[] are message-thread-local, diverge from engine PerfState; added class comment + scope limit; APVTS mute/solo automation re-deferred to 10-07). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 10 | Plan strengthened for enterprise standards |
| 2026-06-18: Enterprise audit on 10-07. Applied 1 must-have (M1: raw [this] capture in FileChooser async lambda → UAF if BrowserScreen destroyed while OS dialog open; fix: SafePointer<BrowserScreen> that becomes nullptr on destruction), 3 strongly-recommended (SR1 stereo WAV downmix — read both channels, average (L+R)*0.5f into mono buf; SR2 P10D4 vacuous test — added getScreen(Screen) accessor to BaqueEditor + dynamic_cast<BrowserScreen*> check; SR3 timerCallback polls forever — stopTimer() when !contents_.isStillLoading(), restart on folder change). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 10 | Plan strengthened for enterprise standards |
| 2026-06-17: Enterprise audit on 10-06. Applied 1 must-have (M1: explicit `private juce::MouseListener` in PerfFxScreen is wrong — juce::Component already inherits MouseListener; would cause MSVC ambiguous-base-class error; CI-blocking on 3-platform matrix; fix: rely on Component's existing virtual methods, no additional inheritance), 3 strongly-recommended (SR1 timerCallback must NOT overwrite MODE toggle states — advisory read races with in-flight UiCommand causing 100ms flicker; only update note_labels_ advisory; SR2 XyPad mouseDown missing — click-without-drag no-op on performance pad; added mouseDown override identical to mouseDrag; SR3 clock_master_state_ hardcoded false — wrong after preset load; init from proc_.clock_master_ in ctor). Deferred 3. Verdict: conditionally acceptable → upgraded | Phase 10 | Plan strengthened for enterprise standards |
| 2026-06-15: Enterprise audit on 10-02. Applied 1 must-have (M1: font TTF files not committed to git — juce_add_binary_data configure fails on CI/fresh-clone with misleading error; added git-add step + git ls-files verification), 5 strongly-recommended (SR1 NAV switch test NAV1 + showScreen() public + isScreenVisible() accessor; SR2 grain overlay pre-baked juce::Image — no Random in paint(); SR3 setTheme() sets juce::Label::textColourId for ScreenPlaceholder findColour() — no cast; SR4 CI green gate on all 3 platforms before UNIFY; SR5 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR for ScreenPlaceholder). Deferred 4. Verdict: conditionally acceptable → upgraded | Phase 10 | Plan strengthened for enterprise standards |
| 2026-06-18: Enterprise audit on 11-01 (3 must-have + 2 strongly-recommended applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 11 | M1: save_to_file() added to PresetManager + P11D3 redesigned with TemporaryFile (no user-dir pollution, no vacuous CI skip); M2: set_feel_pattern() added to BaqueProcessor + P11D2 redesigned with non-default values (enabled, timing_ms=25.5f, seed=42); M3: P11D5 added — strips v5 subtrees, verifies v4 blob loads without crash; SR1: PLock values clamped ±1e6 on restore. Test count 248→249 |
| 2026-06-19: Enterprise audit on 11-02 (2 must-have + 2 strongly-recommended applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 11 | M1: AlertWindow::showInputBox→inline TextEditor preset_name_editor_ (hosting hazard fix — no modal event loop in plugin editor); M2: TestProcessorFixture→inline proc pattern matching P11D1-P11D5 (compile fix); SR1: user_presets_ bounds guard before index access; SR2: P11D6 added Straight (index 0) inner scope + name(0)/name(5) boundary checks; removed name(6) jassert hazard. Test count 249→251 |
| 2026-06-19: Enterprise audit on 12-01 (1 must-have + 3 strongly-recommended applied, 3 deferred). Verdict: conditionally acceptable → upgraded | Phase 12 | M1: P12D1 vacuous all_finite check (block 999 = zeros) replaced with REQUIRE(midi_first.getNumEvents()==16) before block 0 (processBlock clears midi_messages in-place); SR1: dispatch_ui_command() explicitly added to audit checklist (RT-thread context documented); SR2: grep patterns expanded (emplace_back/insert/resize/std::map/lock/CriticalSection); SR3: scheduler.cpp + note_tracker.cpp added to explicit audit file list |
| 2026-06-19: Enterprise audit on 12-02 (1 must-have + 2 strongly-recommended applied, 2 deferred). Verdict: conditionally acceptable → upgraded | Phase 12 | M1: P12D2b added — DSP boundary smoke (all params normalized 1.0 then 0.0, 50 blocks, assert finite); catches filter_cutoff=20Hz/scatter_type=10/delay_time=0.001s NaN before pluginval binary run; test count 253→254; SR1: REQUIRE(proc.isSynth()) added to P12D2 — drum machine must advertise as synth not effect (pluginval picks test suite from this); SR2: pluginval run switched from tee-pipe to file-redirect so $? captures pluginval exit code not tail's |
| 2026-06-19: Enterprise audit on 12-03 (1 must-have + 2 strongly-recommended applied, 1 deferred). Verdict: conditionally acceptable → upgraded | Phase 12 | M1: free() override added to rt_alloc_counter.cpp — bootstrap bump allocator returns pointers in BSS; without free() override, system free() receives non-heap pointer → UB/crash before any test runs; SR1: P12D4 setup 4×note36 not notes36-43 — only pad 0 has sample, notes 37-43 yield trigger_at nullptr (0 DSP work); 4 real voices exercise FxChain/scatter/tape_stop; SR2: P12D3 finite check moved to block 0 — block 499 is vacuous (test_kick decays to silence in 0.5s) |
| SampleVoice::get_position() = frames rendered (voice age) | Phase 4 | Steal metric stable under reverse/varispeed; source position no longer monotonic |
| Pad params single-writer (documented, not enforced) | Phase 4 | UI/automation phases MUST upgrade to atomics or command queue before live edits |

### Deferred Issues

| Issue | Origin | Effort | Revisit |
|-------|--------|--------|---------|
| Sample embed in presets (opt-in "collect & save"?) | ESCOPO §14.5 | M | Phase 11 (presets) |
| Song mode depth — chaining only in v1? | ESCOPO §14.6 | M | Phase 3 planning |
| Multi-out in v1 vs v1.1 | ESCOPO §14.7 | M | Phase 9/10 planning |
| ~~Upgrade CI actions/checkout + cache from v4→v5~~ | Phase 1 | S | ✅ Done 2026-06-15 (PR #1) |
| Scene morph (perf FX, ESCOPO §4.7) deferred from 08-04 — performance gesture coupled to scenes/UI | Phase 8 | M | Phase 10 (UI/UX) or 11 (presets/scenes) |
| TR-8 scatter_type CC68 value-curve — number known, but BAQUE 0-10 types over TR-8 0-127 select map needs a physical unit to confirm; slot ships OFF | Phase 9 | S | When a real TR-8 is on hand (observational, no CI gate) |
| TR-8S assignable per-instrument routing + TR-8S-only CC banks (beyond shared GM default) | Phase 9 | M | Phase 11 |
| Hardware template import/export file format (load/save custom mappings) | Phase 9 | M | Phase 11 (presets, ESCOPO §10) |
| Async directory scan with background thread + loading indicator | Phase 10 | S | Phase 11 (browser refresh) |
| Waveform thumbnail preview in BROWSER right panel | Phase 10 | M | Phase 11 |
| Tag/category/aesthetic filter system | Phase 10 | M | Phase 11 (needs DB/library) |
| Drag-and-drop from BROWSER to pad | Phase 10 | M | Phase 11 |
| Auto-audition audio playback in BROWSER | Phase 10 | M | Phase 11 (needs AudioTransportSource) |
| APVTS mute/solo automation (mute_state_[]/solo_state_[] UI↔engine sync) | Phase 10 | M | Phase 11 |
| Scene morph (perf FX, ESCOPO §4.7) | Phase 8 | M | Phase 11 (presets/scenes) |

### Blockers/Concerns
None.

## Session Continuity

Last session: 2026-06-19 (session 44)
Stopped at: Plan 12-03 created — 64-voice stress + zero-alloc malloc wrap + Phase 12 DoD gate
Next action: /paul:audit .paul/phases/12-hardening/12-03-PLAN.md
Resume file: .paul/phases/12-hardening/12-03-PLAN.md
Resume context:
- Phase 12 is 3 plans: 12-01 ✅ (RT safety), 12-02 ✅ (pluginval + APVTS tests), 12-03 (P12D3+P12D4+DoD)
- 12-03 APPLY: Task 1 = P12D3 (64 note-ons for note 36, 500 blocks, finite check); Task 2 = rt_alloc_counter.cpp (dlsym malloc wrap) + P12D4; Task 3 = [dod] gate
- VoicePool::k_pool_size = 64 (voice_pool.h:14) — already verified
- Malloc wrap: dlsym(RTLD_NEXT, "malloc") + static bootstrap buffer 256 bytes; fallback = mallinfo2() delta if ODR conflict
- Phase 12 DoD: pluginval ✅ (12-02), P12D3+P12D4 (this plan); 254→256 tests

### Git State
Last commit: 821f874 — feat(12): Plan 12-02 APPLY — P12D2+P12D2b APVTS tests + pluginval strictness 5→10
Branch: main
Uncommitted: .claude/ (framework/skills config — intentionally untracked)

---
*STATE.md — Updated after every significant action*
