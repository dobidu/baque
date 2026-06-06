---
phase: 05-feel-engine
plan: 02
subsystem: feel-engine
tags: [feel, humanize, prng, xorshift32, box-muller, gaussian, rt-safety, seed-reproducibility]

requires:
  - phase: 05-feel-engine
    plan: 01
    provides: FeelEngine deferred queue + per-step timing offset + Sequencer add_with_feel lambda

provides:
  - FeelPattern: humanize_timing_ms, humanize_vel_pct, seed fields
  - FeelEngine: xorshift32 PRNG + Box-Muller Gaussian + set_seed() + PRNG-reset prepare()
  - Sequencer: per-note timing jitter (note-on only) + velocity jitter with null guard
  - PluginProcessor: set_seed(feel_pattern_.seed) in prepareToPlay

affects: [05-03-presets, phase-6-fx-plocks, phase-10-ui]

tech-stack:
  added: []
  patterns:
    - "xorshift32 PRNG: 4-byte state, never-zero invariant, seed=0→1 guard"
    - "Box-Muller: 2 PRNG advances per Gaussian variate; +1e-7f epsilon on u1 prevents log(0)"
    - "PRNG call order invariant: vel-jitter first [×2], timing-jitter second [×2] per note-on — changing order breaks preset reproducibility"
    - "prepare() resets prng_state_ = seed_ — same seed = same groove every transport restart"
    - "feel_engine null guard in velocity section (M1 audit fix) — prevents crash when feel_engine=nullptr"

key-files:
  modified:
    - src/audio/feel_pattern.h
    - src/audio/feel_engine.h
    - src/audio/feel_engine.cpp
    - src/audio/sequencer.cpp
    - src/plugin_processor.cpp
    - tests/test_feel_engine.cpp

key-decisions:
  - "Humanize note-on only: note-off uses deterministic step offset — avoids double-jitter on paired events"
  - "PRNG call order (vel→timing) is invariant: if reversed, saved presets produce different grooves silently"
  - "std::log + std::cos are RT-safe (pure math, no allocation, set errno not throw)"
  - "seed=0 treated as 1: xorshift32 stuck-state prevention"
  - "PluginProcessor calls set_seed(feel_pattern_.seed) in prepareToPlay only — runtime seed change requires explicit set_seed() call (Phase 10 UI concern)"

patterns-established:
  - "vel-jitter before add_with_feel; timing-jitter inside add_with_feel: vel[PRNG×2] → timing[PRNG×2]"
  - "prepare() dual reset: deferred_count_=0 AND prng_state_=seed_ — both cleared on transport restart"

duration: ~20min
started: 2026-06-06T00:10:00Z
completed: 2026-06-06T00:30:00Z
description: "Gaussian humanize (timing + velocity) with xorshift32 PRNG + seed-based determinism — Feel Engine v2"
type: Summary
about: "BAQUE"
---

# Phase 5 Plan 02: Gaussian Humanize + PRNG Seeding Summary

**Per-note Gaussian timing and velocity jitter wired into Sequencer; xorshift32 PRNG with
Box-Muller transform; seed-based determinism for transport-restart reproducibility; 76/76 tests.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~20 min |
| Started | 2026-06-06T00:10:00Z |
| Completed | 2026-06-06T00:30:00Z |
| Tasks | 3 completed |
| Files modified | 6 (0 created, 6 modified) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Same seed → identical jitter sequence | Pass | FE9 — two runs with set_seed(42) produce identical positions |
| AC-2: Different seeds → different jitter | Pass | FE10 — seed=1 vs seed=99999 positions differ |
| AC-3: humanize_timing_ms=0 → no jitter | Pass | FE11 — positions match deterministic-only (no PRNG advancement) |
| AC-4: humanize_vel_pct=0 → velocities unchanged | Pass | FE11 implicitly; FE6 regression unaffected |
| AC-5: Jitter + defer mechanics preserved | Pass | FE14 — note fires in block 1 or deferred into block 2 |
| AC-6: Velocity jitter clamped [1, 127] | Pass | FE12 — all 16 velocities in range |
| AC-7: prepare() resets PRNG → reproducible restart | Pass | FE13 — prepare() produces identical positions |
| AC-8: seed=0 treated as 1 | Pass | FE15 — at least one non-zero jitter from seed=0 |
| AC-9: Negative jitter clamped to 0 (not dropped) | Pass | FE16 — 64-seed sweep finds ≥1 clamped-to-0 note |
| AC-10: Both humanize + prepare() reproduces positions+velocities | Pass | FE17 — positions AND velocities identical after prepare() |

