---
phase: 07-lo-fi-granular
plan: "07-01"
subsystem: audio
tags: [dsp, lo-fi, bitcrusher, zoh, sp-1200, sp-303]

requires:
  - phase: 06-fx-plocks
    provides: FxChain DSP pipeline, FxParams, PLockBatch — foundation LoFiProcessor will integrate with in 07-02

provides:
  - LoFiProcessor standalone DSP class (BitCrusher + ZOH sample-rate reduction)
  - k_sp1200, k_sp303, k_8bit preset constants
  - 5 LF tests (LF1–LF5) proving BitCrusher + ZOH DSP correctness

affects: [07-02-fx-integration, 07-03-granular]

tech-stack:
  added: []
  patterns:
    - "std::pow hoisted before per-sample loop (M1 audit fix)"
    - "Channel cap: std::min(num_channels, 2) — held_l_/held_r_ for up to 2ch"
    - "ZOH phase_ initialized to 1.0f → first sample always advances held state"
    - "Processing order: ZOH then bitcrush (matches hardware ADC)"

key-files:
  created:
    - src/audio/lo_fi_processor.h
    - src/audio/lo_fi_processor.cpp
    - tests/test_lo_fi.cpp
  modified:
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "ZOH before bitcrush: matches SP-1200 hardware ADC order (sample at reduced rate, then quantize)"
  - "phase_ = 1.0f in prepare(): first sample always advances held_ (no stale 0.0f output)"
  - "k_sp1200 = {12.0f, 44100.0f/26040.0f}: 26.04kHz native SP-1200 SR → factor ~1.694"

patterns-established:
  - "LF3 exact float == is valid: both samples compute from same held_ with same levels → IEEE754 identical"
  - "levels = std::pow(2.0f, bit_depth-1.0f) hoisted before sample loop, never inside"
  - "LoFiPreset is a plain struct; presets are constexpr free constants in header"

duration: ~30min
started: 2026-06-07T20:30:00Z
completed: 2026-06-07T21:00:00Z
description: "LoFiProcessor DSP (BitCrusher + ZOH) — standalone class with SP-1200/SP-303/8-bit presets, 5 LF tests, 114/114 pass"
type: Summary
about: "BAQUE"
---

# Phase 7 Plan 07-01: LoFiProcessor DSP Summary

**LoFiProcessor standalone DSP class shipped: BitCrusher + ZOH sample-rate reduction with SP-1200/SP-303/8-bit presets; 5 LF tests prove all behaviors; 114/114 pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~30min |
| Started | 2026-06-07T20:30:00Z |
| Completed | 2026-06-07T21:00:00Z |
| Tasks | 2 completed |
| Files created | 3 |
| Files modified | 2 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: 16-bit transparent (< 1 LSB error) | Pass | max_err ≈ 1.53e-5 < 6.1e-5 threshold (LF1) |
| AC-2: 8-bit quantizes 0.3f → 38/128 = 0.296875f exact | Pass | IEEE754 exact; confirmed ≠ 0.3f (LF2) |
| AC-3: ZOH factor=2.0 holds 2 samples, advances | Pass | [0.1,0.1,0.3,0.3] output verified (LF3) |
| AC-4: SP-1200 preset measurably degrades 440Hz sine | Pass | max_diff > 0.001f, max_amp > 0.1f, all finite (LF4) |
| AC-5: Phase 7 lo-fi DoD marker | Pass | LF5 SUCCEED() with [lo_fi] tag (LF5) |

## Accomplishments

- LoFiProcessor RT-safe DSP class (no allocation in process(), all POD state)
- BitCrusher formula: `round(x * 2^(bits-1)) / 2^(bits-1)` — exact grid quantization
- ZOH SR reducer: phase accumulator, phase_ = 1.0f init, `phase_ += 1.0f; if >= factor: advance held; phase_ -= factor`
- Three preset constants: k_sp1200 (12-bit, 26kHz), k_sp303 (12-bit, native), k_8bit (8-bit, 22kHz)
- Enterprise audit found + fixed: std::pow hoisted from per-sample loop (M1), channel UB guard (SR1), LF3 == comment (SR2)

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/lo_fi_processor.h` | Created | LoFiPreset struct, k_sp1200/k_sp303/k_8bit constants, LoFiProcessor class |
| `src/audio/lo_fi_processor.cpp` | Created | prepare() + process() implementation |
| `tests/test_lo_fi.cpp` | Created | LF1–LF5 tests with [lo_fi] tag |
| `CMakeLists.txt` | Modified | Added lo_fi_processor.cpp to BAQUE library sources |
| `tests/CMakeLists.txt` | Modified | Added test_lo_fi.cpp to baque_tests executable |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| ZOH before bitcrush | Matches hardware ADC: sample at reduced rate, then quantize. SP-1200 does this in hardware | Processing order in 07-02 FxChain integration must preserve this |
| phase_ = 1.0f init | First sample must produce output from actual input, not stale 0.0f held state | All ZOH-based tests depend on this; must remain 1.0f in prepare() |
| levels hoisted before sample loop | std::pow is transcendental — not O(1). Per-sample call wastes ~512x compute per block | Pattern for all future DSP hot paths |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Audit-applied | 3 | Essential fixes, no scope change |

**Total impact:** Audit fixes improved RT-safety and prevented future maintenance hazards. No spec or scope changes.

### Audit-Applied Changes

**1. M1 — std::pow hoisted from per-sample loop**
- **Found during:** Enterprise audit (pre-APPLY)
- **Issue:** Plan placed `float levels = std::pow(2.0f, bit_depth - 1.0f)` inside "Per-sample" description → 512+ transcendental calls per block
- **Fix:** Hoisted to `const float levels = ...` before sample loop in process()
- **Verification:** clang-format clean; 114/114 pass

**2. SR1 — channel cap at 2**
- **Found during:** Enterprise audit (pre-APPLY)
- **Issue:** held_l_/held_r_ imply 2ch max but plan said "loop over actual channels" — UB on mono (JUCE buffer ch1 access on 1-channel buffer)
- **Fix:** `const int num_ch = std::min(buffer.getNumChannels(), 2)` + header doc comment
- **Verification:** No regression; handled gracefully

**3. SR2 — LF3 `==` comment**
- **Found during:** Enterprise audit (pre-APPLY)
- **Issue:** Exact float `==` in test is correct but non-obvious — risk of future "fix" to Approx destroying test semantics
- **Fix:** Added comment block explaining IEEE754 guarantee (same held_ + same levels → identical result)
- **Verification:** LF3 passes; semantics preserved

## Issues Encountered

None — plan executed as specified (with audit pre-applied fixes).

## Next Phase Readiness

**Ready:**
- `LoFiProcessor` standalone DSP — testable, RT-safe, format-clean
- Preset constants defined and documented
- 07-02 can include lo_fi_processor.h and wire into FxChain immediately
- FxParams will gain bit_depth + sr_factor fields in 07-02

**Concerns:**
- None — class is clean and isolated

**Blockers:**
- None

---
*Built with PAUL Framework v1.4*
*Phase: 07-lo-fi-granular, Plan: 07-01*
*Completed: 2026-06-07*
