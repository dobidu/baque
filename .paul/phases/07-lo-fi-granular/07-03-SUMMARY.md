---
phase: 07-lo-fi-granular
plan: "07-03"
status: complete
date: 2026-06-08
tests_before: 119
tests_after: 124
type: Summary
about: "BAQUE"
---

# 07-03 SUMMARY — GranularProcessor Standalone DSP

## What Was Built

GranularProcessor: pre-allocated grain pool granular engine with freeze mode, spray position scatter, and pitch spread. Standalone class — no FxChain integration yet (07-04).

## Acceptance Criteria Results

| AC | Test | Result |
|----|------|--------|
| AC-1: finite non-silent output with signal input | GR1 | PASS |
| AC-2: freeze holds captured audio after input silenced | GR2 | PASS |
| AC-3: spray=0.5 measurably differs from spray=0.0 | GR3 | PASS |
| AC-4: pitch_spread=0.5 measurably differs from pitch_spread=0.0 | GR4 | PASS |
| AC-5: Phase 7 granular DoD marker | GR5 | PASS |

**124/124 tests pass. 0 regressions.**

## Files Created

| File | Change |
|------|--------|
| `src/audio/granular_processor.h` | GranularProcessor class: k_capture_size=8192 (power-of-2), k_num_grains=16, Grain struct with Hann envelope, juce::Random rng_ |
| `src/audio/granular_processor.cpp` | prepare(), reset(), process(), spawn_grain(), hann(), lerp_buf() |
| `tests/test_granular.cpp` | GR1–GR5 tests with [granular] tag |

## Files Modified

| File | Change |
|------|--------|
| `CMakeLists.txt` | Added `src/audio/granular_processor.cpp` to target_sources(BAQUE PRIVATE ...) |
| `tests/CMakeLists.txt` | Added `test_granular.cpp` to add_executable(baque_tests ...) |

## Decisions Made

- **k_capture_size=8192 power-of-2** — branchless wrap with `& (k_capture_size - 1)` in lerp_buf and write_pos advance. RT-safe, no branch on boundary.
- **grain_timer_=0 in reset()** — first grain spawns at first sample of first process() call. GR1 detects non-zero output immediately without needing long warmup.
- **spray=0 → scatter=0 math** — `scatter = (rng.nextFloat() - 0.5f) * 2.0f * spray_samples` where spray_samples=0 → scatter=0 regardless of RNG state. Deterministic baseline for GR3.
- **pitch_spread=0 → rate=1.0 math** — `rate = 1.0f + (rng.nextFloat() - 0.5f) * 0.0f = 1.0f`. Deterministic baseline for GR4.
- **GR3/GR4 explicit fill phase** — process() modifies buffer in place; separate fill buffers required per instance before measurement phase (audit M1 fix). Without this, capture buffer stays zero → grains output zero → test vacuously fails.
- **jassert(length>0) in hann()** — matches project pattern established in Phase 6-03. Catches invalid grain state in debug builds (audit SR1).
- **spawn_grain() "all busy → skip" documented** — intentional drop when all 16 slots active. Comment prevents future maintainer from adding retry/dynamic alloc on audio thread (audit SR2).
- **No juce::dsp dependency** — uses juce_audio_basics + `<cmath>` + `<algorithm>` only. No ScopedJuceInitialiser_GUI required in tests.
- **Grain pool pre-allocated** — grains_[k_num_grains] is a fixed member array. Zero RT allocation in process().

## Deferred Issues

- Rate clamp for pitch_spread > 1.0 (negative rate → reverse playback unintended) — deferred to Phase 10; UI slider will enforce pitch_spread ∈ [0, 1]
- juce::Random deterministic seeding for test reproducibility — spray=0/pitch=0 paths are fully deterministic (RNG output zeroed by multiply); practical risk zero
- Output gain normalization (N overlapping grains with Hann windows can sum >1.0) — float, no clipping; wet/dry mix in 07-04 will control level

## Build Verification

```
cmake --build /mnt/d/work/baque/build --target baque_tests → exit 0
ctest -R "GR" --output-on-failure → 5/5 PASSED
ctest --output-on-failure → 124/124 PASSED
clang-format --dry-run --Werror [3 new files] → FORMAT OK
```

## Loop Status

```
PLAN ──▶ AUDIT ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓          ✓
```