## Accomplishments

- Gaussian timing + velocity humanize wired end-to-end: FeelPattern → Sequencer → MidiBuffer
- xorshift32 PRNG: 4-byte state, no allocation, deterministic, stuck-state guarded
- Box-Muller: 2 PRNG advances per variate; epsilon prevents log(0) without branching
- Seed reproducibility: prepare() resets PRNG to stored seed → same groove every playback
- PRNG call order invariant documented in-code and in plan (M2 audit finding)
- Null guard added to velocity section for feel_engine=nullptr case (M1 audit finding)

## Task Commits

*(committed as part of 05-02 work — see git log)*

## Files Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/feel_pattern.h` | Modified | Added humanize_timing_ms, humanize_vel_pct, seed fields |
| `src/audio/feel_engine.h` | Modified | Added set_seed, next_timing_jitter_samples, apply_vel_jitter; prng_state_, seed_ members |
| `src/audio/feel_engine.cpp` | Modified | Added xorshift32, gaussian_sample (file-scope static); set_seed, jitter methods; prepare() resets PRNG |
| `src/audio/sequencer.cpp` | Modified | Timing jitter in add_with_feel (note-on only); vel jitter with feel_engine null guard + PRNG order invariant comment |
| `src/plugin_processor.cpp` | Modified | set_seed(feel_pattern_.seed) in prepareToPlay |
| `tests/test_feel_engine.cpp` | Modified | Added FE9-FE17 (9 new tests, total 76) |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| Humanize note-on only | Jitter on note-off creates wrong gate length | note-off follows deterministic step offset |
| PRNG call order invariant (vel→timing) | Changing order breaks saved preset reproducibility silently | Documented in-code; must not be reordered |
| epsilon +1e-7f on Box-Muller u1 | Prevents log(0) without branching; RT-safe | Max Gaussian output ≈ ±5.7σ; bounded, finite |
| set_seed() only in prepareToPlay | Runtime seed change not needed until Phase 10 UI | Phase 10 must call set_seed() on pattern change |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Audit additions (must-have) | 2 | M1: null guard; M2: invariant comment |
| Audit additions (strong-rec) | 2 | SR1: FE16; SR2: FE17 |

**Total impact:** Critical crash-fix (M1) + invariant documentation (M2) + 2 test coverage improvements. No scope creep.

### Audit-Applied Changes (pre-APPLY)

**M1: feel_engine null guard in velocity humanize**
- Issue: velocity section accessed feel_engine without null check; feel_engine is nullable default param
- Fix: `if (feel->humanize_vel_pct > 0.0f && feel_engine)` guard

**M2: PRNG call order invariant comment**
- Issue: vel-before-timing ordering was invisible contract
- Fix: Comment in both velocity section and add_with_feel lambda documenting invariant

**SR1: FE16 — negative jitter clamp coverage**
- 64-seed sweep verifies at least one seed produces a negative-jitter note clamped to position 0

**SR2: FE17 — combined humanize prepare() reproducibility**
- Both flags active; prepare() reset; positions AND velocities verified identical across two runs

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| None | Plan + audit covered all issues pre-APPLY |

## Next Phase Readiness

**Ready:**
- Humanize infrastructure complete; 05-03 adds feel presets (Dilla Drunk, Burial Broken, etc.)
  on top of FeelPattern.humanize_* + seed fields — no structural changes needed
- `FeelEngine::set_seed()` + PRNG state established; preset loader in 05-03 calls set_seed()
  with preset's seed value
- 76/76 tests green; clang-format clean

**Concerns:**
- PluginProcessor calls set_seed() only in prepareToPlay; runtime seed change (Phase 10 UI)
  needs explicit set_seed() call — not a regression, documented
- FeelPattern thread safety still single-writer; unchanged from 05-01 posture

**Blockers:** None — 05-03 can start immediately.

---
*Built with PAUL Framework v1.4*
*Phase: 05-feel-engine, Plan: 02*
*Completed: 2026-06-06*
