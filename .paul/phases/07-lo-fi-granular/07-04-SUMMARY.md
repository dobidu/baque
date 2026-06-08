---
phase: 07-lo-fi-granular
plan: "07-04"
subsystem: audio-dsp
tags: [granular, fx-chain, plock, juce-dsp, catch2]

requires:
  - phase: 07-03
    provides: GranularProcessor standalone DSP (grain pool, freeze, spray, pitch_spread, Hann window)
  - phase: 07-02
    provides: FxChain with LoFiProcessor as first stage + PLockParam infrastructure

provides:
  - GranularProcessor wired as second stage in FxChain::process() (LoFi → Granular → LP → Reverb → Delay → Sidechain)
  - FxParams: granular_spray, granular_pitch_spread, granular_freeze (float boolean >=0.5f)
  - PLockParam extended to 11 entries (granular_spray=8, granular_pitch_spread=9, granular_freeze=10)
  - apply_plock_batch() handles all 11 PLockParam values
  - GC1-GC5 integration tests (129/129 total)
  - Phase 7 DoD marker (GC5 [dod])

affects: [08-scatter-perf-fx, 10-ui-ux, phase-10-wet-dry-mix]

tech-stack:
  added: []
  patterns:
    - "Granular bypass at neutral params: skip granular_.process() when spray=0 AND pitch=0 AND freeze=false — prevents zero-output regression when capture buffer unpopulated"
    - "Fill phase before granular measurement: same pattern as GR3/GR4 — capture buffer must be populated before grains can reconstruct non-silence"

key-files:
  created:
    - tests/test_granular_chain.cpp
  modified:
    - src/audio/fx_params.h
    - src/audio/plock_pattern.h
    - src/audio/fx_chain.h
    - src/audio/fx_chain.cpp
    - src/plugin_processor.cpp
    - tests/test_plock.cpp
    - tests/test_lo_fi_chain.cpp
    - tests/CMakeLists.txt

key-decisions:
  - "Granular bypass at neutral params in fx_chain.cpp — prevents 7 regressions in existing FxChain tests (all using neutral params without capture buffer pre-fill)"
  - "GC1 fill phase added — same root cause as GR3/GR4 in 07-03; plan did not pre-apply this pattern to GC1"
  - "ctest -R 'dod' doesn't work — Catch2 [dod] tags not exposed as ctest labels; verified via ctest -R 'GC5'"

patterns-established:
  - "FxChain granular bypass: if (spray > 0 || pitch > 0 || freeze) — apply to any future always-on effect that reads from a capture buffer"
  - "PLockParam append-only: existing values 0–7 permanent; new values appended at 8, 9, 10"

duration: ~2h
started: 2026-06-08T05:00:00Z
completed: 2026-06-08T08:00:00Z
description: "GranularProcessor wired into FxChain as second stage; 3 granular PLockParam entries; GC1-GC5 pass; Phase 7 DoD (GC5) confirmed; 129/129 tests"
type: Summary
about: "BAQUE"
---

# Phase 7 Plan 04: GranularProcessor FxChain Integration Summary

**GranularProcessor wired as second FxChain stage (LoFi → Granular → LP → Reverb → Delay → Sidechain); 3 granular params p-lockable; GC1-GC5 pass; Phase 7 DoD confirmed at 129/129**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2h |
| Started | 2026-06-08T05:00:00Z |
| Completed | 2026-06-08T08:00:00Z |
| Tasks | 2 completed |
| Files modified | 8 (1 created) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: FxChain default granular params → finite non-silent | Pass | GC1 ✓ — required fill phase (deviation; see below) |
| AC-2: granular freeze holds captured audio through FxChain | Pass | GC2 ✓ — peak > 0.01f after silence with freeze=1.0f |
| AC-3: spray=0.5 measurably differs from spray=0.0 via FxChain | Pass | GC3 ✓ — max_diff > 0.001f confirmed |
| AC-4: PLockParam granular indices 8/9/10; k_plock_param_count==11 | Pass | GC4 ✓ — enum values verified |
| AC-5: Phase 7 Lo-fi + Granular DoD | Pass | GC5 [dod] ✓ — 129/129 total |

## Accomplishments

