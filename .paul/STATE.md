---
description: "BAQUE — current position and accumulated context"
type: ProjectState
about: "BAQUE"
---

# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-06-04)

**Core value:** Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, and controlled error as first-class features
**Current focus:** Phase 9 (MIDI / Hardware) — in progress (2/4 plans)

## Current Position

Milestone: v1.0 Release
Phase: 9 of 13 (MIDI / Hardware) — In progress (2/4 plans)
Plan: 09-02 complete ✅; 09-03 not started
Status: 09-02 APPLY + UNIFY complete; 182/182 tests
Last activity: 2026-06-08 — 09-02 APPLY complete — MidiClock 24ppqn + start/stop/continue + last_tick_ monotonic guard

Phase 9 decomposition (4-plan, MIDI/Hardware):
- 09-01: Per-lane routing (INT/EXT/BOTH + channel) + Sequencer EXT MIDI out + processBlock merge + stop-flush ✅ 2026-06-08
- 09-02: MIDI clock OUT master (24ppqn + start/stop/continue + monotonic guard) ✅ 2026-06-08
- 09-03: CC out (p-locks→CC, drive TR-8 scatter) + MIDI in + MIDI learn
- 09-04: TR-8/TR-8S mapping templates + Phase 9 DoD (drives real TR-8 — manual hardware verify)

Phase 8 decomposition (4-plan, mirrors Phase 6/7):
- 08-01: ScatterEngine standalone (ring + repeat/reverse/gate/decimate + type 1-10 + depth + beat-sync) ✅ 2026-06-08
- 08-02: ScatterEngine wired into processBlock (post-FxChain) + scatter_type/depth APVTS + p-lock (PLockParam 11/12) ✅ 2026-06-08
- 08-03: TapeStop + Gater perf FX, wired post-scatter, APVTS + p-lock (PLockParam 13/14, count 15) ✅ 2026-06-08
- 08-04: Fills via trig conditions + mute/solo groups + Phase 8 DoD (scene morph deferred) ✅ 2026-06-08

Progress:
- Milestone: [██████░░░░] 62% (8/13 phases complete)
- Phase 5: [██████████] 100% ✅
- Phase 6: [██████████] 100% ✅ (4/4 plans done)
- Phase 7: [██████████] 100% ✅ (4/4 plans done)
- Phase 8: [██████████] 100% ✅ (4/4 plans done)

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
  ✓        ✓        ✓          ✓     [09-02 complete; next: /paul:plan 09-03]
```

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
| SampleVoice::get_position() = frames rendered (voice age) | Phase 4 | Steal metric stable under reverse/varispeed; source position no longer monotonic |
| Pad params single-writer (documented, not enforced) | Phase 4 | UI/automation phases MUST upgrade to atomics or command queue before live edits |

### Deferred Issues

| Issue | Origin | Effort | Revisit |
|-------|--------|--------|---------|
| Sample embed in presets (opt-in "collect & save"?) | ESCOPO §14.5 | M | Phase 11 (presets) |
| Song mode depth — chaining only in v1? | ESCOPO §14.6 | M | Phase 3 planning |
| Multi-out in v1 vs v1.1 | ESCOPO §14.7 | M | Phase 9/10 planning |
| Upgrade CI actions/checkout + cache from v4→v5 (Node.js 20 deprecated) | Phase 1 | S | Before June 16, 2026 (deadline from GitHub) |
| Scene morph (perf FX, ESCOPO §4.7) deferred from 08-04 — performance gesture coupled to scenes/UI | Phase 8 | M | Phase 10 (UI/UX) or 11 (presets/scenes) |

### Blockers/Concerns
None yet.

## Session Continuity

Last session: 2026-06-08 (session 20)
Stopped at: 09-02 APPLY + UNIFY complete
Next action: /paul:plan 09-03 (CC out + MIDI in + MIDI learn)
Resume file: .paul/phases/09-midi-hardware/09-02-SUMMARY.md
09-02 done: MidiClock (src/audio/midi_clock.h/.cpp); 24ppqn 0xF8 + start/stop/continue; last_tick_ monotonic guard (M1); clock_master_ public bool (default false); emitted to midi_buffer_ext_ after stop-flush before clear+addEvents; prepare/reset in prepareToPlay/releaseResources; 9 MC tests; 182/182 green.
09-01 done: LaneRouting (src/audio/lane_routing.h, mode[16] INT/EXT/BOTH + channel[16]); Sequencer::generate(...,routing,midi_ext) emits EXT note per lane on its channel; lane_routing_ PUBLIC member; processBlock merges midi_buffer_ext_ → midi_messages out after MIDI-in; stop-flush All-Notes-Off (CC123) per EXT channel on play→stop edge (was_playing_). EXT note-off uses pattern note (not INT tracker). 173/173 tests.
Git strategy: main (Phase 9 commit at phase transition — like Phase 8)
Resume context:
- Phase 9 in progress (2/4): 09-01 ✅ + 09-02 ✅
- processBlock order: voices → fx_chain → scatter → gater → tape_stop → [EXT notes in midi_buffer_ext_] → stop-flush → clock → midi_messages.clear()+addEvents(ext)
- MidiClock invariants: 24ppqn; last_tick_ absolute index; ppq regression resyncs last_tick_=ceil(ppq_start*24)-1; master=false→silence; start ppq≤0.01 else continue
- clock_master_ single-writer (public, like lane_routing_); UI/automation phase atomicizes
- 182/182 tests; Catch2 ASCII-only + PRE_TEST
- CI Node.js 20 action upgrade due before June 16, 2026 (8 days — critical)
- Deferred: slave/PLL, SPP, sub-block transport position, real TR-8 jitter (09-04 DoD)

### Git State
Last commit: b20d99f — feat(08): Phase 8 complete — Scatter / Perf FX
Branch: main
Feature branches merged: none
Uncommitted: 09-01 + 09-02 changes (staged for phase-transition commit after 09-04)

---
*STATE.md — Updated after every significant action*
