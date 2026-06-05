---
phase: 04-sample-engine
plan: "04"
subsystem: audio
tags: [soundtouch, wsola, time-stretch, fetchcontent, cmake, offline-dsp]

requires:
  - phase: 04-01
    provides: SamplePad single-writer contract, pool.reset_all() before buffer mutation
  - phase: 04-03
    provides: chop_to_pads offline bake-in pattern (allocates, pool.reset_all() first)

provides:
  - TimeStretch::apply() — offline tempo-only stretch (WSOLA via SoundTouch fork)
  - SoundTouch fork FetchContent integration (LGPL v2.1, separate repo)
  - Phase 4 DoD: varispeed + offline time-stretch + chop-to-pads all present

affects: [phase-7-granular, phase-10-ui, r&d-ts-fork]

tech-stack:
  added: [SoundTouch 2.3.3 (fork), FetchContent GIT_SHALLOW]
  patterns: [offline-bake-in, input-snapshot-before-setSize, pool-reset-before-mutation]

key-files:
  created: [src/audio/time_stretch.h, src/audio/time_stretch.cpp, tests/test_time_stretch.cpp]
  modified: [CMakeLists.txt, tests/CMakeLists.txt]

key-decisions:
  - "SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS required: STTypes.h auto-enables SSE on x86_64"
  - "input snapshot before setSize: pad.data() pointer invalidated by AudioBuffer reallocation"
  - "pool.reset_all() before snapshot (single-writer contract); no-op guards skip reset"

patterns-established:
  - "Offline bake-in: pool.reset_all(), snapshot input to vector, process, guard empty output, write back"
  - "WSOLA minimum input: < 256 samples returns without processing (STTypes threshold)"

duration: ~2h
started: 2026-06-05T21:00:00Z
completed: 2026-06-05T23:30:00Z
description: "SoundTouch fork v1 integrated via FetchContent; TimeStretch::apply() bakes offline tempo stretch into SamplePad; 5 new tests; 59/59 pass"
type: Summary
about: "BAQUE"
---

# Phase 4 Plan 04: R&D-TS Fork v1 Integration Summary

**SoundTouch fork (LGPL v2.1) integrated via FetchContent; `TimeStretch::apply()` bakes offline WSOLA tempo stretch into SamplePad; fork hotfix for SSE auto-detection; 59/59 tests pass**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2h |
| Started | 2026-06-05T21:00:00Z |
| Completed | 2026-06-05T23:30:00Z |
| Tasks | 3 completed (checkpoint + Task 2 + Task 3) |
| Files modified | 5 (2 created src, 1 created test, 2 modified cmake) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: ratio 0.5 roughly doubles output length | Pass | T1: 8192 → ~16000–18000 samples |
| AC-2: ratio 2.0 roughly halves output length | Pass | T2: 8192 → ~3000–5000 samples |
| AC-3: pool.reset_all() called on apply | Pass | T3: process_all silence after stretch |
| AC-4a: empty pad → no-op | Pass | T4 |
| AC-4b: ratio=1.0 → no-op | Pass | T5 |
| AC-4c: ratio≤0 → no-op | Pass | T5 |
| AC-4d: < 256 samples → no-op | Pass | T5 (audit-added guard) |

## Accomplishments

