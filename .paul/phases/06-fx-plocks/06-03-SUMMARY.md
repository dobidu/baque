---
phase: 06-fx-plocks
plan: "06-03"
subsystem: audio
tags: [compressor, sidechain, iir, envelope-follower, gain-computer, juce-dsp]

requires:
  - phase: 06-02
    provides: FxChain DSP skeleton with sidechain_threshold placeholder in FxParams
  - phase: 06-01
    provides: PLockPattern + FxParams struct; sidechain_threshold APVTS param

provides:
  - SidechainCompressor class (RT-safe, noexcept, zero-alloc in process())
  - Asymmetric IIR envelope follower wired into FxChain as last stage
  - Pump/ducking-reverb effect controlled per-step via p-lockable threshold

affects: [06-04+, Phase 10 UI, Phase 12 Hardening]

tech-stack:
  added: []
  patterns:
    - "IIR envelope: exp(-1/(tau*sr)) pre-computed in prepare(), applied per-sample"
    - "Hard-knee gain computer: excess_db * (1/ratio - 1) → always negative → attenuation-only"
    - "jlimit(-80,0) guard before decibelsToGain: NaN defense at APVTS boundary"

key-files:
  created:
    - src/audio/sidechain_comp.h
    - src/audio/sidechain_comp.cpp
    - tests/test_sidechain_comp.cpp
  modified:
    - src/audio/fx_chain.h
    - src/audio/fx_chain.cpp
    - tests/CMakeLists.txt
    - CMakeLists.txt

key-decisions:
  - "IIR envelope follower uses separate attack/release coefficients; no SmoothedValue"
  - "SC3/SC5 tests use 88200-sample buffers (2s) to allow IIR envelope convergence with rectified sine"
  - "Sidechain placed LAST in FxChain::process() — reverb tail + delay echoes compressed together"

patterns-established:
  - "IIR envelope tests need large buffers (≥88200 samples) when attack tau >> signal period"
  - "Contrast tests (two configs, opposite expected results) prevent vacuous-pass on IIR assertions"

duration: ~45min
started: 2026-06-07T00:00:00Z
completed: 2026-06-07T01:00:00Z
description: "SidechainCompressor: asymmetric IIR envelope follower + 8:1 hard-knee compressor, wired last in FxChain"
type: Summary
about: "BAQUE"
---

# Phase 6 Plan 06-03: SidechainCompressor DSP Summary

**Asymmetric IIR envelope follower (5ms attack / 200ms release) + 8:1 hard-knee gain computer wired as the last stage of FxChain::process(), driven by p-lockable sidechain_threshold.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~45min |
| Started | 2026-06-07 |
| Completed | 2026-06-07 |
| Tasks | 3 completed |
| Files modified | 7 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Zero input → zero output | Pass | SC1: peak < 1e-6f |
| AC-2: Below threshold → passes through | Pass | SC2: amplitude 0.1 < threshold 0.251; output > 90% |
| AC-3: Above threshold → compressed | Pass | SC3: tail_peak 0.425 → with 88200-sample buffer < 0.4f (8:1 at 12dB above → 0.299 steady state) |
| AC-4: Pump effect | Pass | SC4: loud block0 charges env; block1 peak < 0.16 |
| AC-5: Threshold controls compression | Pass | SC5: -12dBFS < 0.35; 0dBFS > 0.45 (contrast test, audit SR1) |
| AC-6: prepare/reset/prepare cycle | Pass | SC6: 44100→reset→48000, no crash, signal present |

## Accomplishments

- SidechainCompressor: noexcept prepare/process/reset, zero allocation in process(), IIR coeff pre-computed
- Gain computer formula verified: `excess_db * (1/8 - 1) = excess * -0.875` → always negative → attenuation-only
- jlimit(-80, 0) guard on threshold_db before decibelsToGain (NaN defense; audit SR2)
- 104/104 tests pass; SC1-SC6 all pass; FC1-FC6 and PL1-PL8 regression clean

## Task Commits

All tasks committed atomically:

