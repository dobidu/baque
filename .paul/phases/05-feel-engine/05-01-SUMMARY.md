---
phase: 05-feel-engine
plan: 01
subsystem: feel-engine
tags: [feel, micro-timing, sequencer, midi, deferred-queue, rt-safety]

requires:
  - phase: 04-sample-engine
    provides: PadBank + VoicePool + Scheduler + Sequencer baseline (Phase 3/4 call chain)

provides:
  - FeelPattern POD struct (per-step timing_ms + vel_scale, enabled flag)
  - FeelEngine (pre-allocated deferred note queue, flush/defer/ppq-regression-clear)
  - Sequencer::generate() extended with nullable feel params (backward-compat)
  - Sequencer::set_pattern() immediate setter (bypasses 15→0 boundary requirement)
  - PluginProcessor wired with FeelEngine + FeelPattern + block_start_sample_

affects: [05-02-humanize, 05-03-presets, phase-6-fx-plocks, phase-10-ui]

tech-stack:
  added: []
  patterns:
    - "Deferred note queue: pre-allocated fixed array; flush-before-generate ordering"
    - "ppq regression guard: track last_ppq_ in Sequencer; clear queue on backward jump"
    - "RT-safety jassert: defer() asserts getRawDataSize() <= 3 (inline MIDI only)"
    - "set_pattern() for immediate set; set_next_pattern() for live glitch-free switching"

key-files:
  created:
    - src/audio/feel_pattern.h
    - src/audio/feel_engine.h
    - src/audio/feel_engine.cpp
    - tests/test_feel_engine.cpp
  modified:
    - src/audio/sequencer.h
    - src/audio/sequencer.cpp
    - src/plugin_processor.h
    - src/plugin_processor.cpp
    - CMakeLists.txt
    - tests/CMakeLists.txt

key-decisions:
  - "flush_deferred called BEFORE note generation — deferred notes land in correct order relative to new notes"
  - "Negative offset clamped to 0 (no before-block deferral in v1) — imperceptible for typical feel offsets < 50ms"
  - "base velocity hardcoded 100 until p-locks (Phase 6); vel_scale applies multiplicatively"
  - "FeelPattern thread safety deferred to Phase 10 UI — set from message thread when stopped"

patterns-established:
  - "set_pattern() for tests/initial setup; set_next_pattern() for live switching — same Sequencer, different paths"
  - "block_start_sample_ in PluginProcessor: reset prepareToPlay, increment processBlock — absolute sample counter"

duration: ~45min
started: 2026-06-05T23:45:00Z
completed: 2026-06-05T23:59:00Z
description: "FeelPattern + FeelEngine deferred queue + Sequencer per-step timing offset — Feel Engine v1 infrastructure"
type: Summary
about: "BAQUE"
---

# Phase 5 Plan 01: FeelPattern + FeelEngine + Sequencer Integration Summary

**Per-step timing offset + velocity scaling wired into Sequencer; pre-allocated deferred queue handles notes offset beyond block boundary; all 67 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~45 min |
| Started | 2026-06-05T23:45:00Z |
| Completed | 2026-06-05T23:59:00Z |
| Tasks | 3 completed |
| Files modified | 10 (4 created, 6 modified) |
| Commit | `20952ff` |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Positive offset shifts note forward | Pass | +25ms at 44100Hz = 1103 samples ±2 verified (FE1) |
| AC-2: Negative offset clamps to 0 | Pass | step at ppq=0 with -50ms → position 0 (FE2) |
| AC-3: Beyond-block offset defers note | Pass | +150ms at 4096 block → deferred_count=1, note absent in current block (FE3) |
| AC-4: Deferred note fires in correct block | Pass | target=10000 fires at pos 1808 in block [8192,12288) (FE4) |
| AC-5: Feel disabled → unchanged output | Pass | positions identical to no-feel baseline (FE5) |
| AC-6: vel_scale applied + clamped | Pass | 0.5→50, 2.0→127, 0.0→1 (FE6) |
| AC-7: 16 steps with distinct offsets correct | Pass | 16 note-ons, each ±3 samples of expected position (FE7) |
| AC-8: Queue overflow drops note RT-safely | Pass | count stays at k_max_deferred=128, no exception (FE8) |

## Accomplishments

