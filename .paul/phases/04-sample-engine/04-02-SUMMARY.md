---
phase: 04-sample-engine
plan: "02"
subsystem: audio-engine
tags: [choke, velocity-layers, round-robin, adsr, play-modes, sample-voice, juce]

requires:
  - phase: 04-01
    provides: SamplePad buffer, SampleVoice (varispeed/reverse/pan), VoicePool, Scheduler stateless routing

provides:
  - Choke groups (per-voice force-fade on pad group trigger)
  - Velocity layers with round-robin cycling (up to 8 layers per pad)
  - ADSR envelope state machine per voice (attack/decay/sustain/release)
  - PlayMode: one_shot / gate / loop
  - Scheduler stateful (PadRuntime: rr_index, last_voice), prepare() for sample_rate
  - Per-note note-off routing with stolen-voice guard
  - VoicePool::prepare() forwards sample_rate to voices for ms→frames conversion

affects: [04-03, 04-04, 05-feel-engine, 06-per-pad-fx]

tech-stack:
  added: []
  patterns:
    - "choke() vs note_off(): external cut (choke) bypasses play_mode; internal MIDI release (note_off) respects it"
    - "ADSR ms→frames computed once at trigger time, not per-frame"
    - "round-robin bounded two-pass (count matching layers, then pick by index) — RT-safe, no alloc"
    - "frames_rendered_ never reset on loop wrap (steal metric must be monotonically increasing)"
    - "sustain=0 gate guard: env_level_<=0 in note_off() immediately deactivates (prevents zero-rate release leak)"

key-files:
  created: []
  modified:
    - src/audio/pad_bank.h
    - src/audio/sample_voice.h
    - src/audio/sample_voice.cpp
    - src/audio/voice_pool.h
    - src/audio/voice_pool.cpp
    - src/audio/scheduler.h
    - src/audio/scheduler.cpp
    - src/plugin_processor.cpp
    - tests/test_pad_bank.cpp
    - tests/test_voice.cpp

key-decisions:
  - "choke() separate from note_off(): plan used note_off() but one_shot ignores it; choke is external force"
  - "VelocityLayer fields (layers_, num_layers_) private; accessors public"
  - "PlayMode/AdsrParams added to pad_bank.h (not sample_voice.h) — pad owns playback params"
  - "VoicePool::prepare() stores sample_rate_; Scheduler::prepare() resets runtime_ + stores sample_rate_"
  - "scheduler_.prepare() + voice_pool_.prepare() both called from prepareToPlay()"

patterns-established:
  - "choke() = forced 32-frame fade regardless of play_mode (use for group triggers)"
  - "note_off() = ADSR release for gate/loop; ignored for one_shot (use for MIDI events)"
  - "Default ADSR {0,0,1.0,0} + one_shot = backwards-compatible with 04-01 behavior"

duration: ~90min
started: 2026-06-05T00:00:00Z
completed: 2026-06-05T00:00:00Z
description: "Choke groups, 8-layer velocity RR, ADSR state machine, one_shot/gate/loop play modes"
type: Summary
about: "BAQUE"
---

# Phase 4 Plan 02: Choke + Velocity Layers + ADSR + Play Modes — Summary

**Choke groups, 8-layer velocity round-robin, ADSR envelope state machine, and one_shot/gate/loop play modes — all RT-safe, 44/44 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~90 min |
| Tasks | 3 completed |
| Files modified | 10 |
| Tests added | 13 (3 choke + 4 layers/RR + 6 ADSR/modes) |
| Total tests | 44 (all pass) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Choke groups silence same-group pads | Pass | Tested: cross-pad, different-group isolation, scheduler self-retrigger |
| AC-2: Velocity layers + round-robin | Pass | Tested: match, no-match fallback, 2-layer RR alternation, num_layers=0 compat |
| AC-3: ADSR envelope + play modes | Pass | Tested: one_shot ignores note_off, gate release, loop wrap, attack ramp, decay to sustain, sustain=0 guard |

## Accomplishments

