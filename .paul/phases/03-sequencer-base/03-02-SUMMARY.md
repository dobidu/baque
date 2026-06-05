---
phase: 03-sequencer-base
plan: 02
subsystem: dsp
tags: [sequencer, swing, pattern-switch, note-tracker, atomic, midi, catch2]

requires:
  - phase: 03-sequencer-base/03-01
    provides: StepPattern, StepClock, Sequencer::generate(), last_step_fired_, 16×16 grid

provides:
  - StepClock swing (std::atomic<float>, MPC 50-75%, relaxed memory order)
  - Sequencer::set_next_pattern() (release/acquire pattern, 15→0 bar boundary swap)
  - NoteTracker (lane→last_triggered_note, fallback to pattern note on first block)
  - Sequencer::set_swing() delegation to StepClock
  - Phase 3 DoD fully verified: 21/21 tests pass
affects: [04-sample-engine, 05-feel-engine, all-phases]

tech-stack:
  added: [std::atomic<bool> pattern_pending_, std::atomic<float> swing_amount_]
  patterns:
    - "Atomic release/acquire for cross-thread StepPattern transfer: write data then store(true, release); load(acquire) before reading"
    - "NoteTracker fallback to pattern.get_note() when tracker=0 (first block, no note triggered yet)"
    - "Swing applied in sample_offset_of_step_start() for odd step indices only (1,3,5... = off-beat)"
    - "Pattern swap at step 15→0 transition (not ppq epsilon) — deterministic, float-comparison-free"

key-files:
  created:
    - src/audio/note_tracker.h
    - src/audio/note_tracker.cpp
  modified:
    - src/audio/step_clock.h (std::atomic<float> swing_amount_, set_swing())
    - src/audio/step_clock.cpp (set_swing() + swing in sample_offset_of_step_start())
    - src/audio/sequencer.h (next_pattern_, std::atomic<bool> pattern_pending_, NoteTracker, set_next_pattern(), set_swing())
    - src/audio/sequencer.cpp (set_next_pattern(), pattern swap, NoteTracker integration, swing via clock)
    - src/plugin_processor.h (set_next_pattern() public API)
    - CMakeLists.txt (note_tracker.cpp)
    - tests/test_sequencer.cpp (4 DoD tests)

key-decisions:
  - "NoteTracker fallback to pattern note (not skip note-off): avoids stuck note on first block when tracker=0"
  - "Pattern swap via step 15→0 not ppq: deterministic, no floating-point epsilon needed"
  - "std::atomic<float> swing with relaxed order: single value, no cross-variable ordering needed"
  - "swing applied to odd-indexed steps (1,3,5...): convention matches MPC 'off-beat' feel"

patterns-established:
  - "Cross-thread data: write payload before store(release); load(acquire) before reading payload"
  - "NoteTracker always initialized to 0; caller must provide fallback for first-block note-off"
  - "StepClock methods remain callable as methods (not static) despite no state — allows future non-atomic members"

duration: ~3h
started: 2026-06-05T06:00:00Z
completed: 2026-06-05T08:00:00Z
description: "Global swing (std::atomic float, relaxed), seamless pattern switch (release/acquire at step 15→0), NoteTracker per-lane note-off; Phase 3 DoD verified: 21/21 tests"
type: Summary
about: "BAQUE"
---

# Phase 3 Plan 02: Swing + Pattern Switch + NoteTracker Summary

**Global swing (std::atomic float, relaxed), seamless pattern switch (release/acquire at step 15→0), NoteTracker per-lane note-off; Phase 3 DoD verified: 21/21 tests.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~3h |
| Started | 2026-06-05T06:00:00Z |
| Completed | 2026-06-05T08:00:00Z |
| Tasks | 3 completed |
| Files created | 2 |
| Files modified | 7 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Global swing shifts even 16th notes | Pass | Test "swing delays even steps": step 1 offset = 5512+1874 = 7386 samples; step 0 = 5512 (no swing) |
| AC-2: Pattern switch at bar boundary without glitch | Pass | Test "pattern switch happens at step 0": new pattern takes effect on 15→0 transition; no events from old pattern after swap |
| AC-3: Note-off fires for correct note number per lane | Pass | Test "sequencer note-off uses tracker note not fixed fallback": note-off matches note-on (55 in test) via NoteTracker |
| AC-4: Phase 3 DoD verified | Pass | 21/21 ctest pass; pluginval strictness 5 PASS |

