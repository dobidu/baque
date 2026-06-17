---
phase: 10-ui-ux
plan: 05
subsystem: ui
tags: [juce, apvts, slider-attachment, timer, fx-screen, mix-screen, uicommand]

requires:
  - phase: 10-01
    provides: UiCommandQueue, UiStateSnapshot, push_ui_command() contract
  - phase: 10-02
    provides: BaqueLookAndFeel, BaqueKnob, BaqueFader, BaqueMeter, NAV shell, ScreenPlaceholder
  - phase: 06-02
    provides: APVTS params (filter_cutoff, filter_res, reverb_mix, delay_time, delay_mix, sidechain_threshold, master_gain)

provides:
  - FxScreen — 6 APVTS-backed knobs replacing FX ScreenPlaceholder
  - MixScreen — 16-strip mixer (fader/pan/M/S/meter) + master strip, timer-gated at 30fps
  - getAPVTS() named message-thread accessor on BaqueProcessor

affects: [10-06, 10-07, Phase 11 presets]

tech-stack:
  added: []
  patterns:
    - "Screen forward-declares BaqueProcessor in .h, includes plugin_processor.h in .cpp"
    - "SliderAttachment declared after its Slider — RAII destruction order correct"
    - "Timer gated by visibilityChanged() + stopTimer() in dtor (established Phase 10 pattern)"
    - "UiCommand for per-lane mutations; APVTS SliderAttachment for master_gain only"

key-files:
  created:
    - src/ui/fx_screen.h
    - src/ui/fx_screen.cpp
    - src/ui/mix_screen.h
    - src/ui/mix_screen.cpp
    - tests/test_fx_mix_screens.cpp
  modified:
    - src/plugin_processor.h  (getAPVTS() getter)
    - src/plugin_editor.h     (fx_screen.h + mix_screen.h includes)
    - src/plugin_editor.cpp   (FxScreen + MixScreen constructed in screens_ slots)
    - CMakeLists.txt          (fx_screen.cpp + mix_screen.cpp sources)
    - tests/CMakeLists.txt    (test_fx_mix_screens.cpp added)

key-decisions:
  - "getAPVTS() is a named accessor — apvts_ was already public; getter adds message-thread-only semantics"
  - "mute/solo via UiCommand (not APVTS) — DAW automation deferred to 10-07"
  - "mute_state_[]/solo_state_[] are message-thread-local; diverge after preset load (documented, deferred)"
  - "APVTS mute/solo automation re-deferred from 10-05 to 10-07"

patterns-established:
  - "Screen .h forward-declares BaqueProcessor; .cpp includes plugin_processor.h — M1 pattern"
  - "attachments_ declared after knobs_ — RAII order ensures SliderAttachment destroyed before Slider"

duration: ~1 session
started: 2026-06-17T00:00:00Z
completed: 2026-06-17T00:00:00Z
description: "FxScreen (6 APVTS knobs) + MixScreen (16-strip mixer, 30fps timer-gated) replacing ScreenPlaceholders for FX and MIX slots"
type: Summary
about: "BAQUE"
---

# Phase 10 Plan 05: FX + MIX Screens Summary

**FxScreen (6 APVTS knobs) + MixScreen (16-strip mixer, 30fps timer-gated) ship — FX and MIX ScreenPlaceholders replaced.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~1 session |
| Started | 2026-06-17 |
| Completed | 2026-06-17 |
| Tasks | 4/4 completed (3 auto + 1 human-verify) |
| Files modified | 10 (5 created, 5 modified) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: FX screen shows 6 APVTS-backed knobs | Pass | filter_cutoff, filter_res, reverb_mix, delay_time, delay_mix, sidechain_threshold — all via SliderAttachment |
| AC-2: FX screen replaces ScreenPlaceholder | Pass | screens_[Screen::FX] = FxScreen, not ScreenPlaceholder |
| AC-3: MIX screen shows 16 lane strips + master | Pass | 16 fader+pan+M/S strips + master fader + L/R meter bars — human-verified |
| AC-4: MIX lane controls push commands | Pass | set_pad_gain/set_pad_pan/set_mute/set_solo via push_ui_command() |
| AC-5: MIX timer gates on visibility | Pass | MIX1 test: CHECK_FALSE → setVisible(true) → CHECK → setVisible(false) → CHECK_FALSE |
| AC-6: MIX paint smoke | Pass | MIX2: paint() on 1200×700 off-screen image, no crash |
| AC-7: FX paint smoke | Pass | FX1: paint() on 1200×700 off-screen image, no crash |

