---
phase: 10-ui-ux
plan: 02
type: Summary
about: "BAQUE"
completed: 2026-06-15
tests_at_close: 226
---

# Plan 10-02 Summary — LookAndFeel + NAV Shell

**PLAN ✓ → AUDIT ✓ → APPLY ✓ → UNIFY ✓**
**Human-verify:** PENDING (standalone build required; see deferred section)
**CI gate:** PENDING (push to origin; SR4 requires 3-platform green before 10-03)

---

## What Was Built

### `src/ui/` — new directory, 8 files

| File | Role |
|------|------|
| `baque_look_and_feel.h` + `.cpp` | JUCE LookAndFeel_V4 subclass: 4 themes, 5 role colours, custom knob/fader/button/grain render |
| `baque_knob.h` | Header-only RotaryVerticalDrag slider (1.25π–2.75π range) |
| `baque_fader.h` | Header-only LinearVertical slider |
| `baque_meter.h` + `.cpp` | Segmented stereo peak meter; 30fps timer gated on visibility |
| `header_component.h` + `.cpp` | Persistent header bar: wordmark + 6-button NAV + BPM display + master meter |

### Assets
- `assets/fonts/SpaceGrotesk-Medium.ttf` — committed to git (M1)
- `assets/fonts/JetBrainsMono-Regular.ttf` — committed to git (M1)
- Both fonts embedded in `BaqueAssets` BinaryData at build time

### Plugin Editor Restructure
- `src/plugin_editor.h` / `.cpp` — `BaqueEditor` is now a proper NAV shell:
  - `ScreenPlaceholder` inner class (6 instances, one per screen)
  - `showScreen(Screen)` public; `isScreenVisible(int)` public accessor; `activeScreen()` delegation
  - `look_and_feel_` member declared before `header_` before `screens_` (correct destruction order)
  - `setLookAndFeel(nullptr)` in destructor
- `CMakeLists.txt` — fonts in `juce_add_binary_data`, 3 new `.cpp` sources in `target_sources`

### Tests
- `tests/test_lookandfeel.cpp` — LAF1 (role colour hex), LAF2 (4 themes construct, backgrounds differ), LAF3 (static_assert), LAF4 (BaqueMeter no timer when invisible)
- `tests/test_nav_shell.cpp` — NAV1 (showScreen(FX): FX visible, PERFORM hidden, activeScreen==FX)
- Total: 226/226 pass (221 baseline + 5 new)

---

## Acceptance Criteria

| AC | Description | Result |
|----|-------------|--------|
| AC-1 | Colour token completeness — all 5 role colours match spec hex | ✓ LAF1 + LAF3 machine-verified |
| AC-2 | Custom knob/fader render | ✓ Implemented; visual confirm pending human-verify |
| AC-3 | NAV switches 6 screen placeholders | ✓ NAV1 automated; visual confirm pending |
| AC-4 | BaqueMeter reacts to ui_snapshot peaks | ✓ Implemented; wired to snapshot; visual confirm pending |
| AC-5 | Custom fonts loaded from BinaryData | ✓ loadTypefaces() uses `SpaceGroteskMedium_ttf` / `JetBrainsMonoRegular_ttf` (BinaryData symbols); typeface asserted non-null |

---

## Deviations from Plan

### DEV-1: BinaryData symbol naming (plan spec was wrong)

**Plan said:** `BinaryData::SpaceGrotesk_Medium_ttf` (hyphens → underscores)

**Actual JUCE 8 behavior:** JUCE strips hyphens entirely, does not replace with underscores.
`SpaceGrotesk-Medium.ttf` → `SpaceGroteskMedium_ttf`, `JetBrainsMono-Regular.ttf` → `JetBrainsMonoRegular_ttf`.

**Impact:** Compile error on first build. Corrected in `baque_look_and_feel.cpp`.

### DEV-2: `setSize()` must follow all child construction

**Plan code showed** `setSize()` before the `screens_` creation loop. `setSize()` calls `resized()` immediately, which iterates `screens_[]` — null unique_ptrs → SIGSEGV.

**Fix:** Moved `setSize()` / `setResizable()` / `setResizeLimits()` to AFTER all child components are created and wired. Added explanatory comment in constructor.

### DEV-3: Deprecated `juce::Font` constructors

**Plan spec used** `juce::Font(typeface)` and `juce::Font(float)` — deprecated in JUCE 8 with warning.

**Fix:** Replaced with `juce::Font(juce::FontOptions(typeface).withHeight(x))` and `juce::Font(juce::FontOptions{}.withHeight(x))`.