## Accomplishments

- Thread-safe swing via `std::atomic<float>` with `memory_order_relaxed` — correct for single float, eliminates unnecessary fences
- Pattern swap with full release/acquire pair: write `next_pattern_` → store `pattern_pending_(release)` → load `pattern_pending_(acquire)` → copy pattern → clear flag. No lock, no race.
- NoteTracker resolves Phase 2 deferred item: note-off now sends correct note per lane; graceful fallback on first block

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Tasks 1+2+3: all files | `9265273` | feat(03-sequencer-base): swing + pattern switch + NoteTracker (03-02) |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/note_tracker.h` | Created | lane→last_triggered_note; 16-element std::array |
| `src/audio/note_tracker.cpp` | Created | note_triggered/get_active_note/reset |
| `src/audio/step_clock.h` | Modified | std::atomic<float> swing_amount_{0.5f}; set_swing() |
| `src/audio/step_clock.cpp` | Modified | set_swing(clamp+store relaxed); swing in sample_offset for odd steps |
| `src/audio/sequencer.h` | Modified | std::atomic<bool> pattern_pending_; next_pattern_; NoteTracker; set_next_pattern/set_swing |
| `src/audio/sequencer.cpp` | Modified | set_next_pattern(release), 15→0 swap(acquire), NoteTracker integration + fallback |
| `src/plugin_processor.h` | Modified | set_next_pattern() public API (wraps sequencer.set_next_pattern) |
| `CMakeLists.txt` | Modified | note_tracker.cpp added |
| `tests/test_sequencer.cpp` | Modified | 4 DoD tests: swing, pattern switch, tracker, correct note-off |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| NoteTracker fallback to pattern.get_note() | First block: tracker=0, note-off was skipped → stuck note | All future audio using NoteTracker must handle first-block case |
| Pattern swap at step 15→0 (not raw step==0) | raw step==0 would fire on first block without prior step 15 | Swap always preceded by at least one full pattern cycle |
| memory_order_relaxed for swing_amount_ | Single float, no ordering dependency with other atomics | All swing-related atomics use relaxed; no additional fence needed |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | Correctness; no scope change |

**Total impact:** One essential fix; no scope creep.

### Auto-fixed Issues

**1. NoteTracker zero on first block causes skipped note-off**
- **Found during:** Task 3 qualify (test 12 regression — "sequencer generates note-on at step boundary")
- **Issue:** `note_tracker_.get_active_note(lane)` = 0 on first block; `if (prev_note != 0)` guard skipped note-off entirely
- **Fix:** Fallback to `pattern_.get_note(lane, prev_step)` when tracker returns 0
- **Files:** src/audio/sequencer.cpp
- **Verification:** 21/21 ctest pass including test 12
- **Commit:** 9265273

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| Test 12 regression after NoteTracker integration | NoteTracker fallback to pattern note on first block |

## Next Phase Readiness

**Ready:**
- Swing applies to steps 1,3,5,7,9,11,13,15 — Phase 5 Feel Engine builds per-step offsets on top of this global swing
- Pattern swap API: `sequencer_.set_next_pattern(p)` — Phase 10 UI can drive this directly
- NoteTracker fully wired — per-note tracking foundation for Phase 4 (choke groups need active-voice tracking)
- Phase 3 DoD proven: 21 tests covering timing, swing math, pattern switch, note-off correctness

**Concerns:**
- MIDI channel still ignored (all channels trigger) — Phase 9 needs channel routing per lane
- NoteTracker stores `uint8_t note` per lane — single note per lane; choke groups (Phase 4) may need per-voice tracking

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 03-sequencer-base, Plan: 02*
*Completed: 2026-06-05*
