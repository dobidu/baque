---
phase: 12-hardening
plan: 03
subsystem: testing
tags: [catch2, juce, voice-pool, rt-safety, malloc-interposition, dlsym, phase12-dod]

requires:
  - phase: 12-01
    provides: ScopedAudioThreadGuard + tl_is_audio_thread (RT-safety infrastructure)
  - phase: 12-02
    provides: pluginval strictness-10 green + P12D2/P12D2b APVTS/DSP boundary tests
provides:
  - P12D3: 64-voice polyphony stress — saturates VoicePool, 500 stability blocks, finite-at-block-0
  - P12D4: zero-RT-allocation proof — malloc/calloc/realloc/free interposition via dlsym(RTLD_NEXT), rt_alloc_count==0 over 100 measured blocks
  - Phase 12 DoD gate closed: all 5 [dod] tests green
affects: [Phase 13 — CI matrix, release packaging]

tech-stack:
  added: [dlsym(RTLD_NEXT) malloc interposition, 512-byte BSS bootstrap buffer, libdl]
  patterns: [bootstrap bump allocator protects against dlsym recursion; free() guard for non-heap pointers]

key-files:
  created: [tests/rt_alloc_counter.h, tests/rt_alloc_counter.cpp]
  modified: [tests/test_phase12_dod.cpp, tests/CMakeLists.txt]

key-decisions:
  - "dlsym(RTLD_NEXT) not operator new: avoids ODR conflict with JUCE_CHECK_MEMORY_LEAKS=1"
  - "512-byte bootstrap buffer: dlsym itself calls malloc during init; bump allocator survives recursion"
  - "free() must be overridden: system free() on BSS pointer = UB; bootstrap range check skips them"
  - "4×note36 not notes36-43: only pad 0 has sample; notes 37-43 yield trigger_at nullptr (0 DSP)"
  - "Finite check at block 0, not block 499: test_kick.wav decays to silence in ~0.5s"

patterns-established:
  - "RT alloc test pattern: setup block (unmeasured) → counter reset → N measured blocks → assert 0"
  - "Voice stress pattern: k_pool_size × noteOn on same note/pad → saturate pool in one block"

duration: ~90min (including audit + prior-context resume)
started: 2026-06-19T11:30:00Z
completed: 2026-06-19T12:30:00Z
description: "Phase 12 DoD closed: 64-voice polyphony stress (P12D3) + zero RT allocation (P12D4, rt_alloc_count==0) — 256/256 tests green"
type: Summary
about: "BAQUE"
---

# Phase 12 Plan 03: 64-voice Stress + Zero-RT-Alloc + Phase 12 DoD Summary

**Phase 12 DoD closed: 64-voice polyphony stress (P12D3) + zero RT allocation proof (P12D4, rt_alloc_count==0) — 256/256 tests green**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~90min |
| Started | 2026-06-19T11:30:00Z |
| Completed | 2026-06-19T12:30:00Z |
| Tasks | 3 completed |
| Files created | 2 |
| Files modified | 2 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: P12D3 — 64-voice polyphony, no crash, finite output | Pass | 64 noteOns for note 36 (pad 0) at offsets 0-63; finite check immediately after block 0; 500 stability blocks; 0.05s |
| AC-2: P12D4 — zero heap allocation in audio thread | Pass | rt_alloc_count == 0 across 100 measured blocks; 4×note36 setup (real DSP); 0.03s |
| AC-3: Phase 12 DoD — all [dod] tests pass | Pass | 5/5: P12D1 ✅ P12D2 ✅ P12D2b ✅ P12D3 ✅ P12D4 ✅ |

## Accomplishments

