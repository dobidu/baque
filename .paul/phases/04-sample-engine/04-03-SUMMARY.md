---
phase: 04-sample-engine
plan: "03"
subsystem: audio-engine
tags: [slicer, onset-detection, chop-to-pads, juce, energy-rms, hwr-delta]

requires:
  - phase: 04-01
    provides: SamplePad::sample_buffer() accessor (juce::AudioBuffer<float>&); VoicePool::reset_all()
  - phase: 04-02
    provides: PadBank::k_num_pads = 16; single-writer contract documented

provides:
  - Slicer::detect_onsets() — energy HWR-delta onset detection, offline, RT-unsafe
  - Slicer::chop_to_pads() — slices buffer into consecutive SamplePad buffers (max 16)
  - Stale-pad clearing on re-chop (null guard, sorted-onset jassert, stale-clear loop)

affects: [04-04, 10-ui-ux, 11-presets]

tech-stack:
  added: []
  patterns:
    - "Offline-only Slicer: allocates vector; never called on audio thread"
    - "chop_to_pads() always calls pool.reset_all() first (UAF prevention, inherited from 04-01)"
    - "Stale pads cleared before new slices written (idempotent re-chop)"
    - "detect_onsets() always returns {0} — at least one slice invariant holds on any input"
    - "HWR delta = max(0, rms[i] - rms[i-1]): only positive energy rises fire onsets"

key-files:
  created:
    - src/audio/slicer.h
    - src/audio/slicer.cpp
    - tests/test_slicer.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "chop_to_pads clears pads beyond n_slices: re-chop with fewer onsets must not leave old audio in higher pads"
  - "jassert sorted onsets: debug-mode contract enforcement; release builds tolerate silently (length guard catches it)"
  - "detect_onsets max 16 cap references PadBank::k_num_pads not a magic number"
  - "hop=256 samples: balance between detection resolution and per-frame RMS stability"

patterns-established:
  - "detect_onsets + chop_to_pads two-call pattern: detect first, chop second; allows UI to preview before committing"
  - "Null guard + pool.reset_all() in chop_to_pads null path: pool still cleaned even on bad input"

duration: ~30min
started: 2026-06-05T20:00:00Z
completed: 2026-06-05T20:30:00Z
description: "Offline auto-slicer: energy HWR-delta onset detection + chop-to-pads loading PadBank (54/54 tests)"
type: Summary
about: "BAQUE"
---

# Phase 4 Plan 03: Auto-slice + Chop-to-pads — Summary

**Offline Slicer class: energy-based onset detection + chop-to-pads that loads PadBank slices RT-safely — 54/54 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~30 min |
| Tasks | 2 completed |
| Files created | 3 |
| Files modified | 2 |
| Tests added | 10 (S1–S10) |
| Total tests | 54 (all pass) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: detect_onsets locates transients | Pass | Two-burst signal → {0, 1024}; silence → {0} |
| AC-2: chop_to_pads fills pads from onset list | Pass | Pad 0 = 1.0f, Pad 1 = 2.0f; pool voices reset |
| AC-3: Edge cases without crash or out-of-bounds | Pass | null/zero → {0}/no-write; >16 clamped; single onset = whole buffer |
| AC-4: Stale pads cleared on repeated chop | Pass | 8→3 chop: pads 3-7 cleared, no old audio |
| AC-5: min_slice_ms enforces minimum onset spacing | Pass | 3 bursts 2.3ms apart with 500ms min → only {0} |

## Accomplishments

- `Slicer::detect_onsets()`: energy RMS per 256-sample hop → HWR delta → threshold + min_spacing peak picking. Always returns at least {0}. Capped at `PadBank::k_num_pads` results.
- `Slicer::chop_to_pads()`: null guard → pool.reset_all() → stale-pad clear → slice copy via `juce::FloatVectorOperations::copy`. Self-contained, no changes to audio engine runtime path.
- Audit-added correctness fixes applied before APPLY: null guard (UB), stale-pad clear (silent wrong audio), jassert sorted, min_slice_ms test, re-chop regression test.

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/slicer.h` | Created | Slicer class declaration (detect_onsets + chop_to_pads) |
| `src/audio/slicer.cpp` | Created | Energy HWR-delta algorithm + slice copy implementation |
| `tests/test_slicer.cpp` | Created | 10 tests: S1–S8 AC coverage, S9 min_slice_ms, S10 stale-pad regression |
| `CMakeLists.txt` | Modified | Added `src/audio/slicer.cpp` to BAQUE target_sources |
| `tests/CMakeLists.txt` | Modified | Added `test_slicer.cpp` to baque_tests executable |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `chop_to_pads` clears pads beyond `n_slices` | Re-chop with fewer onsets must not leave stale audio in higher pads — audit finding | All future callers get idempotent re-chop behavior |
| `null guard` calls `pool.reset_all()` even on bad input | Voice pool still cleaned on null; consistent post-condition | No voice-leak even on caller error |
| `hop = 256` fixed constant | Balance between resolution (~5.8ms at 44100) and RMS stability | Sufficient for typical drum loop slicing; adaptive hop deferred |
| Two-call API (detect then chop) | Allows UI to preview onset positions before committing to pad write | Phase 10 UI can show markers, let user adjust, then call chop_to_pads |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed (audit) | 5 findings | 2 must-have + 3 strong applied before APPLY |
| Scope additions | 0 | — |
| Deferred | 3 | Test tolerance tightening, adaptive threshold, multi-channel |

**Total impact:** Audit fixes prevented two real bugs (UB crash, silent stale-audio); no scope creep.

## Issues Encountered

None. Implementation matched plan exactly after audit fixes.

## Next Phase Readiness

**Ready:**
- `Slicer` class fully self-contained — Phase 10 UI can call `detect_onsets` + `chop_to_pads` directly
- `SamplePad::sample_buffer()` interface unchanged — all prior voice/scheduler code unaffected
- Two-call API pattern ready for Phase 10 onset-preview feature

**Concerns:**
- `hop = 256` fixed — complex signals (pitched bass, cymbals) may under-detect; acceptable for v1
- `jassert` on sorted onsets is debug-only; future UI callers must ensure sorted list

**Blockers:**
- 04-04 (time-stretch fork) blocked on R&D-TS fork repo bootstrap — unrelated to Slicer

---
*Built with PAUL Framework v1.4*
*Phase: 04-sample-engine, Plan: 03*
*Completed: 2026-06-05*
