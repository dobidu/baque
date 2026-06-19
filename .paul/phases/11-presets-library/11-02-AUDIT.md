# Enterprise Plan Audit Report

**Plan:** .paul/phases/11-presets-library/11-02-PLAN.md
**Audited:** 2026-06-19T03:00:00Z
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable.** Two release-blocking defects (M1, M2) and two strongly-recommended fixes (SR1, SR2). After applying all 4 findings, plan is enterprise-ready.

M1 is a hosting-hazard: `AlertWindow::showInputBox` spins a nested modal event loop inside a JUCE plugin editor, blocking the host's message thread — forbidden by several major DAWs. M2 is a compile-failure: `TestProcessorFixture` doesn't exist; APPLY would produce a type-unknown error. Both are must-fix before APPLY.

## 2. What Is Solid

- **FactoryPresetLibrary delegates to FeelPresets::*** — no value duplication risk; correctness bound to canonical source; switch-based dispatch is auditable and testable.
- **set_feel_pattern() + set_pattern() message-thread constraint documented** — mirrors Phase 5-01 and 11-01 contracts; correctly excludes RT path.
- **P11D7 round-trip via TemporaryFile** — mirrors P11D3's proven pattern; no user-dir pollution in CI.
- **Boundaries are precise** — all Phase 12 browser enhancements (async scan, waveform thumb, drag-drop, auto-audition, mute/solo automation, scene morph) correctly deferred.
- **preset_manager_ init order note** — member declared after proc_ (ref); C++ init-order hazard correctly called out.
- **PresetListModel inner struct** — separate object from BrowserScreen's ListBoxModel impl; no ambiguous-base hazard.
- **user_presets_ stale-array guard (SR1)** — added by audit; prevents undefined behavior on file deletion between refresh and click.

## 3. Enterprise Gaps Identified

### G1 — AlertWindow::showInputBox in plugin editor (M1 — must-have)
`AlertWindow::showInputBox` creates a synchronous blocking modal that spins its own nested event loop inside the plugin editor. In a JUCE plugin, the plugin shares the host's message thread. A nested event loop inside the plugin editor is documented as unsafe in JUCE plugin development — some hosts (Live, REAPER under certain configurations, any host that owns the main event pump) freeze or deadlock when the plugin spins its own modal. This is not a style violation — it is a hosting-hazard that will manifest on specific DAW+OS combinations and is unfalsifiable in CI. The plan's mitigation note ("acceptable, user-initiated modal") provides no protection against host-side freezes.

Fix: always-visible `juce::TextEditor preset_name_editor_` in the right panel. No modal. No nested event loop. SAVE button reads `preset_name_editor_.getText().trim()`. Empty → status label "Enter a preset name". Non-empty → save + clear.

### G2 — TestProcessorFixture doesn't exist (M2 — must-have)
The P11D6/P11D7 test blueprints call `TestProcessorFixture fix;` and access `fix.proc`. No such class exists in test_phase11_dod.cpp or any header visible to the test binary. The plan's prose instructs APPLY to "match exactly the pattern used in P11D1-P11D5" but the code example contradicts this — existing tests use `juce::ScopedJuceInitialiser_GUI init; BaqueProcessor proc;` (default ctor, no prepareToPlay, no wrapper). APPLY will write the code example literally; the compiler will emit `error: unknown type name TestProcessorFixture`; the build fails.

Fix: rewrite both test blueprints to use the exact P11D1-P11D5 pattern: `juce::ScopedJuceInitialiser_GUI init; BaqueProcessor proc;`.

### G3 — user_presets_ accessed without bounds check (SR1 — strongly recommended)
`load_selected_preset()` computes `row - k_count` and indexes `user_presets_` without verifying the index is valid. Between `refresh_presets()` and the user's click, a preset file can be deleted by an external process, file manager, or another plugin instance. The `user_presets_` array is stale; the index is within the original array size but now refers to a wrong entry (or exceeds the array if fewer entries remain). Both cases are undefined behavior or silent wrong-preset loads.

Fix: `if (user_idx >= user_presets_.size()) { status_label_.setText("Preset unavailable - refresh list"); return; }` before the array access.

### G4 — P11D6 single-preset coverage + name(6) jassert hazard (SR2 — strongly recommended)
P11D6 as written tests only index 2 (Dilla Drunk). A bug in case 0, 1, 3, 4, or 5 of the switch ships undetected. Additionally, the plan includes `CHECK(juce::String(FactoryPresetLibrary::name(6)).isEmpty())` — calling `name(6)` fires `jassert(i >= 0 && i < k_count)` in debug builds; in Catch2 context, jassert calls `juce_breakDebugger()` which terminates the test process, failing the entire suite.

