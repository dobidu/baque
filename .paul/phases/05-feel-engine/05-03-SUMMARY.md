---
phase: 05-feel-engine
plan: 03
subsystem: audio
tags: [feel-engine, presets, dilla, burial, prng, humanize, dod]

requires:
  - phase: 05-feel-engine/05-02
    provides: FeelEngine with xorshift32 PRNG + Box-Muller, set_seed(), prepare(), apply_vel_jitter(), next_timing_jitter_samples()

provides:
  - FeelPresets struct with 6 static factory functions (Straight, Boom-Bap, Dilla Drunk, Burial Broken, FlyLo Wonk, Bonobo Loose)
  - Phase 5 DoD met: Dilla Drunk + Burial Broken perceptible AND seed-reproducible

affects: [Phase 6 FX, Phase 10 UI, Phase 11 Presets]

tech-stack:
  added: []
  patterns:
    - "Feel presets as pure data static factories — no allocation, RT-safe, trivially copyable"
    - "DoD gate via Catch2 [dod] tag — ctest-runnable as ./baque_tests '[dod]'"

key-files:
  created:
    - src/audio/feel_presets.h
    - src/audio/feel_presets.cpp
    - tests/test_feel_presets.cpp
    - .paul/phases/05-feel-engine/05-03-AUDIT.md
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt
    - .paul/STATE.md
    - .paul/ledger.toml

key-decisions:
  - "Static factory functions return FeelPattern by value — zero allocation, trivially copyable POD"
  - "Preset data is artistic — treat changes as product decisions, not refactors"
  - "DoD test split: data-level (FP3/FP5) + pipeline-level (FP4/FP6 with AC-9 humanize-actually-ran)"

patterns-established:
  - "Preset data in static constexpr arrays inside function bodies — zero-overhead (linker merges)"
  - "DoD test AC-9 pattern: run with humanize disabled, verify humanized positions differ"

duration: ~20min
started: 2026-06-06T01:00:00Z
completed: 2026-06-06T01:20:00Z
description: "6 named feel presets (Straight→Burial Broken) + Phase 5 DoD met (84/84 tests)"
type: Summary
about: "BAQUE"
---

# Phase 5 Plan 03: Feel Presets + Phase 5 DoD Summary

**6 named feel presets ship as pure-data static factory functions; Phase 5 DoD met — Dilla Drunk and Burial Broken are perceptible and seed-reproducible (84/84 tests).**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~20min |
| Started | 2026-06-06T01:00:00Z |
| Completed | 2026-06-06T01:20:00Z |
| Tasks | 6 completed |
| Files created | 5 |
| Files modified | 2 + 2 build files |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Straight positions == baseline | Pass | FP1 — identical positions to no-feel generate() |
| AC-2: Straight velocities all 100 | Pass | FP2 — SR1 guard: REQUIRE(found) anti-vacuous |
| AC-3: Dilla Drunk perceptible timing | Pass | FP3 — avg\|timing\|=23.75ms > 20ms; humanize=25ms ≥ 20ms |
| AC-4: Dilla Drunk seed reproducible | Pass | FP4 — p1==p2 across two fresh engines with seed=313 |
| AC-5: Burial Broken perceptible scatter | Pass | FP5 — range=160ms (100−(−60)) > 100ms; humanize=50ms ≥ 40ms |
| AC-6: Burial Broken seed reproducible | Pass | FP6 — p1==p2 across two fresh engines with seed=666 |
| AC-7: All 6 presets enabled + valid | Pass | FP7 — std::array<FeelPattern,6>; enabled=true, seed≠0, k_steps==16 |
| AC-8: Bonobo < Burial humanize ordering | Pass | FP8 — bonobo(8ms/8%) < burial(50ms/20%) both axes |
| AC-9: DoD tests not vacuous (audit SR2) | Pass | FP4+FP6 — humanize-actually-ran: p1 ≠ p_det(humanize=0) |

## Accomplishments

- 6 feel presets as zero-allocation static factories; preset data verified mathematically before coding (23.75ms avg, 160ms range, seed ordering)
- Phase 5 DoD formally met: all 4 DoD requirements (FP3+FP4 Dilla, FP5+FP6 Burial) pass
- Audit added AC-9 anti-vacuous guard — proves humanize pipeline ran, not just deterministic output reproduced twice
- DoD gate shortcut: `./baque_tests '[dod]'` runs 5 tests / 13 assertions independently of full suite

## Task Commits

All tasks in single commit (plan + audit + apply):

