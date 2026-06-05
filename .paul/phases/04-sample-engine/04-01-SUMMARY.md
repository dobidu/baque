---
phase: 04-sample-engine
plan: 01
subsystem: audio
tags: [juce, padbank, varispeed, reverse, pan, sampler]

requires:
  - phase: 02-core-audio
    provides: VoicePool (64 voices), SampleVoice, Scheduler sample-accurate dispatch
  - phase: 03-sequencer-base
    provides: note 36+lane convention, two-call dispatch contract, NoteTracker
provides:
  - PadBank — 16 pads (4×4), note→pad arithmetic routing (note − 36)
  - SamplePad — per-pad sample buffer + gain/pan/pitch(semitones+cents)/reverse
  - SampleVoice fractional playback — linear interp, varispeed rate, reverse, equal-power pan
  - Pinned velocity mapping: linear (velocity/127) × pad.gain
  - Safe-load protocol: reset_all() before pad buffer mutation (UAF prevention)
affects: [04-02 choke/velocity-layers, 04-03 chop-to-pads, 04-04 offline-stretch, phase-6 p-locks, phase-10 UI pad params]

tech-stack:
  added: []
  patterns:
    - "Header-only audio data model (pad_bank.h, like transport_state.h)"
    - "Single-writer contract: pad params/buffers written only off audio thread, only with voices invalidated"
    - "Bounds-guaranteed interpolation: forward stops at num_samples−1, reverse at 0, jassert invariant"

key-files:
  created: [src/audio/pad_bank.h, tests/test_pad_bank.cpp]
  modified: [src/audio/sample_voice.h, src/audio/sample_voice.cpp, src/audio/voice_pool.h, src/audio/voice_pool.cpp, src/audio/scheduler.h, src/audio/scheduler.cpp, src/plugin_processor.h, src/plugin_processor.cpp, tests/test_voice.cpp, tests/test_transport.cpp, tests/CMakeLists.txt]

key-decisions:
  - "pad_bank.cpp omitted — header-only (all inline, transport_state.h precedent)"
  - "SampleVoice::get_position() = frames rendered (voice age) — stable steal metric under reverse/varispeed"
  - "gain_ field removed — folded into gain_left_/gain_right_ at trigger (avoids -Wunused-private-field on clang)"
  - "Equal-power center pan = 0.7071 per channel — no existing test asserted absolute levels, zero test churn"

patterns-established:
  - "Note→pad: arithmetic index (note − k_base_note) into std::array — no map, RT-safe"
  - "Per-frame math hoisted to trigger time (rate, pan gains)"

duration: ~25min
started: 2026-06-05T00:00:00Z
completed: 2026-06-05T00:00:00Z
description: "16-pad PadBank with note→pad routing, in-house varispeed (semitones+cents), reverse, equal-power pan; safe-load protocol; 31/31 tests + pluginval strictness 5"
type: Summary
about: "BAQUE"
---

# Phase 4 Plan 01: Pad Bank + Per-Pad Playback Summary

**16-pad PadBank replaces single global sample: note→pad routing (note − 36), in-house varispeed pitch (semitones + cents, linear interp), reverse, equal-power pan, pinned linear velocity — fork-free, RT-safe, 31/31 tests + pluginval strictness 5 SUCCESS.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~25min |
| Tasks | 3 completed (all DONE, qualify PASS, no GAP/DRIFT) |
| Files modified | 12 (2 created, 10 modified) |
| Tests | 31/31 (21 existing + 10 new) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Note routes to pad | Pass | Mapping test (36→0, 35/52/0→nullptr, empty→nullptr) + end-to-end amplitude test (0.25/0.5 × 0.7071 exact) + empty-note silence |
| AC-2: Varispeed pitch | Pass | Rate formula exact (±12 st, 100 cents); +12 st halves duration (±192 frames tol); rate 4.0 short-sample: early deactivation, finite output, jassert clean |
| AC-3: Reverse playback | Pass | Ramp mirrored (strictly descending frames 40–60); reverse + trigger_at offset N silent for N frames |
| AC-4: RT-safety preserved | Pass | Zero-alloc processBlock test green with PadBank; 21 existing tests pass (3 transport tests migrated to new signature, no behavior change) |
| AC-5: Safe sample load (audit) | Pass | reset_all → setSize realloc → process: silence, no use-after-free |

## Verification Results

- `cmake --build build -j24` — clean (only pre-existing Catch2 `-Wfloat-equal` noise in test_transport.cpp:63)
- `ctest --test-dir build` — **31/31 passed**
- pluginval v1.0.4 `--strictness-level 5 --validate-in-process` on VST3 — **SUCCESS**
- `clang-format --dry-run --Werror` on all src + tests — clean

