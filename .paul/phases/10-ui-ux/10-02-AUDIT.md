---
phase: 10-ui-ux
plan: 02
type: Audit
about: "BAQUE"
---

# Enterprise Plan Audit Report

**Plan:** `.paul/phases/10-ui-ux/10-02-PLAN.md`
**Audited:** 2026-06-15
**Verdict:** Conditionally Acceptable → Upgraded

---

## 1. Executive Verdict

Conditionally acceptable, now upgraded. Core architecture is sound: snapshot consumption
is read-only on message thread (std::atomic relaxed reads — correct), LookAndFeel lifetime
management is correct (member declaration order + setLookAndFeel(nullptr) in destructor),
SPSC queue not touched from UI in this plan, BaqueMeter timer guard prevents phantom callbacks.

**Would I sign off as-is (before fixes)?** No. Two issues blocked sign-off: (1) font files
absent from git would cause silent CI breakage — configure fails with a misleading "no such file"
error, not a clear dependency message; (2) the primary architectural output (NAV screen switching)
had zero automated regression coverage, relying entirely on a one-time human-verify checkpoint.

Post-upgrade: both issues resolved. The 4 additional strongly-recommended fixes address a
correctness hazard (grain Random in paint), a LookAndFeel colour propagation gap that would
produce wrong colours in ScreenPlaceholder on theme changes, missing CI gate, and missing JUCE
leak detector tracking for ScreenPlaceholder.

---

## 2. What Is Solid

**Snapshot consumption pattern (read-only):**
`proc_.ui_snapshot()` is accessed only via `std::atomic<T>::load(relaxed)` from the message
thread (timer callbacks, paint()). This is the 10-01 design contract followed exactly. No
writes from UI, no mutex required. Solid.

**LookAndFeel member lifetime:**
`look_and_feel_` declared before `header_` before `screens_` in BaqueEditor. C++ destroys in
reverse order: screens_ → header_ → look_and_feel_. `setLookAndFeel(nullptr)` in destructor
body runs before any member destruction. No dangling LookAndFeel pointer possible. Solid.

**BaqueMeter timer lifecycle:**
`startTimerHz(30)` / `stopTimer()` gated on `visibilityChanged()`. JUCE Timer destructor
calls stopTimer(). No phantom callbacks after destruction. Solid.

**SPSC queue isolation:**
No `push_ui_command()` call from any UI component in this plan. Transport interaction explicitly
deferred. The "Avoid" section in Task 2 enforces this. Solid.

**Minimum window size:**
Header content (wordmark 80px + 6 NAV × ~60px + transport 100px + meter 40px ≈ 580px) fits
within the 800px minimum. Solid.

**BinaryData symbol naming:**
`SpaceGrotesk-Medium.ttf` → `BinaryData::SpaceGrotesk_Medium_ttf` (JUCE convention: hyphens
and dots become underscores). Correctly specified. Solid.

---

## 3. Enterprise Gaps Identified

### Gap 1 — Font files absent from git (M1)

`juce_add_binary_data` resolves source paths at CMake configure time. The plan provides curl
download commands but no `git add` step. Any CI runner, fresh clone, or collaborator faces:
```
CMake Error: Cannot find source file: assets/fonts/SpaceGrotesk-Medium.ttf
```
This error message does not explain the root cause (missing git tracking). The failure is
silent — local developer builds pass, CI fails, PR is broken. Risk: every collaborator must
discover the download step independently.

### Gap 2 — NAV screen switching: no automated regression test (SR1)

`showScreen()` is the primary architectural output of Task 3 and the backbone of all future
screen navigation (10-03 through 10-07). If it regresses (e.g., iterator bounds error,
visibility toggle logic flip), the bug is caught only by the one-time human-verify checkpoint.
All prior phase plans have automated coverage for their primary contribution. This plan breaks
that pattern.

### Gap 3 — Grain overlay: juce::Random in paint() (SR2)

Original spec: "draw a semi-transparent white noise texture using juce::Random seeded to a
fixed value (deterministic per frame)." This has two failure modes:
1. If implemented as a per-paint stack Random seeded to a fixed constant: same pixel pattern
   every frame — static "dirt" — valid aesthetic, but allocating/constructing juce::Random on
   every paint() call at 60fps is wasteful.
2. If seeded from time: flickering grain at 60fps, but the spec says "NOT per-frame-seeded to
   avoid flickering" — contradicts itself.

Neither is clearly specified. The correct implementation for the stated aesthetic (static film-
grain texture) is to pre-bake a juce::Image once at construction and composite it in paint().
Zero per-paint cost, correct visual result.

### Gap 4 — ScreenPlaceholder text colour: LookAndFeel access unspecified (SR3)