| Task | Commit | Description |
|------|--------|-------------|
| T1: feel_presets.h | `6e3e3dd` | FeelPresets struct with 6 [[nodiscard]] factory declarations |
| T2: feel_presets.cpp | `6e3e3dd` | All 6 preset implementations with static constexpr data |
| T3: CMakeLists.txt | `6e3e3dd` | feel_presets.cpp added to BAQUE target |
| T4: tests/CMakeLists.txt | `6e3e3dd` | test_feel_presets.cpp added to baque_tests |
| T5: test_feel_presets.cpp | `6e3e3dd` | 8 tests FP1-FP8 with audit fixes M1+SR1+SR2 |
| T6: Verification | `6e3e3dd` | 84/84 pass; [dod] 5/5 pass; clang-format clean |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/feel_presets.h` | Created | FeelPresets struct: 6 `[[nodiscard]]` static factory functions |
| `src/audio/feel_presets.cpp` | Created | Preset data: static constexpr timing/vel arrays per preset |
| `tests/test_feel_presets.cpp` | Created | FP1–FP8; Phase 5 DoD tests tagged `[dod]` |
| `CMakeLists.txt` | Modified | Added feel_presets.cpp to BAQUE target sources |
| `tests/CMakeLists.txt` | Modified | Added test_feel_presets.cpp to baque_tests |
| `.paul/phases/05-feel-engine/05-03-AUDIT.md` | Created | Audit report: 1M+2SR applied, 3 deferred |
| `.paul/phases/05-feel-engine/05-03-PLAN.md` | Created | Plan with audit fixes incorporated |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| Static factory functions, FeelPattern by value | No allocation, trivially copyable POD — RT-safe from any thread | Preset data is pure data; safe to copy into feel_pattern_ |
| Preset data in static constexpr arrays inside function bodies | Zero overhead (linker merges identical data); avoids TU-level statics | All preset bodies follow same pattern |
| Preset data is artistic — treat as product decisions | Timing values encode musical intent; arbitrary change = silent groove regression | Future maintainers must flag preset changes in PR description |
| AC-9 anti-vacuous guard (audit SR2) | Without it, FP4/FP6 pass even if humanize pipeline silently disabled | Closes gap between "data correct" (FP3/FP5) and "pipeline correct" (FP4/FP6) |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Audit additions | 3 applied | Essential — MSVC compile fix, vacuous-pass guards |
| Deferred | 3 | Harmless clang-tidy/cleanup items |
| Scope creep | 0 | None |

**Total impact:** Audit additions only; no scope changes; 1 extra AC (AC-9) added pre-APPLY.

### Auto-fixed Issues (Audit)

**1. M1 — `#include <array>` missing (MSVC compile error)**
- Found during: Audit of Task 5
- Fix: Added `#include <array>` with explanatory comment in test_feel_presets.cpp
- Commit: 6e3e3dd

**2. SR1 — FP2 vacuous pass if no note-ons generated**
- Found during: Audit of FP2 test
- Fix: Added `bool found = false` + `REQUIRE(found)` after loop
- Commit: 6e3e3dd

**3. SR2 — FP4/FP6 DoD reproducibility tests vacuous if humanize disabled**
- Found during: Audit of DoD tests
- Fix: AC-9 added; FP4+FP6 extended with deterministic-only baseline comparison
- Commit: 6e3e3dd

### Deferred Items

- D1: Integer `0` literals in float constexpr arrays (clang-tidy lowercase suffix warning)
- D2: FP7 `REQUIRE(FeelPattern::k_steps == 16)` is a dead compile-time assertion
- D3: Straight=baseline equivalence not explained in prose

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| Extra alignment spaces violated clang-format | `clang-format -i` applied to all 3 new files — clean on dry-run |
| Unused `collect_velocities` function in test file | Expected warning — helper defined for symmetry, not a test failure |

## Next Phase Readiness

**Ready:**
- Phase 5 Feel Engine complete: FeelPattern + FeelEngine + presets + 84/84 tests
- Phase 6 (FX + P-locks) can start: feel pipeline provides p-lock hook via per-step timing offsets
- Feel presets are RT-safe POD — UI phase can bind preset selector without threading concerns
- Seed determinism enables preset library (Phase 11) to include "canonical groove" saves

**Concerns:**
- Preset data is currently static constexpr — UI phase (10) and preset phase (11) will need a mutable copy with save/load if users can edit preset data
- `collect_velocities` helper is unused — minor cleanup item

**Blockers:**
- None

---
*Built with PAUL Framework v1.4 · https://chrisai.cv/skool*
*Phase: 05-feel-engine, Plan: 03*
*Completed: 2026-06-06*
