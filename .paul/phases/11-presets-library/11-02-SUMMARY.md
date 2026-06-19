---
phase: 11-presets-library
plan: 02
subsystem: ui
tags: [juce, preset-browser, factory-presets, feel-engine, c++]

requires:
  - phase: 11-01
    provides: PresetManager (save/load BQP1 format), set_feel_pattern(), current_feel_pattern(), save_to_file(), P11D1-P11D5 tests

provides:
  - FactoryPresetLibrary (6 aesthetics: Straight/Boom-Bap/Dilla Drunk/Burial Broken/FlyLo Wonk/Bonobo Loose)
  - BrowserScreen SAMPLES|PRESETS tab mode with PresetListModel inner struct
  - inline TextEditor save flow (no AlertWindow modal hazard)
  - P11D6 + P11D7 DoD tests — Phase 11 DoD complete
  - 251 total tests (249 → 251)

affects: Phase 12 (Hardening — pluginval, RT-safety audit, browser async/waveform/drag-drop)

tech-stack:
  added: [factory_preset_library.h, factory_preset_library.cpp]
  patterns:
    - "FactoryPresetLibrary delegates to FeelPresets::* — no value duplication"
    - "PresetListModel inner struct separates file/preset ListBoxModel impls — no ambiguous base"
    - "Inline TextEditor for save name — no AlertWindow modal in plugin editor context"
    - "user_presets_ stale-array bounds guard before index access"

key-files:
  created:
    - src/factory_preset_library.h
    - src/factory_preset_library.cpp
  modified:
    - src/ui/browser_screen.h
    - src/ui/browser_screen.cpp
    - tests/test_phase11_dod.cpp
    - CMakeLists.txt

key-decisions:
  - "FactoryPresetLibrary::load_into() message-thread only — same constraint as set_feel_pattern()"
  - "AlertWindow::showInputBox banned in plugin editors — hosting hazard, nested event loop; inline TextEditor used instead"
  - "PresetListModel as inner struct of BrowserScreen — avoids ambiguous-base with BrowserScreen's ListBoxModel"

patterns-established:
  - "Factory presets delegate to FeelPresets::* — never duplicate FeelPattern values"
  - "Inline TextEditor for user input in plugin UI — AlertWindow/showInputBox is forbidden"

duration: ~1 session
started: 2026-06-19T02:00:00Z
completed: 2026-06-19T04:00:00Z
description: "FactoryPresetLibrary (6 groove aesthetics) + BrowserScreen PRESETS tab + P11D6/P11D7 — Phase 11 DoD complete, 251/251 tests"
type: Summary
about: "BAQUE"
---

# Phase 11 Plan 02: Preset Browser UI + Factory Library Summary

**FactoryPresetLibrary (6 groove aesthetics) + BrowserScreen SAMPLES|PRESETS tab + P11D6/P11D7 — Phase 11 DoD complete, 251/251 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~1 session |
| Started | 2026-06-19T02:00:00Z |
| Completed | 2026-06-19T04:00:00Z |
| Tasks | 3 completed |
| Files modified | 6 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: FactoryPresetLibrary provides 6 named presets | Pass | k_count==6; all delegate to FeelPresets::*; load_into() sets feel + step pattern + master_gain |
| AC-2: BrowserScreen PRESETS tab lists and loads presets | Pass | SAMPLES\|PRESETS tabs; factory [F] + user [U] in preset_list_; double-click or LOAD PRESET button routes correctly |
| AC-3: Save current state as user preset | Pass | Inline TextEditor (no AlertWindow); empty name → "Enter a preset name"; non-empty → save + clear + refresh |
| AC-4: Phase 11 DoD — 251/251 tests pass | Pass | P11D6 (Dilla Drunk + Straight + boundary), P11D7 (factory→file→fresh-proc round-trip), 0 regressions |

## Accomplishments

