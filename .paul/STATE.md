---
description: "BAQUE — current position and accumulated context"
type: ProjectState
about: "BAQUE"
---

# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-06-04)

**Core value:** Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, and controlled error as first-class features
**Current focus:** Phase 2 (Core Audio) complete — RT-safe voice pool, sample-accurate scheduler; starting Phase 3 (Sequencer Base)

## Current Position

Milestone: v1.0 Release
Phase: 3 of 13 (Sequencer Base) — Planning
Plan: 03-01 + 03-02 created + audited, awaiting approval
Status: PLAN audited, ready for APPLY
Last activity: 2026-06-05 — Enterprise audit applied to both Phase 3 plans

Progress:
- Milestone: [██░░░░░░░░] 15%
- Phase 3: [░░░░░░░░░░] 0%

## Loop Position

Current loop state:
```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ○        ○     [Plans created, awaiting approval]
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

Last session: 2026-06-04
Stopped at: Audit complete on both Phase 1 plans; user paused before APPLY
Next action: /paul:audit then /paul:apply .paul/phases/03-sequencer-base/03-01-PLAN.md
Resume file: .paul/phases/03-sequencer-base/03-01-PLAN.md
Resume context:
- 03-01: StepPattern (16×16 grid), StepClock (ppq→step+sample), Sequencer::generate(), pre-populated default pattern
- 03-02: swing (std::atomic<float> in StepClock), NoteTracker (per-lane note history), pattern pending+swap at bar
- Both plans depend on Phase 2 Scheduler — Sequencer generates MidiBuffer, Scheduler dispatches it

---
*STATE.md — Updated after every significant action*
