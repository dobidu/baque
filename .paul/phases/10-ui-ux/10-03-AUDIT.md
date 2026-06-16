# Enterprise Plan Audit Report

**Plan:** `.paul/phases/10-ui-ux/10-03-PLAN.md`
**Audited:** 2026-06-16
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable.** Plan covers the required scope (PadGrid + SequencerComponent + SampleEditorComponent + engine prep) with adequate task decomposition. Four must-have findings prevent direct approval as-is: a data race framed as safe when it isn't, missing timer destructor cleanup following a pattern the codebase already established, no bounds guards on user-controlled indices, and vacuous test assertions that don't actually verify command dispatch.

All four were applied. I would approve the upgraded plan for execution.

---

## 2. What Is Solid

**Engine dispatch pattern (Task 1):** `copy → set_velocity → set_next_pattern` idiom is correct and consistent with how `set_step` is implemented. No direct write to `pattern_` from the message thread.

**Timer lifecycle gating (PadGrid, Sequencer):** `visibilityChanged()` start/stop pattern matches BaqueMeter exactly. Correct for JUCE plugins where components are created before the audio thread activates.

**UiStateSnapshot usage:** PadGrid reads `lane_last_velocity[i]` from the snapshot — this is the correct read path (atomic per-field, no data race). Consistent with 10-01 architecture.

**setSize() ordering:** Not present in PerformScreen constructor (size set by BaqueEditor::resized()); construction order of sub-components is safe since all are owned before any setBounds() call reaches them.

**ScreenPlaceholder polymorphism upgrade:** Changing `screens_` to `unique_ptr<juce::Component>` is the correct incremental approach — all existing showScreen/isScreenVisible logic continues to work through the base type.

**set_step dispatch:** Pre-verified to exist in plugin_processor.cpp at line 219 (10-01 implementation). No gap here.

---

## 3. Enterprise Gaps Identified

### G1 — Data race on `current_pattern()` misrepresented as safe
**Plan said:** "Same relaxed contract as UiStateSnapshot" — returns `const StepPattern&`.

`UiStateSnapshot` uses `std::atomic` per field. `StepPattern` is a plain struct. `Sequencer::pattern_` is written by the audio thread via `pattern_ = next_pattern_` (768-byte struct assign, non-atomic) guarded only by `pattern_pending_` atomic flag. The UI thread reading the reference concurrently is a data race — C++ standard UB.

Returning by reference makes this worse: the alias lives for the entire repaint loop (256 reads over a non-atomic struct being swapped).

**Fix applied:** Return `StepPattern` by value. One copy per repaint; in paint() caller captures result once (`const StepPattern pat = proc_.current_pattern()`). Corrected documentation removes the false "same relaxed contract" claim.

### G2 — Timer destructor cleanup missing
**Plan said:** `~PadGridComponent() override;` and `~SequencerComponent() override;` with no implementation body specified.

`HeaderComponent::~HeaderComponent()` already calls `stopTimer()` — this is the project's established pattern. `BaqueMeter` uses visibility gating and is never made visible in tests, making it safe without explicit destructor. PadGrid and SequencerComponent may be visible (and thus timer-running) at destruction time (PerformScreen destroyed when editor closes). JUCE Timer's own destructor calls stopTimer() but only AFTER member variables may have started destructing. timerCallback() firing during partial destruction → UB.

**Fix applied:** Both destructors specified as `{ stopTimer(); }` in .cpp.

### G3 — No bounds guards on user-controlled pad/lane indices
`setFocusedPad(int pad_idx)` → `display_velocity_[idx]` (unchecked array access).
`setFocusedLane(int lane)` → `focused_lane_` used in paint() loop (unchecked).
`current_pad(int idx)` → `pad_bank_.pad(idx)` → `pads_[size_t(idx)]` (unchecked).

Any future caller passing an out-of-range value is silent UB with potential crash.

**Fix applied:** `std::clamp` in setFocusedPad/setFocusedLane; `jassert` in current_pad(). mouseDown() gets explicit `if (idx < 0 || idx >= 16) return;` guard.

### G4 — Vacuous test assertions `CHECK(true)`
PERF2 and PERF3 both ended with `CHECK(true)` — identical to what would be written if the functions under test didn't exist. This is the same pattern rejected in audits 06-04 (PD4), 07-03 (GR3/GR4), and multiple other audits.