- FactoryPresetLibrary: static-only class, 6 aesthetics, switch-dispatches to FeelPresets::* (no value duplication), sets basic kick/snare StepPattern + master_gain=0.8f
- BrowserScreen: SAMPLES|PRESETS tab mode; PresetListModel inner struct as separate ListBoxModel (avoids ambiguous-base from BrowserScreen's own ListBoxModel impl); factory presets [F] prefix, user presets [U] prefix; load routes to FactoryPresetLibrary or PresetManager
- Audit-driven fixes shipped: inline TextEditor (M1 — AlertWindow hosting hazard), correct test fixture pattern (M2), stale-array bounds guard (SR1), Straight coverage + boundary checks (SR2)
- Phase 11 DoD complete: P11D1-P11D7 all pass; save/load faithful (11-01) + factory library per aesthetic (11-02)

## Task Commits

| Task | Commit | Type | Description |
|------|--------|------|-------------|
| Task 1+2+3 | `46bd9d0` | feat | FactoryPresetLibrary + BrowserScreen PRESETS tab + P11D6/P11D7 |
| State sync | `ca9aeb3` | docs | 11-02 APPLY state + ledger update |
| Audit | `88c20e4` | docs | 11-02 AUDIT report + PLAN.md edits applied |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/factory_preset_library.h` | Created | FactoryPresetLibrary — static class, k_count=6, name/category/load_into API |
| `src/factory_preset_library.cpp` | Created | load_into() delegates to FeelPresets::*; sets kick/snare StepPattern; APVTS master_gain |
| `src/ui/browser_screen.h` | Modified | Added BrowserMode enum, tabs, PresetListModel inner struct, preset panel members, preset_manager_ |
| `src/ui/browser_screen.cpp` | Modified | switchMode(), refresh_presets(), load_selected_preset() with bounds guard, PresetListModel impl, resized() tab-shifted layout |
| `tests/test_phase11_dod.cpp` | Modified | Appended P11D6 (Dilla Drunk + Straight + boundary) and P11D7 (round-trip via file) |
| `CMakeLists.txt` | Modified | Added src/factory_preset_library.cpp to target_sources |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| AlertWindow::showInputBox banned in plugin UI | Spins nested event loop in host's message thread; some DAWs deadlock — untestable in CI, manifests on specific host+OS | All future save-name UI must use inline components |
| PresetListModel as inner struct | BrowserScreen already inherits ListBoxModel for file_list_; second inheritance base = ambiguous override; inner struct = separate object, no ambiguity | Pattern for all future dual-model ListBoxes |
| load_into() message-thread only | Calls set_feel_pattern() which writes feel_pattern_ non-atomically; same constraint as all other non-RT state setters | Must not call from processBlock; matches 11-01 contract |
| Factory presets do not embed samples | Pads without samples play silence — expected behavior; sample embedding is ESCOPO §14.5 deferred | No sample data in .bqpreset files for factory presets |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Audit fixes applied pre-APPLY | 4 | All plan-level; 0 code-level surprises during APPLY |
| Scope additions | 0 | — |
| Deferred | 0 new | All within established deferred list |

All 4 audit findings (M1, M2, SR1, SR2) were applied to PLAN.md before APPLY. APPLY executed the upgraded plan exactly as written. No additional deviations during implementation.

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| cmake reconfigure needed after adding factory_preset_library.cpp | Ran `cmake -S . -B build` before build — build directory needed to know about new source |

## Next Phase Readiness

**Ready:**
- FactoryPresetLibrary::load_into() and name/category API stable; Phase 12 can add preset import/export without touching this class
- BrowserScreen PRESETS tab functional with factory + user presets; Phase 12 can add async scan, waveform preview, drag-drop without structural change
- 251/251 tests pass; Phase 12 Hardening starts from green baseline

**Concerns:**
- Inline TextEditor for preset name has no sanitization at UI level (PresetManager sanitizes on save — defense-in-depth gap deferred to Phase 12)
- Concurrent multi-instance preset writes not guarded (OS atomicity sufficient for v1; advisory locking is Phase 12 concern)
- JUCE 9 deprecation: AlertWindow API may be removed post-upgrade (M1 fix is already the correct long-term pattern — no additional action needed)

**Blockers:** None.

---
*Built with PAUL Framework v1.4*
*Phase: 11-presets-library, Plan: 02*
*Completed: 2026-06-19*