Fix: add a second inner scope in P11D6 testing index 0 (Straight: `enabled==true`, `seed==1u`, `steps[0].timing_ms≈0.0f`). Replace `name(6)` call with valid boundary checks: `k_count==6`, `name(0)` non-empty, `name(5)` non-empty.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | AlertWindow::showInputBox blocks host message loop — hosting hazard | Task 2 action (save_preset_btn_), browser_screen.h additions, switchMode, resized(), avoid section; AC-3 | Replaced AlertWindow call with inline `juce::TextEditor preset_name_editor_`; save_preset_btn_ reads from editor; empty guard added; editor shown/hidden in switchMode; bounds laid out in resized() |
| M2 | TestProcessorFixture nonexistent — P11D6/P11D7 won't compile | Task 3 action (P11D6 + P11D7 code examples) | Replaced `TestProcessorFixture fix; fix.proc` with `juce::ScopedJuceInitialiser_GUI init; BaqueProcessor proc;` matching P11D1-P11D5 exactly; removed TestProcessorFixture ambiguity note |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | user_presets_[user_idx] accessed without bounds check — crash on stale array | Task 2 action (load_selected_preset) | Added `if (user_idx >= user_presets_.size()) { status_label_.setText("Preset unavailable - refresh list"); return; }` before array access |
| SR2 | P11D6 tests only index 2; name(6) fires jassert in debug builds | Task 3 action (P11D6 blueprint) | Added second inner scope for Straight (index 0): enabled==true, seed==1u, timing_ms≈0.0f; replaced `name(6)` with `k_count==6`, `name(0)` non-empty, `name(5)` non-empty |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | AlertWindow deprecation in JUCE 9+ — API may be removed in future JUCE upgrade | Relevant only post-JUCE upgrade; M1 fix (inline TextEditor) is already the correct long-term pattern; no additional action needed |
| D2 | Preset name sanitization at UI level (remove path-unsafe chars, leading dots) | PresetManager::save() already sanitizes the name (11-01 scope); UI-level validation is defense-in-depth; Phase 12 hardening pass |
| D3 | Concurrent multi-instance preset writes (two plugin instances save simultaneously) | OS file I/O is atomic for single writes; no data corruption possible; advisory locking is a Phase 12 concern |

## 5. Audit & Compliance Readiness

**Evidence produced:** P11D6 + P11D7 (after M2 fix) are non-vacuous round-trip tests. P11D6 covers two aesthetic endpoints (case 0 and case 2) and the boundary (k_count, name(0), name(5)). P11D7 proves factory-to-file-to-fresh-proc fidelity via TemporaryFile (CI-safe, no persistent artifacts).

**Silent failure prevention:** SR1 bounds guard prevents silent wrong-preset loads that would be nearly impossible to debug — user sees incorrect feel groove with no error, no log entry, no crash. Adding the status message "Preset unavailable - refresh list" creates a visible signal.

**Post-incident reconstruction:** All preset saves go through PresetManager::save() → save_to_file() which writes a BQP1-headered binary blob. The format is versioned (BQP1 magic + meta XML with name/category). Sufficient for state reconstruction.

**Hosting hazard (M1 pre-fix):** AlertWindow modal would leave no audit trail and is untestable in CI — a latent crash that only manifests on specific host+OS combinations. Post-fix: inline TextEditor is always visible, stateless, and CI-testable if needed.

## 6. Final Release Bar

**Before plan ships:**
- M1 applied: no `AlertWindow::showInputBox` in any src/ file; inline TextEditor used
- M2 applied: P11D6/P11D7 compile with `juce::ScopedJuceInitialiser_GUI init; BaqueProcessor proc;`
- SR1 applied: user_presets_ access guarded
- SR2 applied: P11D6 tests index 0 + boundary; no name(6) call
- 251/251 tests pass (0 regressions, P11D6+P11D7 green)
- Build clean on all 3 platforms (no new warnings)

**Remaining risks if shipped as-is (pre-fix):**
- Hosting deadlock/freeze in hosts with strict message-thread policies (M1) — intermittent, user-reproducible, hard to debug
- Compile failure on APPLY (M2) — immediate, deterministic

**Sign-off statement:** Would sign after M1+M2+SR1+SR2 applied. The factory preset architecture (FeelPresets delegation, BQP1 format, in-process load_into) is sound and Phase-12-ready.

---

**Summary:** Applied 2 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
