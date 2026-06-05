---
description: "BAQUE — current position and accumulated context"
type: ProjectState
about: "BAQUE"
---

# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-06-04)

**Core value:** Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, and controlled error as first-class features
**Current focus:** Phase 3 (Sequencer Base) complete — step grid, swing, pattern switch, NoteTracker; starting Phase 4 (Sample Engine)

## Current Position

Milestone: v1.0 Release
Phase: 4 of 13 (Sample Engine) — In progress (1/4 plans)
Plan: 04-01 complete (unified)
Status: Ready to plan 04-02
Last activity: 2026-06-05 — 04-01 UNIFY complete: SUMMARY created, loop closed

Progress:
- Milestone: [███░░░░░░░] 25%
- Phase 4: [███░░░░░░░] 25%

Phase 4 plan breakdown (slice-around-fork strategy):
- 04-01: Pad bank + per-pad playback (varispeed, reverse, gain/pan) ✅ 2026-06-05
- 04-02: Choke groups + velocity layers + round-robin + ADSR/play modes — fork-free ← next
- 04-03: Auto-slice (transient detection) + chop-to-pads — fork-free
- 04-04: R&D-TS fork v1 integration — offline time-stretch (BLOCKED on fork repo)

## Loop Position

Current loop state:
```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [04-01 loop complete — ready to plan 04-02]
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

Last session: 2026-06-05 (session 4)
Stopped at: 04-01 loop closed; paused before 04-02 planning
Next action: /paul:plan (04-02 — choke groups + velocity layers + round-robin + ADSR/play modes)
Resume file: .paul/HANDOFF-2026-06-05.md
Git strategy: main
Resume context:
- 04-01 shipped: PadBank, varispeed, reverse, pan; 31/31 tests; pluginval green; WIP commit on main, message "wip(04-sample-engine): paused after 04-01 complete" (local — not pushed)
- 04-02 natural place for per-note voice mapping (note-off still pool-global)
- R&D-TS fork repo bootstrap should start in parallel (before 04-04)
- CI Node.js 20 action upgrade due before June 16, 2026 (11 days)

---
*STATE.md — Updated after every significant action*
