---
phase: 09-midi-hardware
plan: 03
subsystem: midi
tags: [midi-cc, midi-learn, juce, atomic, state-persistence, tr-8]

requires:
  - phase: 09-01
    provides: lane_routing_, midi_buffer_ext_, processBlock MIDI section, stop-flush
  - phase: 09-02
    provides: MidiClock, EXT-buffer ordering invariant (notes→stop-flush→clock→clear+addEvents)
provides:
  - midi_cc.h: ParamRange table (SR1), plock_param_norm/denorm, CcOutRouting, CcBinding, CcLearnMap
  - CC-out: p-lock values emitted as MIDI CC on EXT channel (drives TR-8 scatter/FX knobs)
  - MIDI-in: inbound CC parsed and applied to FxParams with APVTS<inbound<plock precedence
  - MIDI learn: arm/capture/bind on audio thread; atomic handshake; state v3 persistence
affects: [09-04, 10-ui, 11-presets]

tech-stack:
  added: [std::atomic<int> for learn_arm handshake]
  patterns:
    - Single source-of-truth range table (k_param_range) — norm+denorm both read it, no drift
    - Atomic arm/disarm handshake (message thread writes, audio thread reads+resets)
    - Non-copyable member (std::atomic) — mutate in-place via clear()+set_binding(), never assign
    - State rebuild-on-load with full bounds-guarding (cc[0,127], channel[1,16], target[0,14])

key-files:
  created: [src/audio/midi_cc.h, tests/test_midi_cc.cpp]
  modified: [src/plugin_processor.h, src/plugin_processor.cpp, tests/CMakeLists.txt]

key-decisions:
  - "plock_pattern_ made public: required for CC3 test (p-lock→CC path); mirrors lane_routing_/clock_master_ pattern"
  - "CcLearnMap non-copyable (std::atomic member): delete all 4 special members; rebuild in-place on state-load"
  - "State v3: cc_learn_ serialized as ValueTree child; pre-v3 loads to empty map (backward compat)"
  - "CC-out offset=0 (block-granular): PLockEvent has no sample offset; sub-block timing deferred"

patterns-established:
  - "Atomic arm/disarm: message thread store release; audio thread load acquire + reset to sentinel"
  - "Non-copyable aggregate member: never assign struct, use clear()+rebuild helpers"
  - "Phase 10 UI contract: snapshots bindings lock-free; never expose raw mutable getter (mirrors pad-params/PerfState)"

duration: ~2h
started: 2026-06-09T00:00:00Z
completed: 2026-06-09T00:00:00Z
description: "CC out (p-lock→MIDI CC for TR-8 scatter/FX), MIDI-in apply, MIDI learn with atomic handshake and state v3 persistence"
type: Summary
about: "BAQUE"
---

# Phase 9 Plan 03: CC Out + MIDI In + MIDI Learn — Summary

**CC out emits p-lock values as MIDI CC on EXT channel (drives TR-8 knobs); inbound CC applies to FxParams with APVTS<inbound<plock precedence; MIDI learn binds next controller via atomic arm/disarm handshake; bindings persist in state v3.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2h |
| Started | 2026-06-09 |
| Completed | 2026-06-09 |
| Tasks | 3 completed |
| Files modified | 5 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: P-lock fires → CC on EXT channel | Pass | scatter_depth 0.5 → CC 16 ch10 val64; CC3 test |
| AC-2: CC out disabled / unmapped emit nothing | Pass | enabled=false + unmapped params → 0 controller events; CC1/CC2 tests |
| AC-3: Inbound CC applies to bound FX param | Pass | CC74 ch1 val127 → filter_cutoff 20000Hz; precedence APVTS<inbound<plock; CC4/CC5 tests |
| AC-4: MIDI learn binds next inbound CC | Pass | arm→CC22 ch3 → binding recorded, disarmed; CC6/CC6b tests |
| AC-5: Learn map persists across save/load | Pass | round-trip preserves bindings; pre-v3 loads empty; out-of-range clamped/dropped; CC7..CC10 tests |
| AC-6: Armed learn ignores note-on; CC out additive | Pass | arm→note-on → no bind, stays armed; CC-out doesn't alter note/clock count; CC11/CC12 tests |

## Accomplishments

- `src/audio/midi_cc.h`: single range table (`k_param_range[15]`) as sole source of truth — `plock_param_norm` + `plock_param_denorm` both read it, zero drift risk; `CcOutRouting`; `CcBinding`; `CcLearnMap` (non-copyable, atomic `learn_arm`)
- CC-out wired into processBlock between `apply_plock_batch` and stop-flush/clock — additive, EXT-buffer ordering invariant preserved
- Inbound CC loop before `apply_plock_batch` establishes APVTS < inbound CC < p-lock precedence; covers all 15 PLockParams via `apply_cc_to_fx` switch
- MIDI learn atomic handshake: message-thread `arm_learn()` store-release; audio-thread load-acquire + capture + disarm; controller-only capture (note-on while armed → no bind)
- State v3: `cc_learn_` serialized as `"cc_learn"` ValueTree child; full bounds-guarding on load; backward-compatible with pre-v3 (missing child → empty map)
- 195/195 tests green (+13 new CC tests); clang-format clean

