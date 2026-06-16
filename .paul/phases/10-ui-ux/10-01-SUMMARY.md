---
phase: 10-ui-ux
plan: 01
subsystem: audio
tags: [spsc-queue, atomic-snapshot, juce-abstractfifo, command-dispatch, processblock]

requires:
  - phase: 09-midi-hardware
    provides: single-writer structs (lane_routing_, cc_out_, clock_master_, plock_pattern_, perf_state_, pad_bank_) all audio-thread-owned without safe mutation path

provides:
  - UiCommandQueue (256-slot SPSC, zero-alloc after construction, lock-free push/drain)
  - UiStateSnapshot (6 atomic fields: current_step, is_playing, peaks, lane_last_velocity[16], bpm)
  - dispatch_ui_command() covering all 25 UiCommandType cases with bounds clamping
  - Drain at top of processBlock + snapshot publish at end
  - suspendProcessing bracket in getState/setState with message-thread pre-drain
  - Integration tests UI1-UI8 driving real processBlock

affects: [10-02-lookandfeel, 10-03-perform, 10-04-feel-field, 10-05-fx-mix, 10-06-perf-midi, 10-07-browser-undo]

tech-stack:
  added: [juce::AbstractFifo (SPSC backing), std::atomic per-field snapshot]
  patterns:
    - "SPSC command queue: UI writes via push_ui_command(), audio thread drains at top of processBlock"
    - "Per-field atomic snapshot: each field independently coherent; cross-field tearing acceptable by design"
    - "suspendProcessing bracket: getState/setState bracket + message-thread drain before serialize/restore"
    - "lane_last_velocity derived from midi_buffer_seq_ note-ons (SR1): auto-respects mute/fill-gate, Sequencer stays UI-agnostic"

key-files:
  created:
    - src/audio/ui_command_queue.h
    - src/audio/ui_state_snapshot.h
    - tests/test_ui_command_queue.cpp
    - tests/test_ui_commands_integration.cpp
  modified:
    - src/plugin_processor.h
    - src/plugin_processor.cpp
    - src/audio/perf_state.h
    - src/audio/lane_routing.h
    - src/audio/midi_cc.h
    - src/audio/pad_bank.h
    - tests/CMakeLists.txt

key-decisions:
  - "AbstractFifo(N) holds N-1 items: array sized k_capacity+1 to expose exactly 256 usable slots"
  - "apply_template invalid id: jassert+ignore (not clamp) — wrong template is worse than no template (SR2)"
  - "set_step_velocity: no-op in v1 (StepPattern has no per-step velocity field); deferred to 10-03"
  - "lane_last_velocity only updates for internal lanes: EXT-only note-ons go to midi_buffer_ext_ not midi_buffer_seq_"
  - "State version bumped 3→4 to serialize lane_routing_, clock_master_, cc_out_ (newly audio-owned)"

patterns-established:
  - "All live engine mutation flows through push_ui_command() → drain() — no direct struct writes from UI components after this plan"
  - "Snapshot publish is the last thing in processBlock (after tape_stop_, after MIDI merge)"
  - "Integration tests MUST drive real processBlock — never Sequencer::generate alone (08-02/09-04 lesson)"

duration: 2 sessions (~4h total)
started: 2026-06-15T00:00:00Z
completed: 2026-06-15T00:00:00Z
description: "UI→engine SPSC command queue + atomic UiStateSnapshot retire all Phase 4/8/9 single-writer contracts before any UI component exists"
type: Summary
about: "BAQUE"
---

# Phase 10 Plan 01: UI Command Queue + UiStateSnapshot Summary

