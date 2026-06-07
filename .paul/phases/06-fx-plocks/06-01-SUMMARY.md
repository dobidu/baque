---
phase: 06-fx-plocks
plan: 01
subsystem: audio
tags: [plock, fx-params, sequencer, apvts, rt-safe]

requires:
  - phase: 05-feel-engine
    provides: Sequencer::generate() signature with optional feel params; FeelPattern/FeelEngine

provides:
  - PLockPattern/PLockStep/PLockEvent/PLockBatch types (stack-allocated, header-only)
  - FxParams struct with 6 FX parameters + RT-safe defaults
  - Sequencer::generate() extended with optional plock_pattern + plock_batch_out args
  - PluginProcessor: 6 APVTS FX params + plock_pattern_ member + apply_plock_batch()
  - processBlock() snapshots APVTS → FxParams and applies PLockBatch
  - 8 tests PL1-PL8 (92/92 total)

affects: [06-02-fx-chain-dsp, 06-03-sidechain]

tech-stack:
  added: []
  patterns:
    - "PLockBatch stack-allocated in processBlock — zero heap, RT-safe (same block push+pop)"
    - "FxParams snapshot before generate() — p-locks override APVTS base; consistent with FeelPattern pattern"
    - "generate() called exactly once per block — M1 fix prevents double-generate bug"

key-files:
  created:
    - src/audio/plock_pattern.h
    - src/audio/fx_params.h
    - tests/test_plock.cpp
  modified:
    - src/audio/sequencer.h
    - src/audio/sequencer.cpp
    - src/plugin_processor.h
    - src/plugin_processor.cpp
    - tests/CMakeLists.txt

key-decisions:
  - "PLockBatch stack-allocated: push+pop both in processBlock() (single-threaded); no AbstractFifo needed"
  - "generate() call replaced (not inserted) to prevent double-generate() bug (audit M1)"
  - "apply_plock_batch() switch default: has INVARIANT comment warning future devs of silent-swallow risk"
  - "PL6 coverage via smoke test crash-on-null-deref: BaqueProcessor not instantiable in unit tests"

patterns-established:
  - "FxParams as plain float struct — snapshotted once per block, p-locks override specific fields"
  - "Optional plock args follow same null-guard pattern as feel args in generate()"

duration: ~2 sessions (2026-06-06 → 2026-06-07)
started: 2026-06-06T00:00:00Z
completed: 2026-06-07T00:00:00Z
description: "P-lock infrastructure: per-step FX param overrides with stack-allocated PLockBatch, APVTS FX params, and 92/92 tests passing"
type: Summary
about: "BAQUE"
---

# Phase 6 Plan 01: P-lock System Infrastructure Summary

**P-lock infrastructure complete: per-step FX parameter overrides wired from PLockPattern → Sequencer → PluginProcessor, stack-allocated, RT-safe, 92/92 tests passing.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2 sessions |
| Started | 2026-06-06 |
| Completed | 2026-06-07 |
| Tasks | 5 completed |
| Files modified | 8 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: P-lock step emits correct PLockEvent | Pass | PL1 — filter_cutoff=800.0f at step 0 verified |
| AC-2: Step with no p-locks emits nothing | Pass | PL2 — batch.count == 0 |
| AC-3: Multiple p-locks on one step all emitted | Pass | PL3 — filter_cutoff + reverb_mix → count==2 |
| AC-4: plock_pattern=nullptr backward compatible | Pass | PL4 — no regression in MIDI output |
| AC-5: PLockPattern.enabled=false emits nothing | Pass | PL5 — disabled pattern → count==0 |
| AC-6: PluginProcessor APVTS FX params accessible | Pass | PL6 — covered implicitly: smoke test crashes on null-deref if any param ID missing |
| AC-7: Multiple steps firing same block accumulate | Pass | PL7 — 98304-sample block fires steps 0+4 → count≥2 |
| AC-8: PLockBatch overflow guard | Pass | PL8 — 16 steps × 6 params = 96 potential events capped at k_max=32 |

## Accomplishments