### DEV-4: NAV1 test requires `ScopedJuceInitialiser_GUI`

**Plan described** constructing BaqueProcessor + BaqueEditor in headless test. Headless GUI component construction requires `juce::ScopedJuceInitialiser_GUI init;` — same pattern used throughout Phase 6–9 tests (e.g., `test_fx_chain.cpp`). Added to NAV1 fixture.

### DEV-5: BaqueKnob / BaqueFader constructors simplified

**Plan showed** `explicit BaqueKnob(const juce::String& name = {})` calling base with args. Implemented as default constructor calling `setSliderStyle()` / `setTextBoxStyle()` / `setRotaryParameters()` directly — cleaner and avoids juce::String dependency in header.

---

## Decisions Made

**D1 — `private juce::Timer` + public forwarding `isTimerRunning()`**
BaqueMeter inherits Timer privately (Timer is an implementation detail). Test access uses a public one-liner: `bool isTimerRunning() const noexcept { return juce::Timer::isTimerRunning(); }`. No dynamic_cast, no friend.

**D2 — `ScreenPlaceholder` implemented inline in `plugin_editor.h`**
Small enough to be header-inline (constructor + one paint override). Avoids a separate .cpp for a placeholder. Will be removed/replaced in each screen plan (10-03–10-07).

**D3 — No `activeScreen_` in `HeaderComponent`; `active_screen_` owned by `BaqueEditor`**
`BaqueEditor::activeScreen()` delegates to its own `active_screen_` member set in `showScreen()`. `HeaderComponent::setActiveScreen()` manages NAV button toggle state only. Avoids `BaqueEditor` having to query its header for state it already owns.

---

## Deferred Items

| # | Item | Deferred to |
|---|------|-------------|
| 1 | Human-verify checkpoint (standalone launch, visual NAV/theme/meter confirmation) | Before 10-03 starts — user must approve |
| 2 | CI green on all 3 platforms (SR4) | Push to origin; monitor CI before 10-03 |
| 3 | Font license NOTICE file in `assets/fonts/` (OFL audit finding) | Before public release |
| 4 | Transport play/stop wiring in HeaderComponent | Plan 10-03 |
| 5 | BPM / FEEL / AGE interactive controls in header | Plan 10-03 |
| 6 | `push_ui_command()` from any HeaderComponent click | Plan 10-03 |
| 7 | Wordmark "B" in ember + "AQUE" in text() contrast rendering | Currently single-colour label; refine in 10-03 paint pass |

---

## Files Created / Modified

**Created:**
- `src/ui/baque_look_and_feel.h`
- `src/ui/baque_look_and_feel.cpp`
- `src/ui/baque_knob.h`
- `src/ui/baque_fader.h`
- `src/ui/baque_meter.h`
- `src/ui/baque_meter.cpp`
- `src/ui/header_component.h`
- `src/ui/header_component.cpp`
- `assets/fonts/SpaceGrotesk-Medium.ttf`
- `assets/fonts/JetBrainsMono-Regular.ttf`
- `tests/test_lookandfeel.cpp`
- `tests/test_nav_shell.cpp`
- `.paul/phases/10-ui-ux/10-02-PLAN.md`
- `.paul/phases/10-ui-ux/10-02-AUDIT.md`

**Modified:**
- `src/plugin_editor.h` — NAV shell restructure
- `src/plugin_editor.cpp` — NAV shell restructure
- `CMakeLists.txt` — fonts + new cpp sources
- `tests/CMakeLists.txt` — 2 new test files
- `.paul/STATE.md` — loop position updated
- `.paul/paul.toml` — loop position updated
- `.paul/ledger.toml` — unify entry appended

---

## Provides (for downstream plans)

- `src/ui/` convention: all UI components live here, compiled into BAQUE target
- `BaqueLookAndFeel` — use for all custom rendering; call `setTheme()` to switch; `paintGrainOverlay()` available in any `paint()` override
- `BaqueKnob` / `BaqueFader` — drop-in controls for screen plans
- `BaqueMeter` — drop-in for any screen needing peak display
- `HeaderComponent::Screen` enum — canonical screen index (PERFORM=0…MIDI=5); replaces in 10-03–10-07
- `BaqueEditor::showScreen()` public — 10-03 screen plan can call this after wiring its component
- `UiStateSnapshot` still the only read path from UI to audio — no writes from any 10-02 component

---

*Loop closed 2026-06-15 — human-verify + CI pending before 10-03*