**UI→engine SPSC command queue (256 slots, zero-alloc) + per-field atomic snapshot retire all Phase 4/8/9 single-writer contracts before any UI component exists. 221/221 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~4h (2 sessions) |
| Started | 2026-06-15 |
| Completed | 2026-06-15 |
| Tasks | 3 completed |
| Files created | 4 |
| Files modified | 7 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: RT-safe SPSC command queue | Pass | QC1-QC4 verify FIFO order, push-on-full returns false, drain-on-empty is no-op, interleaved push/drain correct |
| AC-2: Every targeted struct mutable via command | Pass | 25 command types dispatched; UI1-UI7 prove drain precedes generate and output reflects command |
| AC-3: apply_template through queue with 09-04 semantics | Pass | UI3 verifies routing/cc match TR-8, trig activations preserved (SR1), note rewritten to BD=36 |
| AC-4: Snapshot readable per-field without torn values | Pass | UI6 verifies current_step in [0,16), is_playing=true, bpm=120, peak>0, lane_last_velocity[0]=100 |
| AC-5: State save/load race-free | Pass | UI8 pushes commands, calls getStateInformation without processBlock, restores into fresh processor — lane 3 external/ch7 survives round-trip |

## Accomplishments

- Retired every Phase 4/8/9 single-writer contract: `perf_state_`, `lane_routing_`, `clock_master_`, `cc_out_`, pad params, pattern steps, plock pattern are now exclusively mutable via `push_ui_command()` from the message thread
- `UiCommandQueue`: lock-free SPSC with `juce::AbstractFifo`, 256 slots, trivially-copyable `UiCommand` POD — zero allocation after construction
- `UiStateSnapshot`: 6 per-field `std::atomic` fields (current_step, is_playing, peaks L/R, lane_last_velocity[16], bpm) published at end of every processBlock; UI can read any field safely at any time
- `getStateInformation`/`setStateInformation` upgraded with `suspendProcessing` bracket + message-thread pre-drain — state saves now reflect all pending commands (AC-5 M1)
- State version bumped 3→4 with backward-compatible load path for pre-v4 presets

## Task Commits

Tasks executed in-session without atomic commits (PAUL autonomous track). Files modified atomically from conversation context.