## Task Commits

Phase 9 git strategy: single commit at phase transition (09-04). All 09-03 changes are uncommitted, staged alongside 09-01/09-02.

| Task | Commit | Notes |
|------|--------|-------|
| Task 1: CC out | pending | Part of phase-transition commit after 09-04 |
| Task 2: MIDI in | pending | Part of phase-transition commit after 09-04 |
| Task 3: MIDI learn + state v3 | pending | Part of phase-transition commit after 09-04 |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/midi_cc.h` | Created | ParamRange table, norm/denorm helpers, CcOutRouting, CcBinding, CcLearnMap |
| `src/plugin_processor.h` | Modified | Added `#include "audio/midi_cc.h"`; public `cc_out_`, `cc_learn_`, `arm_learn()`; private `apply_cc_to_fx()`; k_state_version 2→3; `plock_pattern_` moved to public |
| `src/plugin_processor.cpp` | Modified | Inbound CC loop (before apply_plock_batch); CC-out emit (after apply_plock_batch, before clock); `apply_cc_to_fx` impl (switch all 15 params); state v3 serialize/restore |
| `tests/test_midi_cc.cpp` | Created | 13 test cases covering AC-1..AC-6 |
| `tests/CMakeLists.txt` | Modified | Added test_midi_cc.cpp to baque_tests target |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `plock_pattern_` made public | CC3 test needs it to stage a p-lock without duplicating logic; mirrors established lane_routing_/clock_master_ pattern | Phase 10 UI can bind directly; same single-writer contract applies |
| `CcLearnMap` non-copyable | `std::atomic<int>` member prevents copy/move; attempting to assign on state-load would not compile | State-load rebuilds in-place via `clear()` + `set_binding()` loop; never `= other` |
| CC-out offset=0 (block-granular) | `PLockEvent` carries no sample offset; sub-block accuracy would need PLockEvent extension | Deferred to future plan; documented in comment |
| Phase 10 UI contract documented in header | Mirrors pad-params/PerfState escalation pattern | UI must snapshot bindings lock-free; no raw mutable getter exposed |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 3 | Essential compile/format fixes |
| Scope additions | 0 | — |
| Deferred | 0 | All plan deferred items already in plan boundaries |

**Total impact:** Essential fixes only, no scope creep.

### Auto-fixed Issues

**1. Include path in midi_cc.h**
- **Found during:** Task 1
- **Issue:** Wrote `#include "audio/plock_pattern.h"` — midi_cc.h lives in `src/audio/`, so plock_pattern.h is a sibling
- **Fix:** Changed to `#include "plock_pattern.h"`
- **Files:** src/audio/midi_cc.h
- **Verification:** Build passed

**2. Ambiguous `noteOn()` call in test**
- **Found during:** Task 3 (writing test_midi_cc.cpp)
- **Issue:** `juce::MidiMessage::noteOn(1, 60, 100)` ambiguous between `noteOn(int,int,float)` and `noteOn(int,int,uint8)`
- **Fix:** Cast third arg to `juce::uint8{100}`
- **Files:** tests/test_midi_cc.cpp
- **Verification:** Compile clean

**3. clang-format violations**
- **Found during:** Post-task format pass
- **Issue:** Extra alignment spaces in k_param_range array, variable declarations, and function arguments
- **Fix:** `clang-format -i` on all 4 changed files
- **Files:** src/audio/midi_cc.h, src/plugin_processor.h, src/plugin_processor.cpp, tests/test_midi_cc.cpp
- **Verification:** clang-format re-run showed no diff

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| LSP false positives (juce_audio_basics not found, size_t/jassert unknown) | Build-time artifacts only — JUCE headers not in clang LSP path; real build green |

## Next Phase Readiness

**Ready:**
- CC-out: `cc_out_` (public, single-writer) configured by UI in Phase 10; TR-8 mapping templates prepared in 09-04
- MIDI-in: `cc_learn_` + `arm_learn()` API ready for UI binding in Phase 10
- State v3 backward-compatible; 09-04 may bump to v4 if needed
- All 195 tests green, processBlock ordering invariant intact

**Concerns:**
- `plock_pattern_` is now public (single-writer contract); Phase 10 must not write it from the message thread without upgrading to atomic/command-queue (same note as lane_routing_, clock_master_)
- CC-out is block-granular (offset 0); if PLockEvent ever gets a sample offset, CC-out emit needs updating

**Blockers:**
- None

---
*Built with PAUL Framework v1.4 · https://chrisai.cv/skool · https://youtube.com/@chris-ai-systems*
*Phase: 09-midi-hardware, Plan: 03*
*Completed: 2026-06-09*
