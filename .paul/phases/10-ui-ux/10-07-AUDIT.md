---
phase: 10-ui-ux
plan: 07
audited: 2026-06-18T00:00:00Z
auditor: senior-principal-engineer
verdict: conditionally-acceptable
type: Audit
about: "BAQUE"
---

# Plan 10-07 Audit Report

**Plan:** BrowserScreen + undo/redo + Phase 10 DoD  
**Verdict:** Conditionally acceptable — 4 findings applied; plan approved for APPLY.

---

## 1. Executive Summary

Plan is architecturally sound. Thread lifecycle, safe-load protocol, FileChooser async pattern, and undo_manager_ init order all correct. Four gaps identified and auto-applied: one production UAF (M1), one silent data loss for stereo files (SR1), one vacuous test (SR2), and one CPU waste (SR3).

---

## 2. What Is Solid

| Area | Assessment |
|------|-----------|
| Safe-load protocol | `voice_pool_.reset_all()` before buffer mutation — correct, consistent with Phase 4 audit 04-01 |
| Thread lifecycle | `thread_` declared before `contents_`; `stopThread(2000)` explicitly in dtor body before member dtors fire — prevents UAF during scan |
| jassert message-thread guard | Same pattern as `UiCommandQueue::push()` — passes in CI (no MessageManager → short-circuits), enforces in production |
| `undo_manager_` init order | Declared before `apvts_` in header → initialized first → safe to take `&undo_manager_` in APVTS initializer list |
| FileChooser stack lifetime | `std::make_shared<juce::FileChooser>` captured in lambda — correct, avoids stack destruction before async callback fires |
| APVTS undo wiring | `apvts_(*this, &undo_manager_, ...)` is the correct JUCE pattern; only APVTS parameter edits are undoable (UiCommands are not — correct scope for v1) |
| `static_cast<BaqueProcessor&>(processor)` | Safe: `BaqueEditor` is always paired with `BaqueProcessor`; avoids RTTI overhead in host callback path |

---

## 3. Findings

### M1 — FileChooser async callback captures raw `this` (UAF) [MUST-HAVE — release-blocking]

**Location:** `browser_screen.cpp`, `root_btn_.onClick` lambda  
**Problem:** Original plan specified `[this, fc](...)`. If `BrowserScreen` is destroyed (tab closed, plugin window hidden) while the OS file dialog is open, the async callback fires with a dangling `this` pointer → UAF / crash.  
**Why it matters:** This is a production-only path (not tested in CI), making it easy to miss. The OS dialog can stay open for seconds; component destruction is common during that window.  
**Fix applied:** Replace `[this, fc]` with `[safeThis, fc]` where `safeThis = juce::Component::SafePointer<BrowserScreen>(this)`. Add `if (safeThis == nullptr) return;` guard at start of lambda. `SafePointer` automatically becomes `nullptr` when the component is destroyed — JUCE's standard idiom for this pattern.

### SR1 — `load_sample_from_file` reads left channel only for stereo WAVs [STRONGLY RECOMMENDED]

**Location:** `plugin_processor.cpp`, `load_sample_from_file()`  
**Problem:** `buf.setSize(1, num_samples)` + `reader->read(&buf, 0, num_samples, 0, true, false)` reads only the left channel. Stereo WAVs silently lose the right channel. Producer loads a stereo drum loop → hears only left-panned content → mono sounds incorrect or thin.  
**Fix applied:** Check `reader->numChannels >= 2`. If stereo: read into a 2-channel `juce::AudioBuffer<float>`, then downmix `(L + R) * 0.5f` into the 1-channel `buf`. Mono files take the existing fast path unchanged.

### SR2 — P10D4 is vacuous: `isScreenVisible()` passes for ScreenPlaceholder too [STRONGLY RECOMMENDED]

**Location:** `tests/test_phase10_dod.cpp`, `P10D4`  
**Problem:** Original P10D4 checked `editor.isScreenVisible(static_cast<int>(Screen::BROWSER))` after `showScreen(BROWSER)`. This check passes whether the BROWSER slot is a `BrowserScreen` OR a `ScreenPlaceholder` — `showScreen()` makes any slot visible. Test does not prove the slot contains the correct type.  
**Fix applied:** Added `[[nodiscard]] juce::Component* getScreen(Screen s) const noexcept { return screens_[static_cast<int>(s)].get(); }` to `BaqueEditor` public API (in plugin_editor.h). Updated P10D4 to call `editor.getScreen(Screen::BROWSER)` and verify `dynamic_cast<BrowserScreen*>(comp) != nullptr`. This is a real type-identity check.

### SR3 — `timerCallback` polls 2fps indefinitely after directory scan completes [STRONGLY RECOMMENDED]

**Location:** `browser_screen.cpp`, `timerCallback()`  
**Problem:** Original plan's `timerCallback()` calls `file_list_.updateContent(); repaint();` forever at 2fps, even after `juce::DirectoryContentsList` finishes scanning. This wastes CPU (minor but continuous) for the entire plugin session once BROWSER is first shown.  
**Fix applied:** Added `if (!contents_.isStillLoading()) stopTimer();` in `timerCallback`. Timer is restarted in two places: `visibilityChanged()` (when screen shown) and the `root_btn_.onClick` FileChooser callback (when user picks a new folder and a new scan begins). This ensures the poll runs only during active scans.

---

## 4. Deferred (Justified)

| Item | Justification |
|------|--------------|
| UndoManager not persisted across `setStateInformation` | Correct behavior: `juce::UndoManager` is session-local; replaying undo across preset loads is undefined. Document as intentional scope limit. |
| FileChooser unavailable in headless CI | Not testable in process; `launchAsync` is a no-op without display. CI tests don't exercise this path. Runtime-only feature. |
| Subdirectory navigation via FileListComponent | `FileListComponent` shows files only at the selected root. Users change root via SET FOLDER. Acceptable v1 UX constraint; deferred to Phase 11 tree view. |
| Waveform thumbnail, auto-audition, drag-and-drop, tag filters | Phase 11 scope per ROADMAP. Correctly excluded from v1 BROWSER. |

---

## 5. Changes Applied to PLAN.md

| Finding | Section changed |
|---------|----------------|
| M1 | Task 1 → `root_btn_.onClick`: replaced `[this, fc]` with SafePointer pattern |
| SR1 | Task 1 → `load_sample_from_file()` impl: added stereo downmix branch |
| SR2 | Task 2 → plugin_editor.h: added `getScreen()` accessor; Task 3 → P10D4: replaced `isScreenVisible` check with `dynamic_cast` |
| SR3 | Task 1 → `timerCallback()`: added `isStillLoading()` stop guard; M1 fix already includes `startTimerHz(2)` on new folder selection |

---

## 6. Verdict

**APPROVED FOR APPLY.**  
All must-have and strongly-recommended findings applied to PLAN.md. Plan is production-safe.
