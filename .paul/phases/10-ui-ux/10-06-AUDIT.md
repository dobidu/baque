# Enterprise Plan Audit Report

**Plan:** .paul/phases/10-ui-ux/10-06-PLAN.md
**Audited:** 2026-06-17
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable, now upgraded.** Plan is structurally sound: UiCommand field mappings are verified correct, RAII attachment ordering is correct, Timer gate replicates the established Phase 10 pattern. Three issues required correction before APPLY; all applied.

Would approve after upgrades. Remaining concerns are cosmetic/deferred.

---

## 2. What Is Solid

- **M1 forward-declare pattern** correctly carried: both .h files use `class BaqueProcessor;` forward-declare; both .cpp files include `"../plugin_processor.h"` explicitly. Compile-correct on all 3 CI platforms.
- **UiCommand field mapping verified**: all 5 push_ui_command() calls cross-checked against ui_command_queue.h enum comments — `set_clock_master` (c=enabled), `apply_template` (a=template_id), `set_lane_mode` (a=lane, c=mode), `set_lane_channel` (a=lane, c=channel), `set_mute`/`set_solo` (a=lane, c=bool) — all correct.
- **RAII attachment order**: `scatter_type_attach_` / `scatter_depth_attach_` declared after their BaqueKnob controls → destroyed first. Mirrors FxScreen/MixScreen pattern established in 10-05.
- **MidiScreen timer gate**: visibilityChanged() start/stop + stopTimer() in dtor — exact same pattern as MixScreen (10-05, MIX1 test). Test PERF5 exercises it.
- **XyPad null-guards**: `if (on_drag) on_drag(...)` and `if (on_release) on_release()` — safe before callbacks are wired in ctor.
- **visibilityChanged() → xy_pad_.on_release()**: filter_cutoff/filter_res reset to APVTS defaults on screen hide — correct per BAQUE-UI-SPEC §6.3 ("state clears on screen change").
- **Boundaries**: Scene Morph, STUTTER/RISER/CRUSH/FILL, CC-out table, NOTE editing, engine MIDI learn all explicitly scoped out with rationale.
- **Test count arithmetic**: 234 + 5 = 239. Correct.

---

## 3. Enterprise Gaps Identified

### M1 — MouseListener ambiguous base class (CI-blocking on MSVC)

Plan text states: "*PerfFxScreen inherits `private juce::MouseListener`*". `juce::Component` already inherits `juce::MouseListener`. Adding explicit `private juce::MouseListener` to PerfFxScreen would cause:
- MSVC: `error C2385: ambiguous access of 'mouseDown'`
- GCC/Clang: ambiguous-base-class warning, potential linker issue
- This hits the 3-platform CI matrix immediately.

The header declaration `class PerfFxScreen : public juce::Component` is correct as-is. `addMouseListener(this, false)` works because `Component*` IS-A `MouseListener*`. mouseDown/mouseUp overrides work via Component's existing virtual methods. No additional base class needed.

### SR1 — MidiScreen timerCallback overwrites mode toggles causing flicker

Original spec: "Advisory re-sync: read proc_.lane_routing_.mode[i] for each lane and update toggle states." This creates a 100ms flicker loop:

1. User clicks INT button → `mode_int_[i].setToggleState(true, ...)` (immediate responsive feedback)
2. UiCommand `set_lane_mode` enqueued; audio thread hasn't drained it yet
3. timerCallback fires (10fps = 100ms) → reads stale `proc_.lane_routing_.mode[i]` still == old value → sets toggle back to old state
4. Visual flicker until audio thread catches up (typically <1ms but jitter can extend to 10s of ms)

Fix: timerCallback updates only note_labels_ (advisory pattern read). Mode toggle state is owned by onClick callbacks. Same "optimistic-write" pattern used by MixScreen mute/solo state.

### SR2 — XyPad no mouseDown override — click-without-drag is a no-op

Original XyPad overrides: mouseDrag + mouseUp only. For a performance pad, a single click (press + immediate release without drag) must position the cursor and fire on_drag. Without mouseDown, clicking the XY pad does nothing until the mouse moves. This would be immediately noticed in any performance-context testing.

Fix: add `void mouseDown(const juce::MouseEvent& e)` override identical to mouseDrag (update cursor_x_/y_, repaint, call on_drag).

### SR3 — clock_master_state_ initialized to false regardless of engine state

