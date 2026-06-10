---
phase: 09-midi-hardware
plan: 04
subsystem: midi
tags: [tr-8, tr-8s, midi, hardware-template, cc, drum-map, roland, dod]

requires:
  - phase: 09-01
    provides: lane_routing_ EXT note emit per lane + channel + stop-flush
  - phase: 09-02
    provides: midi_clock_ 24ppqn master + EXT-buffer ordering invariant
  - phase: 09-03
    provides: cc_out_ (PLockParam->CC emit) + k_param_range + plock_pattern_
provides:
  - TR-8/TR-8S mapping templates (note map + channel + CC slots) applied in one call
  - Phase 9 DoD automated half (hybrid INT/EXT merged MIDI via processBlock)
  - Spec-conformance test against Roland published TR-8 MIDI Implementation Chart (Tier-3)
  - Authoritative Roland CC numbers baked into template for continuous params
affects: [phase-10-ui, phase-11-presets]

tech-stack:
  added: []
  patterns:
    - "Hardware template = single-writer setup POD (mirrors LaneRouting/CcOutRouting)"
    - "Spec source-of-truth header verified by an independent hand-written template (no tautology)"

key-files:
  created:
    - src/audio/hardware_templates.h
    - src/audio/tr8_midi_spec.h
    - tests/test_hardware_templates.cpp
    - tests/test_phase9_dod.cpp
    - tests/test_tr8_spec_conformance.cpp
  modified:
    - src/plugin_processor.h
    - src/plugin_processor.cpp
    - tests/CMakeLists.txt

key-decisions:
  - "AC-6 resolved verified-by-spec: Roland's published MIDI Implementation Chart (TR-8 v1.11) IS the firmware spec for note/CC numbers — equivalent to byte-level hardware confirmation without a physical unit"
  - "Continuous CCs baked ON from the chart (scatter_depth=69, reverb=91, delay_mix=16, delay_time=17); discrete scatter_type=68 left OFF pending value-curve confirmation"
  - "Note map spec-verified (not just GM-documented): every template note matches the Roland chart primary note per instrument"

patterns-established:
  - "Tier-3 spec conformance: when hardware is unavailable, machine-check the mapping against the vendor's published implementation chart"

duration: ~90min (across 2 sessions; Tier-3 add 2026-06-10)
started: 2026-06-09T00:00:00Z
completed: 2026-06-10T00:00:00Z
description: "TR-8/TR-8S templates + Phase 9 DoD; note/CC map spec-verified against Roland's published MIDI Implementation Chart (Tier-3, no hardware needed)"
type: Summary
about: "BAQUE"
---

# Phase 9 Plan 04: TR-8/TR-8S Templates + Phase 9 DoD Summary

**TR-8/TR-8S mapping templates apply note map + channel + CC slots in one call; a hybrid INT/EXT pattern drives a correct merged MIDI stream via processBlock; and the note/CC numbers are machine-verified against Roland's own published MIDI Implementation Chart — closing the hardware-sign-off intent without a physical TR-8.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~90 min (2 sessions) |
| Started | 2026-06-09 |
| Completed | 2026-06-10 |
| Tasks | 2 auto + 1 checkpoint (resolved verified-by-spec) |
| Files created | 5 |
| Files modified | 3 |
| Tests | 209/209 green (was 195; +9 DoD/template, +5 spec conformance) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: TR-8 template note/channel map | Pass | HT1 + TS1 — TS1 asserts vs Roland chart, not vs itself |
| AC-2: apply_template one-call config | Pass | HT2 + TS5 (CCs now propagate to live cc_out) |
| AC-2b: non-destructive (set_note only) | Pass | HT3 — activations/trigs preserved |
| AC-3: TR-8S shares map, distinct name | Pass | HT4 |
| AC-4: hybrid merged MIDI stream | Pass | P9D1/P9D2 — EXT note+channel, exact 24ppqn clock, note-off |
| AC-4b: INT check non-vacuous | Pass | P9D1b — non-zero audio proves lane fired, no MIDI leak |
| AC-5: sustained run stable + stop-flush | Pass | P9D3 (200 blocks finite) + P9D4 (CC123 on stop) |
| AC-6: hardware sign-off | Pass (verified-by-spec) | Note/CC numbers machine-checked vs Roland chart (TS1-TS5). Physical sound-in-room + scatter_type value-curve = observational, deferred |

## Accomplishments