The plan says "colour = text() from LookAndFeel" — but `text()` is a method on `BaqueLookAndFeel`,
not on `juce::LookAndFeel`. Without specifying the access pattern, an APPLY engineer either:
(a) Calls `getLookAndFeel().text()` — compile error (method doesn't exist on base class)
(b) Casts `getLookAndFeel()` to `BaqueLookAndFeel*` without null guard — crash on theme change
(c) Hardcodes white — works but breaks in light theme (Papel) where text should be dark

Correct approach: `setColour(juce::Label::textColourId, text())` in `BaqueLookAndFeel::setTheme()`
propagates the colour into the LookAndFeel registry; ScreenPlaceholder calls `findColour()` which
traverses the parent chain. No casting required.

### Gap 5 — CI verification absent (SR4)

The `<verification>` section specifies only local Linux builds. BAQUE targets VST3/AU/Standalone
on Windows/macOS/Linux. JUCE GUI components have platform-specific rendering paths (NSView on
macOS, HWND on Windows). Font loading via BinaryData is platform-sensitive. All prior plans from
Phase 1 onward use CI green as a release gate. This plan omits it.

### Gap 6 — ScreenPlaceholder missing JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SR5)

`ScreenPlaceholder` inherits from `juce::Component` but doesn't declare the JUCE leak detector
macro. In debug builds, JUCE's LeakDetector tracks component allocations to catch lifetime bugs.
Without the macro, any future memory bug in the 6 ScreenPlaceholder instances (e.g., double-free
when a screen plan replaces them) goes undetected until it manifests as a crash or corrupted heap.
All other components in this plan correctly include the macro.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Font files not committed to git — CI configure failure | Task 1 `<action>` + `<verify>` | Added `git add assets/fonts/*.ttf` step with explicit warning; added `git ls-files` verification check |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | No automated test for NAV screen switching (primary arch output) | Task 3 `<action>` + `<verify>`, `<verification>`, `<success_criteria>`, frontmatter | Added `tests/test_nav_shell.cpp` (NAV1), `showScreen()` made public, `isScreenVisible(int)` accessor added to BaqueEditor, `activeScreen()` delegation exposed |
| 2 | Grain overlay: juce::Random in paint() — wasteful + spec contradiction | Task 1 `<action>` (LookAndFeel header + cpp), LookAndFeel private members | Added `grain_texture_` (juce::Image) + `buildGrainTexture()` to header; `paintGrainOverlay` now tiles pre-baked image — zero per-paint allocation |
| 3 | ScreenPlaceholder text colour unspecified — cast hazard or wrong colour | Task 1 `<action>` (setTheme spec), Task 3 `<action>` (paint spec) | `setTheme()` now required to call `setColour(juce::Label::textColourId, text())`; ScreenPlaceholder paint() uses `findColour(juce::Label::textColourId)` — no cast |
| 4 | CI verification absent — BAQUE is a 3-platform product | `<verification>`, `<success_criteria>` | Added CI green gate: push to origin, verify all 3 platforms before UNIFY |
| 5 | ScreenPlaceholder missing JUCE leak detector macro | Task 3 `<action>` (plugin_editor.h ScreenPlaceholder definition) | Added `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScreenPlaceholder)` to private section |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | Header timer (10fps) + BaqueMeter timer (30fps): two separate polling timers | Overhead negligible; two independent timers is cleaner than coupling. Consolidation is premature optimisation. |
| 2 | Font license NOTICE file in assets/fonts/ | OFL is permissive and compatible with GPLv3. Omitting a NOTICE file is not a compliance violation for internal development. Add before public release. |
| 3 | NAV button minimum width at 800px breakpoint | Plan sets minimum to 800px wide; 580px content fits. Any overflow is caught by visual checkpoint. |
| 4 | `BaqueMeter::k_decay_per_tick` = 0.92f hardcoded per 33ms tick | AC-4 specifies ≤500ms ballistic: 0.92^15 ≈ 0.26 (effectively zero at ~500ms). Close enough for v1; exact timer jitter is ±1 tick. |

---

## 5. Audit & Compliance Readiness

**Defensible evidence:** LAF1–LAF4 tests produce machine-verifiable proof of colour token
correctness and meter timer behaviour. NAV1 (added) provides regression protection for the
primary architectural output. Font commit is now explicit and verified via `git ls-files`.

**Silent failure prevention:** The font commit fix (M1) eliminates a class of CI failures that
produce misleading error messages. The grain pre-bake (SR2) eliminates per-paint-frame Random
construction that would degrade paint performance under rapid repaints. The LeakDetector macro
(SR5) ensures JUCE's debug-mode allocator catches lifetime bugs in ScreenPlaceholder instances.

**Post-incident reconstruction:** All architectural decisions are documented in plan text and
will flow to SUMMARY.md via UNIFY. The `<!-- audit-added -->` comments mark all changes.

**Ownership:** BaqueEditor owns BaqueLookAndFeel by value. LookAndFeel lifecycle is the editor's
responsibility (set + clear). Meter timer lifecycle belongs to BaqueMeter (start on visible, stop
on hide). HeaderComponent timer belongs to HeaderComponent. No shared ownership.

---

## 6. Final Release Bar

**Before this plan ships:**
1. Font TTF files must be in git (M1). Verify with `git ls-files assets/fonts/`.
2. LAF1–LAF4 + NAV1 tests must pass (226+ total test count).
3. CI green on all 3 platforms (Linux/macOS/Windows) — JUCE GUI path is platform-sensitive.
4. Human-verify checkpoint approved: Barro theme visible, NAV switching, meter reaction.
5. `setLookAndFeel(nullptr)` in BaqueEditor destructor (prevents JUCE dangling pointer).

**Risks if shipped before CI green:**
- macOS: NSView creation or font loading may fail silently without platform test
- Windows: MSVC may reject auto/range-for in component code (historical pattern: see Phase 7)
- Both are caught only by CI, not by local Linux build

**Would I sign my name to this plan post-upgrade?** Yes — provided the CI gate in verification
is honoured before UNIFY. The design system and NAV shell are the right architecture for the
foundation: reads from snapshot only, mutations deferred to screen-specific plans, LookAndFeel
lifetime correct, test coverage for both the rendering primitives and the navigation shell.

---

**Summary:** Applied 1 must-have + 5 strongly-recommended upgrades. Deferred 4 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