- GranularProcessor (07-03 standalone) wired into FxChain at correct position (after LoFi, before LP filter)
- PLockParam extended from 8 to 11 entries; all 11 cases handled in apply_plock_batch()
- GC1-GC5 integration tests created and passing
- Phase 7 DoD marker (GC5) confirms lo-fi + granular both wired and p-lockable
- Zero regressions in existing 124 tests

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/fx_params.h` | Modified | Added granular_spray, granular_pitch_spread, granular_freeze fields |
| `src/audio/plock_pattern.h` | Modified | Extended PLockParam with entries 8/9/10; count=11 |
| `src/audio/fx_chain.h` | Modified | Added granular_processor.h include + GranularProcessor granular_ member |
| `src/audio/fx_chain.cpp` | Modified | granular_.prepare()/process()/reset() wiring + bypass at neutral params |
| `src/plugin_processor.cpp` | Modified | apply_plock_batch(): 3 new cases for granular params |
| `tests/test_plock.cpp` | Modified | PL6: k_plock_param_count 8→11 |
| `tests/test_lo_fi_chain.cpp` | Modified | LC4: k_plock_param_count 8→11 (audit M1 fix) |
| `tests/test_granular_chain.cpp` | Created | GC1-GC5 FxChain integration tests |
| `tests/CMakeLists.txt` | Modified | Added test_granular_chain.cpp to baque_tests |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| Granular bypass at neutral params | Without bypass, grains reconstruct silence from empty capture buffer → 7 existing tests regressed to zero output. Neutral granular = no audible effect = passthrough is correct behavior | Required deviation from plan's SR1 "always-on" comment; establishes bypass-at-neutral pattern for future capture-buffer effects |
| GC1 fill phase (deviation from plan) | Plan described single 4096-sample buffer; actual implementation requires separate fill+measure same as GR3/GR4 (07-03 audit pattern not pre-applied to GC1) | GC1 now uses `prepare(44100.0, 4096)` + fill 4096 + measure 1024 |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 2 | Essential: prevented regressions and vacuous tests |
| Deferred | 0 | — |

**Total impact:** Essential fixes, no scope creep

### Auto-fixed Issues

**1. GC1 Missing Fill Phase**
- **Found during:** T1 verify (first ctest run)
- **Issue:** Plan described `chain.prepare(44100.0, 1024)` with single 4096-sample buffer. Peak = 0.0f: grains read from unprimed capture buffer (zeros).
- **Fix:** Changed to `prepare(44100.0, 4096)` + fill 4096 samples first, then separate 1024-sample measurement buffer
- **Files:** `tests/test_granular_chain.cpp`
- **Root cause:** Same as GR3/GR4 in 07-03 (capture buffer priming); audit applied that fix to GC3 but not GC1

**2. Granular "Always-On" Regression (7 tests)**
- **Found during:** T2 full suite (post-GC fix rebuild)
- **Issue:** granular_.process() with empty capture buffer reconstructs silence → 7 existing tests (FC2, FC4, FC6, LC1, LC2, LC3, PD3) got zero output. Plan SR1 comment said "always-on" but this destroyed signal for tests using neutral params without fill phase.
- **Fix:** Added bypass condition in fx_chain.cpp: `if (spray > 0 || pitch > 0 || freeze)` — skip granular when all three params neutral
- **Files:** `src/audio/fx_chain.cpp`
- **Verification:** 129/129 (was 122/129 before fix)

**3. ctest -R "dod" Doesn't Work (plan verification error)**
- **Found during:** T2 dod verification
- **Issue:** Catch2 [dod] tags not exposed as ctest labels; `-R "dod"` regex matches test names, not tags. Plan's verification step was wrong.
- **Fix:** Verified GC5 via `ctest -R "GC5"` — passes (GC5 is the DoD marker)
- **No file change needed**

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| GC1 peak=0.0f (first run) | Added fill phase; same pattern as GR3/GR4 |
| 7 existing tests zeroed by granular (second run) | Added neutral-param bypass in fx_chain.cpp |
| clang-format violation in fx_chain.cpp granular block | `clang-format -i` then dry-run confirmed clean |

## Next Phase Readiness

**Ready:**
- GranularProcessor is wired and p-lockable; all 11 PLockParam entries have dispatch cases
- Phase 7 DoD satisfied: LoFiProcessor + GranularProcessor both in FxChain, both tested
- 129/129 tests green; clang-format clean
- FxChain processing order finalized: LoFi → Granular → LP → Reverb → Delay → Sidechain

**Concerns:**
- Granular wet/dry mix is always-active when non-neutral (deferred to Phase 10 UI)
- Per-pad granular routing deferred to Phase 10
- CI Node.js 20 action upgrade still pending (deadline June 16, 2026)

**Blockers:**
- None

---
*Phase: 07-lo-fi-granular, Plan: 07-04*
*Completed: 2026-06-08*
