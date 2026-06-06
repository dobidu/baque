---
description: "BAQUE — current position and accumulated context"
type: ProjectState
about: "BAQUE"
---

# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-06-04)

**Core value:** Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, and controlled error as first-class features
**Current focus:** Phase 5 (Feel Engine) — 05-03 audited (feel presets + Phase 5 DoD); ready for apply

## Current Position

Milestone: v1.0 Release
Phase: 5 of 13 (Feel Engine ⭐) — In progress
Plan: 05-03 (feel presets + Phase 5 DoD) — PLAN audited, ready for apply
Status: 05-03 AUDIT complete — next: apply
Last activity: 2026-06-06 — Phase 5 plan 05-03 audited (1M+2SR applied, 3 deferred)

Progress:
- Milestone: [█████░░░░░] 40%
- Phase 4: [██████████] 100% ✅
- Phase 5: [████████░░] 75% (plan 2 of 3 complete, 05-03 audited)

Phase 4 complete (slice-around-fork strategy):
- 04-01: Pad bank + per-pad playback (varispeed, reverse, gain/pan) ✅ 2026-06-05
- 04-02: Choke groups + velocity layers + round-robin + ADSR/play modes ✅ 2026-06-05
- 04-03: Auto-slice (transient detection) + chop-to-pads ✅ 2026-06-05
- 04-04: R&D-TS fork v1 integration — offline time-stretch ✅ 2026-06-05

Phase 5 (Feel Engine ⭐):
- 05-01: FeelPattern + FeelEngine + per-step timing offset ✅ 2026-06-05
- 05-02: Gaussian humanize + PRNG seeding ✅ 2026-06-06
- 05-03: Feel presets + Phase 5 DoD 🔍 PLAN audited

## Loop Position

Current loop state:
```
PLAN ──▶ AUDIT ──▶ APPLY ──▶ UNIFY
  ✓       ✓              [05-03 audited — apply next]
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
| SampleVoice::get_position() = frames rendered (voice age) | Phase 4 | Steal metric stable under reverse/varispeed; source position no longer monotonic |
| Pad params single-writer (documented, not enforced) | Phase 4 | UI/automation phases MUST upgrade to atomics or command queue before live edits |

### Deferred Issues

| Issue | Origin | Effort | Revisit |
|-------|--------|--------|---------|
| Sample embed in presets (opt-in "collect & save"?) | ESCOPO §14.5 | M | Phase 11 (presets) |
| Song mode depth — chaining only in v1? | ESCOPO §14.6 | M | Phase 3 planning |
| Multi-out in v1 vs v1.1 | ESCOPO §14.7 | M | Phase 9/10 planning |
| Upgrade CI actions/checkout + cache from v4→v5 (Node.js 20 deprecated) | Phase 1 | S | Before June 16, 2026 (deadline from GitHub) |

### Blockers/Concerns
None yet.

## Session Continuity

Last session: 2026-06-06 (session 9)
Stopped at: Phase 5 plan 05-03 audited — feel presets + Phase 5 DoD; 3 findings applied
Next action: /paul:apply 05-03
Resume file: .paul/phases/05-feel-engine/05-03-PLAN.md
Git strategy: main
Resume context:
- Phase 4 git commit includes all 04-01..04-04 work
- Phase 5: Feel Engine ⭐ — product core; micro-timing, humanize, dual-grid, feel presets
- CI Node.js 20 action upgrade due before June 16, 2026

---
*STATE.md — Updated after every significant action*