- Choke groups: `VoicePool::choke_group()` iterates pool, calls `choke()` on matching-group active voices; Scheduler dispatches before `trigger_at()`
- Velocity layers: `VelocityLayer` struct + 8-slot array per pad; two-pass RT-safe selection (count matching → pick by rr_index%count); round-robin counter in `PadRuntime`
- ADSR: `EnvState` state machine (idle→attack→decay→sustain_hold→release); ms→frames computed once at trigger; `dec_frames_` cached for attack→decay transition
- PlayMode: `one_shot` ignores `note_off()`; `gate`/`loop` enter ADSR release; loop wraps position without resetting `frames_rendered_`
- Scheduler stateful: `PadRuntime{rr_index, last_voice}` per pad; `prepare()` resets all state + caches sample_rate

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/pad_bank.h` | Modified | Added `VelocityLayer`, `PlayMode`, `AdsrParams`, `choke_group`, `adsr`, `play_mode`, layer accessors |
| `src/audio/sample_voice.h` | Modified | Added `pad_index` param, `choke()`, `note_off()` updated, ADSR fields (`EnvState`, env_level_, etc.) |
| `src/audio/sample_voice.cpp` | Modified | Updated `trigger()` (ADSR init), `note_off()` (play_mode + sustain=0 guard), `process()` (ADSR state machine, loop wrap, no-goto) |
| `src/audio/voice_pool.h` | Modified | `trigger_at()` returns `SampleVoice*`, `choke_group()` decl, `prepare()`, `sample_rate_` field |
| `src/audio/voice_pool.cpp` | Modified | `trigger_at()` passes pad_index/adsr/play_mode; `choke_group()` uses `choke()`; `prepare()` stores sample_rate_ |
| `src/audio/scheduler.h` | Modified | `PadRuntime` struct, `runtime_` array, `prepare()` decl, `sample_rate_` field, updated class comment |
| `src/audio/scheduler.cpp` | Modified | `prepare()` impl (runtime_.fill + store sr); note-on: choke dispatch + layer RR selection + pass adsr/play_mode; note-off: pad_index guard |
| `src/plugin_processor.cpp` | Modified | Added `voice_pool_.prepare()` + `scheduler_.prepare()` calls in `prepareToPlay()` |
| `tests/test_pad_bank.cpp` | Modified | +3 choke tests, +4 velocity-layer/RR tests (tests 6–12) |
| `tests/test_voice.cpp` | Modified | +6 ADSR/play-mode tests (F1–F6: one_shot, gate, loop, attack, decay, sustain=0) |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `choke()` method separate from `note_off()` | Plan used `note_off()` but one_shot ignores it; choke is an external group-cut, not a MIDI event | All choke paths use `choke()`; MIDI note-off uses `note_off()` |
| `VelocityLayer` fields private, accessors public | Consistent with SamplePad::buffer_ pattern; single-writer contract enforced via accessors | Layer setup off-audio-thread only |
| `PlayMode`/`AdsrParams` in `pad_bank.h` | Pad owns all playback parameters; voice reads them at trigger time | No breaking change to SampleVoice ABI for callers |
| `scheduler_.prepare()` + `voice_pool_.prepare()` both in `prepareToPlay()` | Scheduler needs sample_rate for ADSR; pool needs it too; clear ownership | Both must be called; documented in each .h |
| Legacy `fade_out_remaining_` kept alongside ADSR | Backward compat for choke (force-fade) and any other forced-stop callers | Two fade paths; ADSR handles gate/loop release, fade_out handles choke |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | Essential correctness fix |
| Scope additions | 0 | — |
| Deferred | 0 | — |

**Total impact:** One essential fix (choke semantic), no scope creep.

### Auto-fixed Issues

**1. `choke()` vs `note_off()` — plan used wrong method for choke dispatch**
- **Found during:** Task 3 integration (choke test failures after PlayMode added)
- **Issue:** Plan's `VoicePool::choke_group()` called `v.note_off()`, which returns early for `PlayMode::one_shot`. Since default pad play_mode is one_shot, choke had no effect.
- **Fix:** Added `SampleVoice::choke()` method — unconditionally sets `fade_out_remaining_ = k_fade_frames` regardless of play_mode. `choke_group()` calls `choke()` instead of `note_off()`.
- **Files:** `src/audio/sample_voice.h`, `src/audio/voice_pool.cpp`
- **Verification:** Tests 25 + 26 (choke group silences, self-retrigger) pass after fix.

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| `uint8_t`/`PadBank` undefined in `voice_pool.h` | Added `#include <cstdint>` + forward decl `class PadBank;`; full include in `.cpp` |
| `layers_`/`num_layers_` accidentally public in first edit | Moved to `private:` section; accessors kept public |
| Choke tests failed after Task 3 | `choke()` method added (see Deviations above) |

## Next Phase Readiness

**Ready:**
- `SamplePad` fully extended: choke_group, VelocityLayer array, AdsrParams, PlayMode — 04-03 (auto-slice) loads via `sample_buffer()` unchanged
- `Scheduler::prepare()` wired in `prepareToPlay()` — sample_rate always valid before first block
- Single-writer contract documented and enforced by API shape; no live-edit races in current phase

**Concerns:**
- `fade_out_remaining_` legacy path coexists with ADSR — two fade mechanisms could interact if both set simultaneously (currently impossible by construction, but worth documenting for Phase 6 FX)
- ADSR default `{0,0,1.0,0}` + `one_shot` = identical to 04-01 behavior (verified by backward-compat tests)

**Blockers:**
- 04-04 (time-stretch) blocked on R&D-TS fork repo bootstrap — unrelated to 04-03

---
*Built with PAUL Framework v1.4*
*Phase: 04-sample-engine, Plan: 02*
*Completed: 2026-06-05*