**Fix applied:** `toggleStep()` and `setPlayMode()` both return `bool` (the `push_ui_command()` result). Tests assert the return value is `true`. A full queue would produce `false`, catching a regression where commands are dropped.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | `current_pattern()` returns reference into non-atomic struct — data race; wrong contract doc | Task 1 action, plugin_processor.h | Return `StepPattern` by value; fix doc comment; caller snaps copy once per repaint |
| M2 | PadGrid/Sequencer missing explicit `stopTimer()` in destructor — timer fires during partial destruction | Task 2 action (both components) | Specified `{ stopTimer(); }` destructor bodies matching HeaderComponent pattern |
| M3 | setFocusedPad/setFocusedLane/current_pad have no bounds guards — OOB silent UB | Task 2 action, Task 3 header | `std::clamp` in setters; `jassert` in current_pad(); mouseDown guard |
| M4 | PERF2/PERF3 `CHECK(true)` vacuous — doesn't verify command dispatch | Task 3 action (tests) | `toggleStep()`/`setPlayMode()` return `bool`; tests assert return == true; AC-5 added |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | `toggleStep()` optimistic repaint causes stopped-sequencer display divergence | Task 2 SequencerComponent action | Removed `repaint()` from `toggleStep()`; 30fps timer reconciliation is sufficient |
| SR2 | PadGrid::mouseDown → on_pad_clicked → setFocusedPad → pad_grid_->setFocusedPad() redundant double-update loop | Task 2 PerformScreen constructor note | mouseDown calls only on_pad_clicked; PerformScreen::setFocusedPad() is sole authority for all three children |
| SR3 | Knob `onChange` reads `focused_pad_` at call time — mid-drag pad focus change targets wrong pad | Task 3 knob construction | `onDragStart` captures `focused_pad_` into `knob_drag_pad_`; `onValueChange` uses captured value |
| SR4 | `perform_screen.h` including full child-component headers pollutes BaqueEditor's include chain | Task 2 PerformScreen header | Forward-declare PadGridComponent/SequencerComponent/SampleEditorComponent; `~PerformScreen()` defined in .cpp where types are complete |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Negative `knob_size` guard at extreme resize (`r.getHeight() - 20 < 0`) | Editor has 600px height floor via setResizeLimits; layout math won't reach negative at valid sizes |
| D2 | AC-3 tests setPlayMode() directly, not the button onClick wiring | Full button-click simulation requires JUCE GUI event dispatch in tests; setPlayMode() being the actual code path makes this low-risk |
| D3 | `mode_toggle_` TextButton label stays "TODAS" when FOCO is active | Cosmetic UX; button toggle state communicates mode visually; relabel deferred |

---

## 5. Audit & Compliance Readiness

**Evidence produced:** 3 tests (PERF1–PERF3), all with non-vacuous assertions after M4 fix. PERF2/PERF3 now verify the command dispatch path end-to-end (push returns true → queue accepted). Adequate for post-incident "did the UI actually send the command?" reconstruction.

**Silent failures:** M1 fix (by-value copy) eliminates the reference aliasing issue. M4 ensures tests fail loudly if command queue is full or push is broken. M3 bounds guards provide early jassert failure rather than silent OOB.

**Thread safety:** UiStateSnapshot reads (velocity, current_step) remain atomic. Pattern reads are advisory/display-only with accurate documentation. No message-thread writes to audio-owned structs except via push_ui_command (unchanged contract from 10-01).

**Potential audit gap:** StepPattern::velocity storage is v1 only — the sequencer doesn't feed it to Scheduler note-on velocity yet (explicitly deferred, documented in boundaries). This is acceptable IF the boundary note is preserved through APPLY.

---

## 6. Final Release Bar

**Must be true before this plan ships:**
- M1–M4 applied (checked ✓ above)
- `~PadGridComponent()` and `~SequencerComponent()` implementations verified in .cpp (grep check added to verification section)
- `current_pattern()` return type verified as `StepPattern` by value (grep check added)
- 229+ tests pass (226 baseline + 3 new), zero clang-format violations
- Human-verify: PERFORM screen renders; step toggle persists after release; pad focus syncs across all three panels

**Risks remaining if shipped as-is (after upgrades):**
- StepPattern data race on bar-boundary swap — acknowledged, advisory, x86 byte-atomicity relied upon; will persist until UiStateSnapshot bitmask approach is adopted (D3, deferred)
- No waveform rendering (placeholder only) — explicitly scoped out

**Sign-off:** Upgraded plan is approvable. Execute with the applied fixes.

---

**Summary:** Applied 4 must-have + 4 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