- Phase 12 DoD fully closed: pluginval strictness-10 green (12-02) + 64-voice stress (P12D3) + zero-alloc proof (P12D4)
- malloc/calloc/realloc/free interposition via `dlsym(RTLD_NEXT)` works under LTO (97 LTRANS jobs) — interposition not defeated by link-time optimization
- 512-byte BSS bootstrap buffer protects against dlsym recursion: dlsym itself calls malloc; bump allocator survives; system free() protected from non-heap pointers by range check
- 256/256 tests green; no regressions in prior 254 tests

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Task 1+2+3: P12D3 + P12D4 + DoD gate | `16766b8` | feat(12): Plan 12-03 APPLY — 64-voice stress + zero-RT-alloc + Phase 12 DoD gate |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `tests/rt_alloc_counter.h` | Created | Extern declaration of `RtSafety::rt_alloc_count` (std::atomic<int>) |
| `tests/rt_alloc_counter.cpp` | Created | malloc/calloc/realloc/free wrappers via dlsym(RTLD_NEXT); 512-byte bootstrap buffer; free() range guard |
| `tests/test_phase12_dod.cpp` | Modified | `#include "rt_alloc_counter.h"` added; P12D3 + P12D4 TEST_CASEs appended |
| `tests/CMakeLists.txt` | Modified | `rt_alloc_counter.cpp` added to sources; `dl` added to target_link_libraries |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `dlsym(RTLD_NEXT)` not `operator new` override | JUCE BAQUE target uses `JUCE_CHECK_MEMORY_LEAKS=1` which defines operator new — ODR conflict at link time | malloc interposition catches all allocations including those from JUCE's own operator new (which calls malloc internally) |
| 512-byte bootstrap buffer (plan had 256) | M1 from audit: extra headroom for dlsym's own calloc/realloc during initialization; 256 was documented but 512 is safer | bootstrap still fits in BSS; no runtime cost |
| `free()` override mandatory | M1 from audit: BSS bump-allocator pointers passed to system free() = UB/crash before any test runs | range check (`p >= s_bootstrap_buf && p < end`) → no-op for bootstrap ptrs, real_free for all others |
| 4×note36 for P12D4 setup (plan said notes 36-43) | SR1 from audit: pads 1-7 have no sample in test fixture; `trigger_at` returns nullptr → 0 DSP work; only pad 0 is real | 4 voices exercise full FxChain/scatter/tape_stop accumulation path with real audio output |
| Finite check at block 0 (plan said block 499) | SR2 from audit: test_kick.wav decays to silence in ~0.5s; isfinite(0.0f) at block 499 is trivially true regardless of bugs | checking immediately after block 0 with 64 active voices = genuinely non-vacuous coverage |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed (from 12-03 audit) | 3 | Essential correctness fixes — no scope creep |
| Plan template vs actual | 2 minor | Bootstrap size 256→512; `std::memset`+`<cstring>` vs `__builtin_memset` |
| Scope reduction | 1 | `src/rt_safety.h` listed in `files_modified` frontmatter — no changes needed (tl_is_audio_thread already present) |

### Auto-fixed Issues (applied during audit before APPLY)

**1. M1: free() override missing → UB/crash**
- Found during: 12-03 audit (M1)
- Issue: Bootstrap bump allocator returns pointers in `s_bootstrap_buf` (BSS); system `free()` on non-heap pointer = UB
- Fix: Added `extern "C" void free(void*)` with range check; bootstrap pointers silently skipped
- Files: `tests/rt_alloc_counter.cpp`

**2. SR1: P12D4 setup used notes 36-43 → 0 DSP work**
- Found during: 12-03 audit (SR1)
- Issue: Pads 1-7 have no sample in test fixture; `VoicePool::trigger_at()` returns nullptr for unmapped pads → no FxChain/scatter/gater processing
- Fix: 4×noteOn for note 36 (pad 0) at offsets 0-3 — same pad retriggers, 4 real DSP voices
- Files: `tests/test_phase12_dod.cpp`

**3. SR2: P12D3 finite check at block 499 → vacuous**
- Found during: 12-03 audit (SR2)
- Issue: test_kick.wav decays to silence in ~0.5s (~42 blocks at 512 samples/44100Hz); isfinite(0.0f) always true
- Fix: Moved finite check to immediately after block 0 when all 64 voices are actively mixing
- Files: `tests/test_phase12_dod.cpp`

### Deferred Items

- P12D4 mallinfo2() fallback (documented in plan boundaries) — not needed; dlsym interposition worked under LTO

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| LTO concern: 97 LTRANS jobs might inline glibc malloc calls | No issue in practice — `dlsym(RTLD_NEXT)` interposition holds under LTO on Linux/glibc; build exits 0, P12D4 passes with count==0 |

## Next Phase Readiness

**Ready:**
- Phase 12 DoD fully met: pluginval strictness-10 ✅ + P12D1-P12D4 ✅ + 256/256 tests ✅
- RT-safety harness (tl_is_audio_thread + rt_alloc_counter) in place for regression detection in Phase 13
- pluginval CI at strictness 10 on all 3 platforms (Linux/macOS/Windows) from 12-02

**Concerns:**
- P12D4 malloc interposition is Linux/glibc-specific (`dlsym` + `RTLD_NEXT`); macOS and Windows CI does not run P12D4 equivalent — audio-thread allocation drift on those platforms undetected by automated tests

**Blockers:**
- None — Phase 12 complete; Phase 13 routing next

---
*Built with PAUL Framework v1.4*
*Phase: 12-hardening, Plan: 03*
*Completed: 2026-06-19*