## Accomplishments

- FxScreen ships: 6 labeled knobs (CUTOFF, RESO, REVERB, DELAY TIME, DELAY MIX, SIDECHAIN) APVTS-attached, 3×2 grid layout
- MixScreen ships: 16 per-lane strips each with BaqueFader + BaqueKnob (pan) + M/S TextButtons + velocity meter decay bar; master strip with BaqueFader + L/R peak meter bars, SliderAttachment on master_gain
- getAPVTS() named message-thread accessor added — documented as named accessor, not capability unlock (apvts_ was already public)
- 234/234 tests pass (3 new: FX1, MIX1, MIX2)

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| All tasks | `29927ba` | feat(10): Plan 10-05 APPLY — FX + MIX screens |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/ui/fx_screen.h` | Created | FxScreen class — 6 knobs + 6 labels + 6 SliderAttachments |
| `src/ui/fx_screen.cpp` | Created | Constructor wires attachments; paint fills background; resized() 3×2 grid |
| `src/ui/mix_screen.h` | Created | MixScreen class — 16-strip mixer, Timer private base, known-limitation comment |
| `src/ui/mix_screen.cpp` | Created | 16-lane setup, master SliderAttachment, timerCallback() with relaxed atomics + k_decay=0.88 |
| `tests/test_fx_mix_screens.cpp` | Created | FX1 (paint smoke), MIX1 (timer gate), MIX2 (paint smoke) |
| `src/plugin_processor.h` | Modified | getAPVTS() named message-thread accessor added after push_ui_command() |
| `src/plugin_editor.h` | Modified | Added #include "ui/fx_screen.h" and #include "ui/mix_screen.h" |
| `src/plugin_editor.cpp` | Modified | screens_[FX] + screens_[MIX] constructed before placeholder loop |
| `CMakeLists.txt` | Modified | fx_screen.cpp + mix_screen.cpp added to target_sources |
| `tests/CMakeLists.txt` | Modified | test_fx_mix_screens.cpp added to add_executable |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| getAPVTS() is named accessor, not unlock | apvts_ was already public (line 64); getter adds message-thread-only annotation matching Phase 10 accessor pattern | Corrects misleading plan comment; future devs won't think this added new access |
| mute/solo via UiCommand, not APVTS | Live UI control is the right pattern; DAW automation recording is separate concern | APVTS mute/solo automation re-deferred to 10-07 (Phase 10 DoD) |
| mute_state_[]/solo_state_[] are message-thread-local | PerfState not exposed in UiStateSnapshot in v1 | Known drift after preset load — documented in mix_screen.h class comment and boundaries |
| Forward-declare BaqueProcessor in .h, include in .cpp | Faster compile times; .h only needs pointer/ref for params | Requires explicit #include "plugin_processor.h" in both .cpp files (audit M1) |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Audit-added | 3 | M1 + 2 SR applied before APPLY — no code deviations |
| Scope additions | 0 | — |
| Deferred | 3 | APVTS mute/solo automation, state drift fix, layout tightness |

**Total impact:** Zero deviations during APPLY — audit upgrades applied to plan pre-APPLY resolved all gaps.

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| Standalone launched and exited immediately (WSL2 ALSA warning) | ALSA /dev/snd/seq warning is non-fatal in WSL2; app ran and human-verified successfully |

## Next Phase Readiness

**Ready:**
- FX and MIX screens live in NAV shell; PERF FX + MIDI screens (10-06) can be built next
- getAPVTS() accessor available for any future screen needing APVTS params
- UiCommand patterns (set_pad_gain/pan/mute/solo) established and working

**Concerns:**
- mute/solo UI state diverges from engine after preset load — deferred to 10-07 or Phase 11
- APVTS mute/solo automation (DAW recording) re-deferred to 10-07

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 10-ui-ux, Plan: 05*
*Completed: 2026-06-17*
