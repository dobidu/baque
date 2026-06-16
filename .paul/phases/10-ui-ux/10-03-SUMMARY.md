---
phase: 10-ui-ux
plan: 03
subsystem: ui
tags: [juce, perform-screen, pad-grid, sequencer, sample-editor, velocity, timer]

requires:
  - phase: 10-02
    provides: BaqueLookAndFeel design system, BaqueEditor NAV shell, ScreenPlaceholder
  - phase: 10-01
    provides: UiCommandQueue, UiStateSnapshot, push_ui_command(), ui_snapshot()
  - phase: 04
    provides: PadBank, SamplePad, PlayMode, pad params single-writer contract

provides:
  - PerformScreen: 4×4 PadGrid + 16×16 Sequencer (TODAS/FOCO) + SampleEditor
  - StepPattern per-step velocity (1–127)
  - BaqueProcessor::current_pattern() by-value + current_pad() read accessors
  - BaqueEditor PERFORM slot upgraded from ScreenPlaceholder to PerformScreen

affects: [10-04, 10-05, 10-07]

tech-stack:
  added: []
  patterns:
    - "Timer-gated components: start in visibilityChanged, stopTimer() in explicit destructor"
    - "Callback ownership: PerformScreen sole authority for focused-pad propagation"
    - "Knob drag-capture: onDragStart captures focused_pad_ → knob_drag_pad_ (SR3)"
    - "No optimistic repaint: 30fps timer reconciles within 33ms (SR1)"
    - "current_pattern() by value — display-only, tolerates torn state on bar boundary"

key-files:
  created:
    - src/ui/pad_grid_component.h
    - src/ui/pad_grid_component.cpp
    - src/ui/sequencer_component.h
    - src/ui/sequencer_component.cpp
    - src/ui/sample_editor_component.h
    - src/ui/sample_editor_component.cpp
    - src/ui/perform_screen.h
    - src/ui/perform_screen.cpp
    - tests/test_perform_screen.cpp
  modified:
    - src/audio/step_pattern.h
    - src/audio/step_pattern.cpp
    - src/plugin_processor.h
    - src/plugin_processor.cpp
    - src/plugin_editor.h
    - src/plugin_editor.cpp
    - src/ui/header_component.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "set_step_velocity: direct write to sequencer_.pattern() matching existing set_step dispatch (not copy→set_next_pattern)"
  - "current_pattern() returns by value — NOT same contract as UiStateSnapshot atomics"
  - "PerformScreen::setFocusedPad() sole authority — no self-setFocusedPad in mouseDown"
  - "FOCO mode selector: two visible tab buttons (TODAS|FOCO), not single toggle"

patterns-established:
  - "All timer-gated components: visibilityChanged starts/stops; explicit ~Ctor() { stopTimer(); }"
  - "Non-vacuous tests: toggleStep/setPlayMode return bool; tests assert == true"
  - "PerformScreen forward-declares child types — unique_ptr only needs complete type at destruction"

duration: ~3 sessions
started: 2026-06-15T00:00:00Z
completed: 2026-06-16T00:00:00Z
description: "PERFORM screen shipped: 4×4 velocity PadGrid, 16×16 TODAS/FOCO Sequencer, SampleEditor PITCH/DECAY/PAN knobs; StepPattern gains per-step velocity; 229 tests pass"
type: Summary
about: "BAQUE"
---

# Phase 10 Plan 03: PERFORM Screen Summary

**PERFORM screen shipped: 4×4 velocity-pulsing PadGrid, 16×16 TODAS/FOCO Sequencer, SampleEditorComponent with PITCH/DECAY/PAN knobs; StepPattern gains per-step velocity; 229 tests pass**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~3 sessions (2026-06-15–16) |
| Tasks | 4 completed (3 auto + 1 human-verify) |
| Tests | 229 pass (226 baseline + 3 new PERF1/2/3) |
| Files created | 9 |
| Files modified | 9 |
| Commit | `653b2a3` |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: PerformScreen replaces PERFORM placeholder | Pass | PERF1 constructs editor; PERFORM slot is PerformScreen not ScreenPlaceholder |
| AC-2: Step toggle sends UiCommand | Pass | PERF2: toggleStep(0,0) returns true |
| AC-3: SampleEditor mode tab changes play_mode | Pass | PERF3: setPlayMode(gate) returns true |
| AC-4: PadGrid timer runs while visible | Pass | visibilityChanged starts 30fps timer; stopTimer in explicit dtor |
| AC-5: toggleStep/setPlayMode return push result | Pass | Both return bool; tests assert == true (non-vacuous) |