| Task | Commit | Description |
|------|--------|-------------|
| T1+T2+T3 | `a0c5cd5` | sidechain_comp.h/cpp + FxChain wire-in + SC1-SC6 tests |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/sidechain_comp.h` | Created | SidechainCompressor class declaration |
| `src/audio/sidechain_comp.cpp` | Created | IIR envelope follower + gain computer implementation |
| `src/audio/fx_chain.h` | Modified | Added `#include "sidechain_comp.h"` + `sidechain_comp_` member |
| `src/audio/fx_chain.cpp` | Modified | prepare/process/reset wired; process() last call |
| `tests/test_sidechain_comp.cpp` | Created | SC1-SC6 tests |
| `tests/CMakeLists.txt` | Modified | Added test_sidechain_comp.cpp to executable |
| `CMakeLists.txt` | Modified | Added sidechain_comp.cpp to BAQUE target_sources |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| IIR envelope, not SmoothedValue | Per-sample pump behavior requires envelope tracking the audio signal directly; SmoothedValue is for parameter smoothing | Core architecture of the compressor |
| 88200-sample test buffers for SC3/SC5 | IIR with attack_tau=220 samples and rectified sine input does not converge in 3584 samples (the plan's 4096-512); needs ~37+ tau for steady state | Test file deviation from plan |
| Single commit for all 3 tasks | Session continuity from compacted context; all changes ready together | Non-blocking |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 2 | clang-format violations; test buffer size |
| Deferred | 0 | — |

**Total impact:** Two auto-fixes; no scope creep.

### Auto-fixed Issues

**1. clang-format violations in sidechain_comp.cpp and test_sidechain_comp.cpp**
- **Found during:** T3 build verification
- **Issue:** Multi-line `threshold_linear` assignment and comment alignment not conforming to project style
- **Fix:** `clang-format -i` on all 5 modified/new files
- **Verification:** `clang-format --dry-run --Werror` — FORMAT OK
- **Commit:** `a0c5cd5`

**2. SC3/SC5 test buffers: 4096 → 88200 samples**
- **Found during:** T3 test run (SC3: tail_peak=0.425 > 0.4; SC5: measure_tail=0.390 > 0.35)
- **Issue:** Plan assumed IIR envelope converges in 4096 samples. With attack_tau=220 samples and rectified 440Hz sine (100-sample period), the envelope requires many more cycles to reach steady state. After 4096-512=3584 samples (16 tau but with half-cycle oscillation), envelope sits at ~60-70% of steady state, not the ~99% needed for the compression assertions.
- **Fix:** Changed SC3/SC5 buffers to 88200 (2s), skip first 8192 samples (37 attack tau). At steady state: A=1.0 → gain=-10.5dB → output≈0.299 < 0.4 ✓; A=0.5 at -12dBFS → gain=-5.25dB → output≈0.273 < 0.35 ✓
- **Root cause:** Rectified sine creates asymmetric convergence: attack fires only during ascending half-cycles (~50 samples per cycle); 4096 samples is only 40 sine cycles, not enough for the envelope to integrate to near-amplitude.
- **Verification:** All 8 assertions in SC1-SC6 pass

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| SC3/SC5 initial failures (IIR not converged in 4096 samples) | Increased buffer to 88200, measurement window to 8192-88200 |
| Broken baque_tests inode on NTFS/WSL2 after background build | `rm -f` + rebuild; NTFS inode corruption known WSL2 issue on NTFS mounts |
| Background build completion race with `--dry-run` check | Ran format check immediately after `clang-format -i`; build awaited before testing |

## Next Phase Readiness

**Ready:**
- FxChain::process() chain complete: filter → reverb → delay → sidechain (all 4 DSP stages wired)
- p-lock on sidechain_threshold produces audible pump effect; 06-03 satisfies Phase 6 DoD item
- 104/104 tests; regression clean across all prior phases

**Concerns:**
- Ratio/attack/release are constants (8:1, 5ms, 200ms); Phase 10 UI may need to expose as FxParams fields — impacts struct layout and APVTS registration
- Make-up gain not implemented; heavily-pumped signals lose loudness; deferred to Phase 10/12

**Blockers:**
- None

---
*Phase: 06-fx-plocks, Plan: 06-03*
*Completed: 2026-06-07*
