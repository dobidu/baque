---
description: "BAQUE — current position and accumulated context"
type: ProjectState
about: "BAQUE"
---

# Project State

## Project Reference

See: .paul/PROJECT.md (updated 2026-06-04)

**Core value:** Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, and controlled error as first-class features
**Current focus:** Phase 1 (Setup) complete — CI green, skeleton built; starting Phase 2 (Core Audio)

## Current Position

Milestone: v1.0 Release
Phase: 1 of 13 (Setup) — Planning
Plan: Phase 1 complete — transitioning to Phase 2
Status: Ready to plan Phase 2
Last activity: 2026-06-04 — Phase 1 (Setup) complete, all 2 plans unified

Progress:
- Milestone: [█░░░░░░░░░] 8%
- Phase 1: [██████████] 100% ✅

## Loop Position

Current loop state:
```
PLAN ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓     [Phase 1 complete — ready to plan Phase 2]
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
Next action: /paul:plan (Phase 2 — Core Audio: sample voice, 1 pad, transport)
Resume file: .paul/ROADMAP.md
Resume context:
- Phase 1 complete: JUCE 8.0.13 skeleton + 3-OS CI green; repo at github.com/dobidu/baque
- Phase 2 goal: first real audio (single pad fires sample, sample-accurate, RT-safe)
- Node.js 20 CI action upgrade due before June 16, 2026

---
*STATE.md — Updated after every significant action*