## Task Commits

All tasks in single commit (plan spanned 2 sessions, squash approach):

| Task | Commit | Description |
|------|--------|-------------|
| Task 1–4 + fixes | `653b2a3` | feat(10): Plan 10-03 APPLY — PERFORM screen |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/ui/pad_grid_component.h/.cpp` | Created | 4×4 pad grid, velocity glow, 30fps timer, focus ring |
| `src/ui/sequencer_component.h/.cpp` | Created | 16×16 step grid, TODAS/FOCO mode, playhead, toggleStep |
| `src/ui/sample_editor_component.h/.cpp` | Created | Mode tabs, PITCH/DECAY/PAN knobs, SR3 drag-capture |
| `src/ui/perform_screen.h/.cpp` | Created | Wires all three children; sole focus authority |
| `src/audio/step_pattern.h/.cpp` | Modified | Added per-step velocity field (1–127) + get/set_velocity |
| `src/plugin_processor.h/.cpp` | Modified | current_pattern() by value; current_pad(); set_step_velocity dispatch |
| `src/plugin_editor.h/.cpp` | Modified | screens_ → unique_ptr<juce::Component>; PERFORM = PerformScreen |
| `src/ui/header_component.cpp` | Modified | Fixed setActiveScreen(): clear all tabs before setting active |
| `CMakeLists.txt` | Modified | Added 4 new .cpp sources |
| `tests/test_perform_screen.cpp` | Created | PERF1, PERF2, PERF3 |
| `tests/CMakeLists.txt` | Modified | Added test_perform_screen.cpp |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| set_step_velocity: direct write to sequencer_.pattern() | Plan said copy→set_next_pattern; discovered set_step already does direct write (audio thread owns pattern_). Consistent pattern. | set_step_velocity works; set_next_pattern path only for message-thread full-pattern swaps |
| current_pattern() returns StepPattern BY VALUE | Reference into non-atomic struct = data race on bar-boundary swap. Display-only; stale data tolerable. | NOT same contract as UiStateSnapshot atomics — documented in header |
| Two-button TODAS/FOCO tab selector | Single "TODAS" toggle unintuitive: user looks for "FOCO" button. Both always visible. | Better discoverability; active mode visually highlighted |
| HeaderComponent::setActiveScreen bug fix | Pre-existing: only set new button true, never cleared old. All clicked tabs stayed highlighted. | Unrelated to plan but discovered during human-verify; fixed in place |

## Deviations from Plan

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 2 | Essential correctness fixes |
| Scope additions | 1 | Minor UX improvement + pre-existing bug fix |
| Deferred | 3 | As planned |

**Auto-fixed:**
1. **set_step_velocity dispatch idiom** — Plan specified copy→set_next_pattern. Existing set_step at line 219 writes directly to sequencer_.pattern() (audio thread owns it). Fixed to match existing direct-write pattern.
2. **const removeFromBottom** — bar_r declared const but removeFromBottom is non-const. Fixed with `withTop(bottom - bar_h)`.

**Scope addition (human-verify phase):**
- FOCO button: single toggle → two tab buttons (TODAS|FOCO)
- HeaderComponent::setActiveScreen: cleared previous tab highlight (pre-existing bug)

**Deferred:**
- EXT-only lane pulse in lane_last_velocity (v1 limitation — 10-07)
- Feel Field radial visualizer centre column (10-04)
- StepPattern velocity → note-on velocity in Scheduler (10-05/07)

## Next Phase Readiness

**Ready:**
- PERFORM screen fully wired: pad click → focus propagates to sequencer lane + sample editor
- Step grid live: click toggles step, 30fps playhead tracking
- Sample editor knobs push UiCommands to audio thread
- PerformScreen wired into BaqueEditor; NAV switching correct

**Concerns:**
- Feel Field placeholder (centre column) is empty grey rect — 10-04 fills it
- SampleEditor knob values read from current_pad() advisory read — acceptable for display; audio thread is sole writer

**Blockers:** None