| Task | Description |
|------|-------------|
| Task 1 | `ui_command_queue.h` (UiCommandType×25 + UiCommand POD + UiCommandQueue SPSC), `test_ui_command_queue.cpp` (QC1-QC4), `CMakeLists.txt` |
| Task 2 | `plugin_processor.h/.cpp`: drain at top of processBlock, `dispatch_ui_command()` (25 cases, jlimit clamping), getState/setState suspend bracket + drain; single-writer comments updated in 4 headers; k_state_version 3→4 |
| Task 3 | `ui_state_snapshot.h` (UiStateSnapshot 6 atomics), snapshot publish at end of processBlock, `test_ui_commands_integration.cpp` (UI1-UI8), `CMakeLists.txt` updated |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/ui_command_queue.h` | Created | UiCommandType enum (25 values), UiCommand POD, UiCommandQueue SPSC (AbstractFifo, 256+1 buffer) |
| `src/audio/ui_state_snapshot.h` | Created | UiStateSnapshot: 6 per-field atomics published every processBlock |
| `src/plugin_processor.h` | Modified | Added `ui_commands_` + `ui_snapshot_` members, `push_ui_command()` + `ui_snapshot()` accessors, `dispatch_ui_command()` private, k_state_version 4 |
| `src/plugin_processor.cpp` | Modified | `dispatch_ui_command()` 25-case switch; drain at top of processBlock; snapshot publish at end; getState/setState suspend+drain brackets; state v4 serialize/restore; `<cmath>/<algorithm>` includes |
| `src/audio/perf_state.h` | Modified | Single-writer comment → references push_ui_command(set_fill/set_mute/set_solo) |
| `src/audio/lane_routing.h` | Modified | Single-writer comment → references push_ui_command(set_lane_mode/set_lane_channel) |
| `src/audio/midi_cc.h` | Modified | Single-writer comment → references push_ui_command(set_cc_out_*/set_cc_slot) |
| `src/audio/pad_bank.h` | Modified | Single-writer comment → references push_ui_command(set_pad_*); buffer stays safe-load protocol |
| `tests/test_ui_command_queue.cpp` | Created | QC1: FIFO wrap-around; QC2: push-on-full returns false; QC3: drain-on-empty no-op; QC4: interleaved push/drain |
| `tests/test_ui_commands_integration.cpp` | Created | UI1-UI8: real processBlock integration tests proving all 5 ACs |
| `tests/CMakeLists.txt` | Modified | Added both new test files to baque_tests executable |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| AbstractFifo internal buffer k_capacity+1 | JUCE AbstractFifo(N) holds N-1 items (bookkeeping slot); `+1` exposes exactly 256 usable slots — caught by QC2 FAIL during Task 1 | All future queue capacity math must account for this |
| apply_template invalid id: jassert+ignore (not clamp) | SR2 audit: clamping to 0 would silently apply TR-8 template when user intended something else — a wrong template destroys routing; ignore is safer | integrate template ID validation in UI layer |
| lane_last_velocity from midi_buffer_seq_ not Sequencer internals | SR1 audit: auto-respects 08-04 mute/fill-gate (suppressed step emits no note-on); keeps Sequencer UI-agnostic | EXT-only lanes do not pulse (v1 documented limitation; revisit in 10-03) |
| set_step_velocity: no-op | StepPattern has no per-step velocity field in v1; deferred to 10-03 | 10-03 must add velocity field and wire the command |
| State version 3→4 | lane_routing_, clock_master_, cc_out_ are now audio-thread-owned and must be serialized to state | pre-v4 presets load with defaults for new fields (backward compat preserved) |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | AbstractFifo off-by-one: critical, caught before merge |
| No deviations | 0 | Plan executed exactly as written |

### Auto-fixed Issues

**1. AbstractFifo N-1 capacity (QC2 FAIL during Task 1)**
- **Found during:** Task 1 (test_ui_command_queue.cpp)
- **Issue:** `AbstractFifo(256)` holds 255 items (one slot reserved for bookkeeping). QC2 pushed 256 items expecting full queue, but only 255 succeeded — the 256th push returned true instead of false.
- **Fix:** Changed `fifo_(k_capacity)` → `fifo_(k_capacity + 1)` and `std::array<UiCommand, k_capacity>` → `std::array<UiCommand, k_capacity + 1>` with a comment explaining the invariant.
- **Files:** `src/audio/ui_command_queue.h`
- **Verification:** QC2 went from FAIL to PASS; all QC1-QC4 pass

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| CI OOM on ubuntu-latest (exit 143, SIGTERM) | Fixed before Task 1: split build step — Linux uses `-j$(nproc)` (=2 on 2-core runner), others keep bare `-j`; merged in PR #1 |
| Windows CI VS generator mismatch (v17→v18) | Fixed before Task 1: added cleanup step removing stale `_build`/`_subbuild` dirs after cache restore; merged in PR #1 |
| WSL2 clock skew | Resolved with `touch` on modified files before rebuild |

## Next Phase Readiness

**Ready:**
- All single-writer contracts retired: `push_ui_command()` is the sole live-mutation path; 10-02 UI components can call it directly without any additional threading work
- `ui_snapshot()` provides a safe read path for all display values: transport, peaks, per-lane velocity, bpm — 10-03 PERFORM screen and 10-04 Feel Field can bind to it immediately
- Integration test pattern established: UI1-UI8 drive real `processBlock` — future plan tests should follow this pattern
- State version 4 with v3 backward-compat load: preset files from Phase 9 will load correctly

**Concerns:**
- EXT-only lanes do not contribute to `lane_last_velocity` — visible gap in the PERFORM grid for hardware-routed lanes. Revisit in 10-03.
- Queue-full UI policy (what to show user when 256 commands back up) deferred to 10-02.
- `set_step_velocity` is a no-op until StepPattern gains a velocity field in 10-03.

**Blockers:** None

---
*Phase: 10-ui-ux, Plan: 01*
*Completed: 2026-06-15*