`bool clock_master_state_ = false;` in header. `proc_.clock_master_` is a public member (plugin_processor.h:112). After a preset load that had clock master enabled, the MidiScreen button shows OFF when the engine is ON. Click toggles UI to ON and pushes `set_clock_master(true)` — no-op since engine already has it true. Second click pushes false — now UI matches but engine was wrongly double-toggled.

Fix: initialize `clock_master_state_ = proc_.clock_master_` in ctor body; call `setToggleState(clock_master_state_, ...)` before wiring onClick.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Explicit `private juce::MouseListener` in PerfFxScreen causes MSVC ambiguous-base-class error | Task 1 — Implementation, "Final pattern" paragraph | Corrected to: Component already inherits MouseListener — no explicit inheritance; added note that DO NOT add MouseListener to class declaration; added mouseDown/mouseUp override declarations to private section spec |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | timerCallback mode-toggle re-sync causes 100ms flicker after onClick | Task 2 — timerCallback spec | Changed to: update note_labels_ advisory ONLY; explicit comment that mode toggle states must NOT be written from timer |
| SR2 | XyPad mouseDown missing — click-without-drag is no-op on performance pad | Task 1 — XyPad header declaration + XyPad implementations | Added `void mouseDown(const juce::MouseEvent& e) override` to XyPad inner class; added mouseDown implementation (identical to mouseDrag: update cursor + repaint + call on_drag) |
| SR3 | clock_master_state_ hardcoded false — wrong after preset load | Task 2 — Constructor clock_master_btn_ setup | Added `clock_master_state_ = proc_.clock_master_` + `setToggleState(clock_master_state_, ...)` before wiring onClick |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | LEARN toggle never auto-resets — stays "armed" forever | Plan explicitly documents LEARN as visual-only v1; engine lane-note MIDI learn deferred to Phase 10/11. No user confusion if text says "ARMED" — the deferred note in boundaries is clear. |
| D2 | scatter_type as rotary may feel wrong for a "type selector" (0-10 integers) | SliderAttachment on rotary with step=1 works correctly (proven by prior SliderAttachment usage). Cosmetic UX preference — linear slider may be better but not broken. |
| D3 | XY pad has no text label ("FILTER XY" overlay) | Not specified in plan; functional behavior is clear from crosshair+cursor visual. Cosmetic — Phase 11 UI polish pass. |

---

## 5. Audit & Compliance Readiness

**Evidence**: 5 Catch2 tests (PERF1–PERF5) provide construct + paint smoke + timer gate evidence. Timer gate test (PERF5) matches MixScreen's MIX1 pattern — defensible.

**Silent failure risk**: The SR1 flicker (mode toggle resync) would not crash, but would produce wrong visual state for ~100ms after each click. With the fix applied this is eliminated.

**Post-incident reconstruction**: UiCommand push paths are identical to established Phase 10 patterns. All 5 command types were previously exercised in prior phases. No new command types introduced.

**Ownership**: BaqueProcessor& proc_ stored in both screens — ownership is AudioProcessorEditor's BaqueProcessor lifetime, which outlives all child components. Correct.

**APVTS parameter names**: "tape_stop", "gate_depth", "scatter_type", "scatter_depth", "filter_cutoff", "filter_res" — all verified present in plugin_processor.cpp create_parameter_layout(). Typo in a param ID string causes silent null-dereference of AudioProcessorParameter*. The verify step in Task 1 should check the standalone runs without crash at construction.

---

## 6. Final Release Bar

**Must be true before shipping:**
1. M1 applied: PerfFxScreen compiles on MSVC without ambiguous-base-class errors (3-platform CI green)
2. SR2 applied: XyPad mouseDown fires on click — manually tested in standalone
3. SR3 applied: CLOCK MASTER toggle reflects engine state after construction
4. SR1 applied: No mode-toggle flicker after clicking INT/EXT/BOTH
5. 239/239 tests pass including all 5 new PERF tests
6. Human-verify checkpoint: PERF_FX + MIDI tabs render and interact correctly

**Remaining risks if shipped:**
- LEARN toggle persists armed state forever (v1 documented limitation — acceptable for Phase 10)
- mute/solo group state (group_mute_[]/group_solo_[]) is message-thread-local — diverges after preset load (same known issue as MixScreen mute_state_[], documented in 10-05 and recurring here)
- Note labels in MIDI table are advisory reads — may show stale data for ~100ms after pattern change

**Sign-off**: Will sign after 5 pre-ship checks above are confirmed by CI + human-verify checkpoint.

---

**Summary:** Applied 1 must-have + 3 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