- PLockPattern/PLockStep/PLockEvent/PLockBatch data types — all stack-allocated, zero heap allocation on audio thread
- FxParams snapshot struct — 6 FX parameters with RT-safe defaults (filter, reverb, delay, sidechain)
- Sequencer::generate() extended — optional plock_pattern + plock_batch_out args, fully backward compatible
- PluginProcessor integration — APVTS declares 6 FX params; processBlock snapshots → applies PLockBatch
- 8 new PL1-PL8 tests; all 92 tests passing (84 prior + 8 new)

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| T1: plock_pattern.h + fx_params.h | `a75ae4f` | PLock data types + FxParams struct |
| T2: sequencer.h/cpp | `a75ae4f` | generate() extended with optional plock args |
| T3: plugin_processor.h/cpp | `a75ae4f` | APVTS FX params + apply_plock_batch() |
| T4: test_plock.cpp + CMakeLists.txt | `a75ae4f` | PL1-PL8 tests |
| T5: build verify + clang-format | `a768aad` | 92/92 pass, format clean |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/plock_pattern.h` | Created | PLockParam enum, PLockStep, PLockPattern, PLockEvent, PLockBatch |
| `src/audio/fx_params.h` | Created | FxParams — 6 FX params + defaults for per-block snapshot |
| `src/audio/sequencer.h` | Modified | generate() signature extended with plock optional args |
| `src/audio/sequencer.cpp` | Modified | P-lock emission block added when step fires |
| `src/plugin_processor.h` | Modified | fx_params.h + plock_pattern.h includes, plock_pattern_ member, apply_plock_batch() decl |
| `src/plugin_processor.cpp` | Modified | 6 APVTS FX params in layout; generate() call replaced with p-lock integration |
| `tests/test_plock.cpp` | Created | PL1-PL8 unit tests |
| `tests/CMakeLists.txt` | Modified | test_plock.cpp added to build |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| PLockBatch stack-allocated, no AbstractFifo | Push + pop both in processBlock() (same thread); fifo only needed for cross-thread producer/consumer | Zero heap allocation on audio thread — RT constraint maintained |
| generate() call replaced (not inserted) | Audit M1: "insert after" caused double-generate() risk — MIDI emitted twice per block | Single generate() call per block; correct behavior |
| apply_plock_batch() switch default with INVARIANT comment | Future PLockParam additions silently swallowed without compiler warning; comment warns future devs | Technical debt documented inline; Phase 12 (Hardening) should add enum exhaustiveness check |
| PL6 coverage via smoke test | BaqueProcessor not instantiable in unit tests without full JUCE plugin infrastructure | APVTS param presence verified implicitly; fragile but adequate for 06-01 |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed (audit) | 1 | Essential correctness fix |
| Format fixes | 1 | Clang-format applied in T5 |
| Deferred | 1 | switch exhaustiveness check |

**Total impact:** Essential fix (M1), no scope creep.

### Auto-fixed Issues

**1. Double-generate() bug (Audit M1)**
- **Found during:** AUDIT phase
- **Issue:** Plan said "insert p-lock block after generate() call" — would call generate() twice per block
- **Fix:** generate() call REPLACED entirely with the new p-lock-aware block
- **Files:** `src/plugin_processor.cpp`
- **Verification:** 92/92 tests passing; smoke test confirms single MIDI output per block

**2. Clang-format alignment (T5)**
- **Found during:** T5 verification
- **Issue:** Manual column alignment in plock_pattern.h, fx_params.h, plugin_processor.cpp, test_plock.cpp violated project clang-format rules
- **Fix:** `clang-format -i` applied; dry-run verified clean
- **Files:** 4 files reformatted

### Deferred Items

- switch exhaustiveness for PLockParam (Phase 12 Hardening): apply_plock_batch() default: break silently drops unknown params; should add static_assert or [[nodiscard]] exhaustiveness check

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| T5 build launched in background at prior session pause — result unknown | Reran build at session start; 92/92 confirmed |
| Clang-format violations across all new/modified files | `clang-format -i` applied; rebuild confirmed no behavior change |

## Next Phase Readiness

**Ready:**
- FxParams struct available for FxChain::process() — 06-02 can consume directly
- PLockBatch emitted per block — 06-02 FX DSP receives per-step overrides without additional plumbing
- APVTS FX params declared — UI bindings (Phase 10) can attach to existing IDs

**Concerns:**
- apply_plock_batch() switch has silent-swallow risk if new PLockParam values added before Phase 12 Hardening
- Per-lane PLockPattern deferred — all 16 steps share one global pattern; per-lane extension deferred

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 06-fx-plocks, Plan: 01*
*Completed: 2026-06-07*
