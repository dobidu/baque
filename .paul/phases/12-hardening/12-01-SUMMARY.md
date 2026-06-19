---
phase: 12-hardening
plan: 01
subsystem: audio
tags: [rt-safety, processblock, thread-local, audit, stress-test]

requires:
  - phase: 11-presets-library
    provides: Full plugin state (252 tests baseline), preset system, all audio engine phases

provides:
  - RtSafety::ScopedAudioThreadGuard — thread-local RT flag RAII infrastructure
  - ScopedAudioThreadGuard at processBlock entry (tl_is_audio_thread lifecycle)
  - Verified-clean processBlock call graph: 0 RT-path allocations across 14 source files
  - P12D1 stability harness: 1000 blocks × 16 voices, 252/252 tests

affects: [12-02, 12-03, pluginval, phase-13]

tech-stack:
  added: [src/rt_safety.h (inline thread_local C++17)]
  patterns: [ScopedAudioThreadGuard at processBlock entry, grep audit checklist per plan]

key-files:
  created: [src/rt_safety.h, tests/test_phase12_dod.cpp]
  modified: [src/plugin_processor.cpp, tests/CMakeLists.txt]

key-decisions:
  - "ensureSize(512) pre-existing at prepareToPlay():91-92 — AC-2 pre-satisfied, no change needed"
  - "operator new override deferred: ODR conflict with JUCE_CHECK_MEMORY_LEAKS=1 when linking pre-compiled BAQUE"
  - "dispatch_ui_command() (~30 cases, struct field writes + jlimit bounds) confirmed RT-safe by inspection"

patterns-established:
  - "tl_is_audio_thread flag: future jassert-on-alloc hook can check this without instrumentation overhead"
  - "P12D1 pattern: REQUIRE setup check BEFORE processBlock (in-place midi_messages replacement makes post-block check vacuous)"

duration: ~2h (including WSL2 cmake stale-state recovery)
started: 2026-06-19T06:00:00Z
completed: 2026-06-19T09:00:00Z
description: "RT-safety infrastructure (ScopedAudioThreadGuard + thread-local flag) + zero-violation processBlock call graph audit + P12D1 stability harness (1000 blocks × 16 voices, 252/252)"
type: Summary
about: "BAQUE"
---

# Phase 12 Plan 01: RT-Safety Audit + ScopedAudioThreadGuard — Summary

**RT-safety infrastructure added (ScopedAudioThreadGuard), processBlock call graph audited clean (0 violations across 14 files), P12D1 stability test proves 1000-block × 16-voice operation; 252/252 tests green.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2h |
| Started | 2026-06-19T06:00:00Z |
| Completed | 2026-06-19T09:00:00Z |
| Tasks | 2 completed |
| Files created | 2 (`src/rt_safety.h`, `tests/test_phase12_dod.cpp`) |
| Files modified | 2 (`src/plugin_processor.cpp`, `tests/CMakeLists.txt`) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: processBlock call-graph 0 RT violations | Pass | 14 files audited; `time_stretch.cpp output.insert` + `slicer.cpp push_back` confirmed off-path; `dispatch_ui_command()` confirmed RT-safe |
| AC-2: MidiBuffer members pre-sized in prepareToPlay() | Pass | Pre-existing at lines 91-92 (`midi_buffer_seq_.ensureSize(512)` + `midi_buffer_ext_.ensureSize(512)`) — no change required |
| AC-3: ScopedAudioThreadGuard at processBlock entry; RAII verified | Pass | `rt_guard` declared as first line of processBlock body; flag true for block duration, false after return |
| AC-4: P12D1 — 1000 blocks × 16 voices, no crash, RAII verified | Pass | 252/252 pass; 1000 REQUIRE_FALSE(tl_is_audio_thread) assertions; REQUIRE(midi_first.getNumEvents()==16) honest pre-loop setup check |

## Accomplishments