- `TimeStretch::apply()` wraps SoundTouch WSOLA with correct `setTempo()` (not `setRate()`) — pitch-independent duration change
- Input snapshot pattern enforced: `pad.data()` copied to vector before `setSize()` invalidates pointer
- Fork hotfix: `SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS` added to baque-soundtouch fork — prevents vtable link errors from STTypes.h SSE auto-detection on x86_64
- 5 tests (T1–T5) cover all AC including audit-added guards; `TimeStretchJuceFixture` defined locally (not cross-TU)
- All 59/59 tests pass; clang-format clean

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/time_stretch.h` | Created | TimeStretch class — offline apply() API |
| `src/audio/time_stretch.cpp` | Created | WSOLA wrap: guards, snapshot, SoundTouch, drain, write-back |
| `tests/test_time_stretch.cpp` | Created | 5 tests: T1-T5 (length ratios, pool reset, edge cases) |
| `CMakeLists.txt` | Modified | FetchContent(SoundTouch), time_stretch.cpp in sources, SoundTouch linked |
| `tests/CMakeLists.txt` | Modified | test_time_stretch.cpp added, SoundTouch linked |
| `https://github.com/dobidu/baque-soundtouch` | Fork hotfix | SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS added to CMakeLists |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS` in fork | STTypes.h auto-defines SSE on x86_64 (`__x86_64__`) → TDStretchSSE/FIRFilterSSE declared → vtable link errors without sse_optimized.cpp | Phase 7 R&D must re-evaluate when adding SIMD |
| `setTempo(ratio)` not `setRate()` | setRate changes pitch+duration linked; setTempo changes duration only (WSOLA path) | Correct pitch-independent stretch |
| `pool.reset_all()` before input snapshot | Single-writer contract: no reads while voices may be reading | Guards (empty/ratio=1.0/ratio≤0) placed BEFORE reset — no-ops never kill voices |
| No-op if `num_input < 256` | SoundTouch WSOLA needs minimum frames to emit output; < 256 would hit empty-output guard silently | Explicit early return avoids jassertfalse on very short pads |
| `jassert(!output.empty())` before return | Silent empty-output = voices killed but pad unchanged — debug builds catch this with clear callstack | Audit SR1 |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | Fork hotfix — no interface change |
| Audit-added guards | 3 | min_input, jassert empty output, T3 buffer 8192 |
| Deferred | 0 | — |

**Total impact:** Essential build fix + audit hardening. No scope change.

### Auto-fixed: Fork SSE Vtable Link Error

- **Found during:** Task 3 (first `cmake --build baque_tests`)
- **Issue:** `undefined reference to vtable for soundtouch::TDStretchSSE` — `STTypes.h:109` auto-defines `SOUNDTOUCH_ALLOW_X86_OPTIMIZATIONS=1` on x86_64 (checks `__x86_64__`), which then sets `SOUNDTOUCH_ALLOW_SSE=1`, causing class declarations in TDStretch.h/FIRFilter.h. Without `sse_optimized.cpp` in the build, vtables missing.
- **Fix:** Added `SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS` to fork's `target_compile_definitions`. Force-updated `v1.0.0-baque` tag. Cleared FetchContent cache, re-configured.
- **Verification:** baque_tests linked clean; 59/59 pass.

### Audit additions applied:
- SR1: `jassert(!output.empty())` — debug diagnostic
- SR2: `if (num_input < 256) return;` — explicit min-input guard  
- SR3: T3 buffer 4096→8192 — stay above WSOLA boundary
- M1: `TimeStretchJuceFixture` local struct — cross-TU compile fix
- M2: `(void)pool.trigger_at(...)` — nodiscard discard

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| Fork SSE vtable link error | SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS in fork CMakeLists; force-updated v1.0.0-baque tag |

## Next Phase Readiness

**Ready:**
- Phase 5 Feel Engine: VoicePool, PadBank, Scheduler all stable; RT-safe foundation solid
- Phase 7 R&D-TS: fork structure in place; Phase 7 upgrades SIMD + transient preservation
- UI wiring (Phase 10): `TimeStretch::apply()` is a simple one-call API from any background thread

**Concerns:**
- Fork's `v1.0.0-baque` tag was force-updated — any cached FetchContent build will need `build/_deps/soundtouch-*` cleared if switching machines
- Phase 7 re-enabling SIMD must add `sse_optimized.cpp` + `mmx_optimized.cpp` back to fork CMakeLists AND remove `SOUNDTOUCH_DISABLE_X86_OPTIMIZATIONS`

**Blockers:**
- None

---
*Phase: 04-sample-engine, Plan: 04*
*Completed: 2026-06-05*