- Per-step timing offset wired end-to-end: FeelPattern → Sequencer::generate() → MidiBuffer samplePosition
- Pre-allocated deferred note queue (128 slots) handles late notes across block boundaries with O(n) compaction
- ppq regression detection (DAW loop/restart) automatically clears stale deferred notes — no ghost notes on loop
- Backward-compatible API: all 59 prior tests unaffected (generate() default params = pre-Phase-5 behavior)
- `set_pattern()` immediate setter enables clean test setup without bar-boundary ceremony

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| All 3 tasks | `20952ff` | Full 05-01 implementation (single commit, 14 files, +1253 -19) |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/feel_pattern.h` | Created | FeelStep + FeelPattern POD structs (16 steps, timing_ms + vel_scale, enabled) |
| `src/audio/feel_engine.h` | Created | FeelEngine class: deferred queue, flush/defer, timing_ms_to_samples |
| `src/audio/feel_engine.cpp` | Created | flush_deferred (fire + discard stale + keep future), defer (RT-safe drop) |
| `src/audio/sequencer.h` | Modified | New generate() signature (feel params), set_pattern(), last_ppq_ member |
| `src/audio/sequencer.cpp` | Modified | set_pattern() impl, ppq regression guard, flush_deferred call, add_with_feel lambda, vel_scale |
| `src/plugin_processor.h` | Modified | feel_engine_ + feel_pattern_ + block_start_sample_ members |
| `src/plugin_processor.cpp` | Modified | feel_engine_.prepare() + block_start_sample_=0 in prepareToPlay; wired generate() |
| `tests/test_feel_engine.cpp` | Created | 8 tests FE1–FE8 |
| `CMakeLists.txt` | Modified | Added feel_engine.cpp to BAQUE target_sources |
| `tests/CMakeLists.txt` | Modified | Added test_feel_engine.cpp to baque_tests executable |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| flush_deferred BEFORE step loop | Deferred notes from prev block land in order before new block's notes | Correct temporal ordering in MidiBuffer |
| Negative offset clamps to 0 (v1) | Steps firing near block start (ppq=0) would need pre-block deferral; adds complexity for imperceptible case | 05-02 humanize will keep offsets small; document limitation |
| last_ppq_ tracks ppq_at_block_START (not end) | Simpler; regression detection fires one block late at worst | Acceptable; ghost notes cleared before they'd cause audible issues |
| base velocity = 100 hardcoded | No per-step velocity in StepPattern yet; vel_scale applies to this constant | Phase 6 p-locks will provide per-step base velocity; vel_scale will multiply against it |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Audit additions (must-have) | 4 | Essential fixes applied before APPLY |
| Audit additions (strong-rec) | 2 | Test quality improvements |
| No-change from original | 0 deferred | All audit items resolved |

**Total impact:** Essential RT-safety + correctness fixes; no scope creep.

### Audit-Applied Changes (applied pre-APPLY, not discovered during execution)

**M1: set_pattern() added to Sequencer**
- Issue: set_next_pattern() requires 15→0 bar boundary; all test cases using set_next_pattern() + immediate generate() produced zero notes
- Fix: Added Sequencer::set_pattern() for immediate (non-queued) pattern set; tests updated

**M2: flush_deferred stale-note discard**
- Issue: `else` branch in flush loop re-queued past notes forever → queue leak → eventual overflow
- Fix: Split else into `target >= block_end` (keep) vs `target < block_start` (jassertfalse + discard)

**M3: ppq regression guard**
- Issue: DAW loop restart doesn't clear deferred queue; stale notes from prev iteration fire in new iteration
- Fix: Track `last_ppq_` in Sequencer; call `feel_engine->prepare()` on backward ppq jump

**M4: defer() RT-allocation guard**
- Issue: `juce::MidiMessage` copy allocates for messages > 7 bytes; no guard prevented passing sysex etc.
- Fix: `jassert(msg.getRawDataSize() <= 3)` in defer()

### SR Improvements

**SR1: FE7 position verification**
- Was: `REQUIRE(positions.size() >= 16)` — count-only
- Now: verifies each step fires at `round(i * 5512.5) + timing_offset ± 3 samples`

**SR2: FE3 deferred content verification**
- Was: checked deferred_count >= 1 only
- Now: also calls flush_deferred on simulated next block and verifies note-on #36 fires

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| None | Plan + audit covered all issues pre-APPLY |

## Next Phase Readiness

**Ready:**
- FeelPattern + FeelEngine infrastructure established; 05-02 adds humanize_timing_ms + humanize_vel_pct + seed PRNG on top without changing core offset/defer mechanism
- `FeelEngine::timing_ms_to_samples()` is pure static — usable from humanize path directly
- `FeelPattern` has `// Phase 05-02 will add: float humanize_timing_ms, humanize_vel_pct, uint32_t seed` placeholder

**Concerns:**
- FeelPattern not thread-safe (single-writer by convention; message thread only when stopped). Must upgrade in Phase 10 (UI)
- Base velocity hardcoded 100 — Phase 6 p-locks will need to supply per-step velocity; vel_scale multiply chain must be preserved

**Blockers:** None — 05-02 can start immediately.

---
*Built with PAUL Framework v1.4*
*Phase: 05-feel-engine, Plan: 01*
*Completed: 2026-06-05*