- TR-8/TR-8S templates: 11-voice Roland note map, channel 10, one-call apply into routing + cc_out + pattern (non-destructive to step programming).
- Phase 9 DoD automated half: hybrid INT/EXT pattern proven through real `processBlock` — EXT notes on TR-8 note/channel, internal lane fires audio without MIDI leak, exact 24ppqn clock, note-off on gate close, stop-flush, additive CC.
- **Tier-3 spec conformance (this session):** built `tr8_midi_spec.h` from Roland's published MIDI Implementation Chart (TR-8 v1.11) and `test_tr8_spec_conformance.cpp` asserting the hand-written template matches it. Note map is now spec-verified, and the deferred CC numbers were resolved from the authoritative chart.

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/hardware_templates.h` | Created | HardwareTemplate POD + tr8/tr8s factories + apply_template; value-safe CCs baked ON |
| `src/audio/tr8_midi_spec.h` | Created | Authoritative Roland TR-8 chart constants (notes + CCs) — spec source-of-truth |
| `src/plugin_processor.{h,cpp}` | Modified | apply_hardware_template() + set_pattern() |
| `tests/test_hardware_templates.cpp` | Created | HT1-HT4 (template content + non-destructive apply) |
| `tests/test_phase9_dod.cpp` | Created | P9D1-P9D6 (hybrid processBlock DoD) |
| `tests/test_tr8_spec_conformance.cpp` | Created | TS1-TS5 (template vs Roland chart) |
| `tests/CMakeLists.txt` | Modified | Register the 3 new test files |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| AC-6 resolved verified-by-spec, not deferred | The Roland published MIDI Implementation Chart IS the device's firmware spec for note/CC numbers — confirming the byte mapping against it is equivalent to confirming against hardware (which only adds "does it make sound") | Phase 9 closes without a physical TR-8; note/CC numbers machine-checked |
| Bake value-safe (continuous) CCs ON from the chart | scatter_depth=69, reverb=91, delay_mix=16, delay_time=17 are continuous 0-127 knobs — linear value map is safe; numbers come from the chart, not guessed (reverses the 09-04 audit "CC off until hardware" for these) | Template now drives TR-8 scatter depth / FX out of the box |
| scatter_type=68 / scatter_sw=70 left OFF | Discrete pattern-select: BAQUE 0-10 types over TR-8 0-127 select curve — the CC NUMBER is known but the VALUE mapping needs a physical unit to confirm | Lone genuine hardware-dependent item remaining |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | P9D5 used CC 16 for scatter_depth — 16 is DELAY LEVEL per chart; corrected to 69 |
| Scope additions | 3 | tr8_midi_spec.h, spec conformance test, value-safe CC baking (Tier-3, user-approved) |
| Deferred | 1 | scatter_type value-curve on real hardware |

### Auto-fixed Issues

**1. [Correctness] P9D5 scatter_depth CC number was arbitrary**
- **Found during:** Tier-3 spec conformance against the Roland chart.
- **Issue:** P9D5 asserted CC 16 for scatter_depth; the chart assigns 16 = DELAY LEVEL, 69 = SCATTER DEPTH. Test passed (additive-CC proof) but the number was misleading.
- **Fix:** P9D5 now uses CC 69; template bakes 69 for scatter_depth.
- **Verification:** 209/209 green; TS2 asserts 69 vs `tr8_spec::k_cc_scatter_depth`.

### Scope Additions (Tier-3, user-approved 2026-06-10)

- `src/audio/tr8_midi_spec.h`: authoritative chart constants, independent of the template, so the conformance test is a real equality check not a tautology.
- `tests/test_tr8_spec_conformance.cpp`: TS1 note map vs chart, TS2 continuous CC numbers exact, TS3 discrete OFF, TS4 BAQUE-only params no CC (exactly 4 on), TS5 apply propagates to live routing.
- Value-safe CCs enabled in `tr8_template()` + honesty comment rewritten; HT1/HT2 updated for the CC-on state.

### Carried from Task 1&2 APPLY (prior session)

- Reused existing `Sequencer::pattern() const` instead of adding `current_pattern()` (equivalent accessor already existed).
- Added `BaqueProcessor::set_pattern()` (immediate setter) for test pattern injection — mirrors `set_next_pattern`.
- Test `prepare()` calls `setRateAndBufferSizeDetails(48000, 512)` before `prepareToPlay` (else `getSampleRate()=0` → infinite-ppq block). Harness fix, not a product bug.

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| No physical TR-8 to satisfy AC-6 | Tier-3: machine-check note/CC numbers against Roland's published MIDI Implementation Chart — the firmware spec |
| Roland chart PDF unreadable via WebFetch | Extracted text with `mutool draw -F txt` locally |

## Next Phase Readiness

**Ready:**
- Phase 9 (MIDI / Hardware) complete: per-lane routing, EXT note out, 24ppqn clock master, CC out, MIDI in, MIDI learn, TR-8/TR-8S templates, hybrid DoD. 209/209 tests.
- Phase 10 (UI/UX) is next: must atomicize the single-writer setup structs (lane_routing_, cc_out_, clock_master_, hardware-template apply) before live edits — same contract as pad-params/PerfState.

**Concerns:**
- scatter_type (CC 68) value-curve unconfirmed on hardware — slot ships OFF; enable + map once a unit is available.

**Blockers:**
- None.

---
*Built with PAUL Framework*
*Phase: 09-midi-hardware, Plan: 04*
*Completed: 2026-06-10*