## Accomplishments

- Pad data model established for the whole Phase 4 chain: 04-02 (choke/layers) attaches to SamplePad, 04-03 fills pads with slices, 04-04 renders stretch into pad buffers
- Varispeed shipped without SoundTouch fork — slice-around-fork strategy validated (ESCOPO §4.11 realtime path is a rate change)
- Both audit must-haves implemented and test-proven (UAF protocol, OOB bounds)
- Phase 2 behavior preserved: test_kick.wav → pad 0 → note 36 fires unchanged

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/pad_bank.h` | Created | SamplePad + PadBank, single-writer contract docs, header-only |
| `tests/test_pad_bank.cpp` | Created | 5 tests: mapping, rate formula, routing, velocity, safe-load |
| `src/audio/sample_voice.h/.cpp` | Modified | double position_, linear interp, rate/reverse/pan, bounds termination + assert, frames_rendered_ age |
| `src/audio/voice_pool.h/.cpp` | Modified | trigger_at forwards rate/reverse/pan (defaulted — old call sites compile) |
| `src/audio/scheduler.h/.cpp` | Modified | PadBank routing, pinned velocity (v/127 × pad.gain), silent skip for unmapped notes |
| `src/plugin_processor.h/.cpp` | Modified | pad_bank_ replaces sample_buffer_; safe-load in prepareToPlay; WAV → pad 0 |
| `tests/test_voice.cpp` | Modified | +5 tests: half-duration, rate-4 bounds, reverse ramp, reverse+offset, hard-left pan |
| `tests/test_transport.cpp` | Modified | Migrated 3 scheduler tests to PadBank signature (note 60 → 36) |
| `tests/CMakeLists.txt` | Modified | Added test_pad_bank.cpp |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| pad_bank.cpp omitted (header-only) | All methods inline; transport_state.h precedent | No CMake source-list change needed |
| get_position() = frames rendered | Source position fractional + decreasing in reverse; rendered frames = stable voice-age metric | Steal logic correct for any rate/direction |
| gain_ member removed | Written-never-read after pan folding → clang -Wunused-private-field on macOS CI | Pan gains computed once at trigger |
| Center pan keeps equal-power (−3 dB vs old mono-to-both) | No existing test asserted absolute level; deferred level policy to mixer phase per audit | Zero test churn; documented |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 2 | Essential consistency fixes, no scope creep |
| Plan-doc inaccuracies | 2 | Frontmatter said src/CMakeLists.txt (doesn't exist; root CMakeLists, no change needed) and pad_bank.cpp (not needed) |
| Scope additions | 1 | test_transport.cpp migration (forced by Scheduler signature change; absent from files_modified) |
| Deferred | 0 | — |

**Total impact:** Minimal — all deviations mechanical, none architectural.

### Auto-fixed Issues

**1. [consistency] test_transport.cpp used note 60 — outside pad range**
- **Found during:** Task 3 (Scheduler routing)
- **Issue:** Old tests triggered note 60 with raw sample ptr; under PadBank, note 60 maps to nothing
- **Fix:** Migrated to PadBank + note 36 (k_base_note); added load_pad helper
- **Verification:** All 3 transport tests pass unchanged in intent

**2. [warnings] SampleVoice::gain_ written-never-read**
- **Found during:** Task 2 (voice extension)
- **Issue:** Pan folding left gain_ unused → -Wunused-private-field under clang
- **Fix:** Field removed
- **Verification:** Clean build

## Issues Encountered

None — no NEEDS_CONTEXT, no BLOCKED, no qualify loops needed.

## Next Phase Readiness

**Ready:**
- SamplePad struct ready to receive choke_group, velocity layers, ADSR, play modes (04-02)
- PadBank::pad(i) load path ready for chop-to-pads slice distribution (04-03)
- Mono buffer convention + background-load contract documented for offline stretch (04-04)

**Concerns:**
- Pad params rely on documented (not enforced) single-writer discipline — must upgrade to atomics/command-queue before UI phase (audit-flagged)
- Scheduler note-off still pool-global (Phase 2 carry-over); 04-02 choke work is the natural place for per-note voice mapping

**Blockers:**
- 04-04 still blocked on R&D-TS fork repo bootstrap (not started — should run parallel to 04-02/04-03)

---
*Built with PAUL Framework v1.4 · https://chrisai.cv/skool · https://youtube.com/@chris-ai-systems*
*Phase: 04-sample-engine, Plan: 01*
*Completed: 2026-06-05*
