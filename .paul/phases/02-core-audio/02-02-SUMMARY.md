---
phase: 02-core-audio
plan: 02
subsystem: dsp
tags: [scheduler, midi, transport, juce, sample-accurate, catch2]

requires:
  - phase: 02-core-audio/02-01
    provides: VoicePool (trigger_at field start_offset_), APVTS, processBlock skeleton

provides:
  - Scheduler class (JUCE 8 range-for, sample-accurate samplePosition dispatch)
  - VoicePool::trigger_at(offset) method
  - TransportState struct + null-safe getPlayHead() integration
  - processBlock: scheduler replaces block-boundary MIDI loop
  - 4 sample-accurate timing tests (11/11 total ctest pass)
affects: [03-sequencer-base, all-phases]

tech-stack:
  added: []
  patterns:
    - "Scheduler: stateless class, one method process(), called from processBlock"
    - "JUCE 8 range-for on MidiBuffer: for (const juce::MidiMessageMetadata& meta : midi)"
    - "getPlayHead() null-guard: if (auto* ph = getPlayHead()) { if (auto pos = ph->getPosition()) { ... } }"
    - "trigger_at(): VoicePool method, sets start_offset_ after trigger() resets it to 0"
    - "clamped_pos: samplePosition clamped to [0, block_size) — defensive against out-of-range host values"

key-files:
  created:
    - src/audio/scheduler.h
    - src/audio/scheduler.cpp
    - tests/test_transport.cpp
  modified:
    - src/audio/voice_pool.h (trigger_at declaration)
    - src/audio/voice_pool.cpp (trigger_at implementation)
    - src/plugin_processor.h (TransportState, Scheduler, transport_ member)
    - src/plugin_processor.cpp (getPlayHead null-guard, scheduler dispatch)
    - CMakeLists.txt (scheduler.cpp source)
    - tests/CMakeLists.txt (test_transport.cpp source)

key-decisions:
  - "Scheduler is stateless: no members, no allocation — safe to construct in processor header"
  - "Note-off in Phase 2 scheduler is no-op: per-note voice tracking deferred to Phase 3"
  - "samplePosition clamped defensively: some hosts may emit out-of-range values"
  - "TransportState exposed as public struct (not nested): testable without processor instantiation"

patterns-established:
  - "JUCE 8 MidiBuffer iteration: range-for with MidiMessageMetadata (not deprecated Iterator)"
  - "getPlayHead() always null-checked before use — required for standalone/unit-test contexts"
  - "Scheduler processes entire MidiBuffer in processBlock; Phase 3 sequencer generates its own MidiBuffer"

duration: ~2h
started: 2026-06-05T02:00:00Z
completed: 2026-06-05T04:00:00Z
description: "Sample-accurate MIDI scheduler (samplePosition dispatch via trigger_at) + null-safe host transport; Phase 2 DoD fully verified: 11/11 tests pass"
type: Summary
about: "BAQUE"
---

# Phase 2 Plan 02: Scheduler + Transport Summary

**Sample-accurate MIDI scheduler (samplePosition dispatch via trigger_at) + null-safe host transport; Phase 2 DoD fully verified: 11/11 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2h |
| Started | 2026-06-05T02:00:00Z |
| Completed | 2026-06-05T04:00:00Z |
| Tasks | 3 completed |
| Files created | 3 |
| Files modified | 6 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: MIDI events dispatched at sample-accurate offset | Pass | Test "scheduler dispatches at correct sample offset": silence frames 0..255, audio at 256+ |
| AC-2: Host transport available | Pass | TransportState populated from getPlayHead() with null-guard; defaults verified by test |
| AC-3: Phase 2 DoD — fires without clicks, sample-accurate, RT-safe | Pass | All three properties verified: fade-in/out in SampleVoice, samplePosition dispatch, zero-alloc test |
| AC-4: All tests pass, pluginval green | Pass | ctest 11/11 pass; pluginval strictness 5 PASS |

## Accomplishments

- Sample-accurate dispatch: MIDI events at offset N produce silence before N and audio from N — verified by test
- JUCE 8 range-for on MidiBuffer — uses non-deprecated API throughout
- getPlayHead() null-guard: works in standalone, unit tests, and any host (including those that return nullptr)
- Phase 2 DoD fully closed: "pad fires sample without clicks; sample-accurate; RT-safe"

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Tasks 1+2+3: scheduler + transport + tests | `abf9e57` | feat(02-core-audio): sample-accurate scheduler + host transport |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/scheduler.h` | Created | Stateless Scheduler; process() dispatches via trigger_at |
| `src/audio/scheduler.cpp` | Created | JUCE 8 range-for, samplePosition clamping, trigger_at call |
| `src/audio/voice_pool.h` | Modified | trigger_at() declaration |
| `src/audio/voice_pool.cpp` | Modified | trigger_at() implementation: allocate + trigger + set start_offset_ |
| `src/plugin_processor.h` | Modified | TransportState struct, Scheduler member, transport_ member |
| `src/plugin_processor.cpp` | Modified | null-safe getPlayHead(), scheduler_.process() replaces MIDI loop |
| `CMakeLists.txt` | Modified | scheduler.cpp added to target_sources |
| `tests/test_transport.cpp` | Created | 4 tests: offset dispatch, silence before note-on, note-off no-crash, transport defaults |
| `tests/CMakeLists.txt` | Modified | test_transport.cpp source added |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| Note-off is no-op in Scheduler (Phase 2) | Per-note tracking needs sequencer note-map (Phase 3); SampleVoice::note_off() exists, wiring deferred | Phase 3 must add note tracking to map note number → active voice |
| Scheduler is stateless | No reason to store state; process() is pure transformation of MidiBuffer → VoicePool | Easy to unit-test without mocking; no lifetime concerns |
| samplePosition clamped to [0, block_size) | Defensive: some hosts emit values outside block range | Prevents out-of-bounds start_offset_ |
| TransportState public (not nested in processor) | Test file needs to reference the type; nested type would require full processor include | Phase 3 sequencer can use TransportState directly |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | Cosmetic; no scope change |
| Deferred | 1 | Expected (note-off per Phase 3) |

**Total impact:** One format fix; note-off deferral is intentional and documented.

### Auto-fixed Issues

**1. clang-format violation in voice_pool.cpp**
- **Found during:** Final verification (clang-format --dry-run)
- **Issue:** trigger_at() declaration line exceeded column 120
- **Fix:** `clang-format -i voice_pool.cpp`
- **Commit:** abf9e57 (format applied before commit)

### Deferred Items

- **Note-off per-note tracking:** Scheduler receives note-off but does nothing (Phase 2 no-op). Phase 3 (Sequencer) must add a note→voice map to connect note-off events to the correct active voice.

## Issues Encountered

None beyond the format fix.

## Next Phase Readiness

**Ready:**
- Scheduler::process() is the single integration point — Phase 3 sequencer generates a MidiBuffer with correct samplePositions, passes to same Scheduler
- TransportState.bpm and ppq_position available for BPM-synced step grid (Phase 3)
- VoicePool::trigger_at() extensible: Phase 4 can add per-pad sample data and channel routing
- Phase 2 DoD fully met and tested — Phase 3 builds on solid RT-safe foundation

**Concerns:**
- Note-off tracking: Phase 3 needs a note→voice map; leaving it unaddressed into Phase 4+ risks stuck notes
- MIDI channel ignored: all channels trigger; Phase 3 should add channel-filtering for hybrid INT/EXT mode

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 02-core-audio, Plan: 02*
*Completed: 2026-06-05*