- **`src/rt_safety.h`**: `RtSafety::ScopedAudioThreadGuard` (RAII, `inline thread_local bool tl_is_audio_thread`, C++17 static TLS, zero heap alloc) + `JUCE_DECLARE_NON_COPYABLE`. Infrastructure for future jassert-on-alloc hook without instrumentation overhead.
- **processBlock RT guard**: `ScopedAudioThreadGuard rt_guard;` as first line of `BaqueProcessor::processBlock()` — flag covers entire block duration including MIDI drain, FX chain, scatter, granular, MIDI clock output.
- **Call-graph audit clean**: 14 source files reachable from processBlock audited; 0 RT-path violations. Known safe off-path hits: `time_stretch.cpp:37 output.insert()` (offline stretch, message thread), `slicer.cpp:9,34 push_back` (message-thread auto-slice), `plugin_processor.cpp new BaqueEditor/new BaqueProcessor` (createEditor/createPluginFilter). `dispatch_ui_command()` (~30 switch cases, struct field writes + jlimit bounds) confirmed RT-safe.
- **P12D1 stability harness**: 1000 blocks at 44100 Hz / 512 samples (~11.6 s audio time), 16 simultaneous voices (notes 36–51), no crash or abort. RT guard RAII verified by 1000 `REQUIRE_FALSE(tl_is_audio_thread)` assertions. Setup check `REQUIRE(midi_first.getNumEvents()==16)` placed before block 0 (avoids vacuous check — processBlock replaces midi_messages in-place).

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Task 1 + Task 2 (combined) | `17c0f03` | feat(12): Plan 12-01 APPLY — ScopedAudioThreadGuard + RT audit + P12D1 |
| PLAN | `99c320d` | plan(12): 12-01 PLAN |
| AUDIT | `2e4aa47` | docs(12): 12-01 AUDIT report |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/rt_safety.h` | Created | `RtSafety::ScopedAudioThreadGuard` + `tl_is_audio_thread` thread-local flag |
| `src/plugin_processor.cpp` | Modified | `#include "rt_safety.h"` + `ScopedAudioThreadGuard rt_guard;` at processBlock entry |
| `tests/test_phase12_dod.cpp` | Created | P12D1: 1000-block × 16-voice stability + RT guard RAII harness |
| `tests/CMakeLists.txt` | Modified | Added `test_phase12_dod.cpp` to baque_tests executable |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| ensureSize(512) unchanged | Already present at prepareToPlay():91-92 — AC-2 pre-satisfied | No code diff needed; confirms prior plan already addressed primary alloc risk |
| operator new override deferred | baque_tests links pre-compiled BAQUE which has `JUCE_CHECK_MEMORY_LEAKS=1` (defines operator new); user-defined override → ODR violation at link | Noted in plan boundaries; Phase 12-02/12-03 can revisit if JUCE_CHECK_MEMORY_LEAKS=0 option added to test CMakeLists |
| dispatch_ui_command() verified RT-safe by inspection | ~30 switch cases all do struct field writes + jlimit bounds clamps; no string construction, no allocation, no lock | Documented explicitly so future contributors don't add allocating calls to this path without awareness |
| P12D1 check before block 0 (M1 audit fix) | processBlock clears midi_messages and replaces with EXT MIDI in-place; post-block check would be vacuous | Established pattern for future processBlock tests: verify MIDI setup before first processBlock call |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Pre-satisfied | 1 | None — less code change than planned |
| Build issue (resolved) | 1 | ~20min delay; cmake WSL2 clock skew |

**Total impact:** Positive — one fewer file change (AC-2 pre-satisfied); build issue resolved without scope change.

### Pre-satisfied Items

**AC-2: MidiBuffer ensureSize already present**
- **Found during:** Task 1 Step 3 (reading prepareToPlay)
- **Issue:** Plan specified adding ensureSize(512); code already had it at lines 91-92
- **Action:** No change; noted in SUMMARY only
- **Verification:** `Read plugin_processor.cpp offset:88 limit:10` — confirmed exact lines

### Issues Encountered

| Issue | Resolution |
|-------|------------|
| `No rule to make target 'tests/CMakeFiles/baque_tests.dir/link.txt'` after adding test file | Deleted stale `baque_tests.dir` then re-ran cmake reconfigure; WSL2 clock skew caused incomplete Makefile generation |

## Next Phase Readiness

**Ready:**
- `tl_is_audio_thread` flag available for future operator new hook (Phase 12-03 or post-v1)
- processBlock call graph documented clean — baseline for pluginval (12-02) and 64-voice stress (12-03)
- P12D1 harness provides regression baseline for RT-path changes

**Concerns:**
- operator new override not yet wired — allocations in processBlock caught by tl_is_audio_thread only via explicit jassert calls, not automatically; deferred to post-v1 or test infra refactor
- WSL2 clock skew is endemic; reconfigure-after-new-source-file pattern required (cmake stale Makefile → delete baque_tests.dir → reconfigure)

**Blockers:** None.

---
*Built with PAUL Framework v1.4 · https://chrisai.cv/skool*
*Phase: 12-hardening, Plan: 01*
*Completed: 2026-06-19*
