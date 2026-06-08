---
phase: 07-lo-fi-granular
plan: "07-02"
status: complete
date: 2026-06-08
tests_before: 114
tests_after: 119
type: Summary
about: "BAQUE"
---

# 07-02 SUMMARY — LoFiProcessor FxChain Integration

## What Was Built

LoFiProcessor (07-01 standalone) wired into FxChain as first processing stage. bit_depth + sr_factor exposed as FxParams fields and PLockParam enum entries with full p-lock plumbing.

## Acceptance Criteria Results

| AC | Test | Result |
|----|------|--------|
| AC-1: default params finite non-silent | LC1 | PASS |
| AC-2: bit_depth=8 measurably degrades | LC2 | PASS |
| AC-3: sr_factor=2.0 measurably differs | LC3 | PASS |
| AC-4: PLockParam indices correct | LC4 | PASS |
| AC-5: Phase 7 lo-fi DoD marker | LC5 | PASS |

**119/119 tests pass. 0 regressions.**

## Files Modified

| File | Change |
|------|--------|
| `src/audio/fx_params.h` | Added `bit_depth=16.0f`, `sr_factor=1.0f` fields after `sidechain_threshold` |
| `src/audio/plock_pattern.h` | Extended PLockParam: `bit_depth=6`, `sr_factor=7`, `count=8` (was 6) |
| `src/audio/fx_chain.h` | Added `LoFiProcessor lo_fi_` + `int max_block_size_=512` members; `#include "lo_fi_processor.h"` |
| `src/audio/fx_chain.cpp` | `prepare()`: store max_block_size_, call `lo_fi_.prepare()`; `process()`: lo_fi_ as FIRST stage; `reset()`: lo_fi_ reset |
| `src/plugin_processor.cpp` | `apply_plock_batch()`: added cases for `PLockParam::bit_depth` + `PLockParam::sr_factor` |
| `tests/test_plock.cpp` | PL6: `k_plock_param_count == 6` → `== 8` |
| `tests/test_lo_fi_chain.cpp` | Created: LC1–LC5 integration tests |
| `tests/CMakeLists.txt` | Added `test_lo_fi_chain.cpp` to `baque_tests` sources |

## Files Created

- `tests/test_lo_fi_chain.cpp` — 5 integration tests proving LoFiProcessor wired into FxChain

## Decisions Made

- **LoFiProcessor is FIRST stage in FxChain::process()** — matches hardware ADC order (coloration at source, before reverb/delay/filter). Order: lo_fi → LP filter → reverb → delay → sidechain.
- **No SmoothedValue for bit_depth/sr_factor** — lo-fi is a discrete switch, not a crossfade. SmoothedValue would incorrectly ramp between lo-fi states and silence transiently; correct behavior is hard cut. Documented in fx_chain.cpp comment (audit SR1).
- **PLockParam append-only** — existing values 0–5 unchanged; new values 6,7 appended; k_plock_param_count derives from count automatically.
- **apply_plock_batch() dispatch verified indirectly** — BaqueProcessor cannot be instantiated in unit tests (full JUCE plugin init required). LC4 verifies enum plumbing; smoke test crash behavior verifies dispatch (null getRawParameterValue → processBlock crash). Same pattern as PL6 (established in Phase 6-01).
- **sidechain_threshold=0.0f in LC tests** — 0 dBFS disables compression for normal signals; established canonical rule from Phase 6.

## Deferred Issues

- bit_depth range validation (jlimit) — deferred to Phase 10 (UI slider will enforce [4, 24])
- Separate APVTS params for bit_depth + sr_factor — currently not exposed in create_parameter_layout() (lo-fi is p-lock only, not a persistent APVTS param); may need Phase 10 UI work
- Vinyl simulation / tape wow+flutter — 07-03+
- Granular engine — 07-03+

## Build Verification

```
cmake --build /mnt/d/work/baque/build --target baque_tests → exit 0
ctest -R "(LC|PL6)" → 6/6 PASSED
ctest --output-on-failure → 119/119 PASSED
clang-format -i [6 modified files] → no violations
```

## Loop Status

```
PLAN ──▶ AUDIT ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ✓          ✓
```
