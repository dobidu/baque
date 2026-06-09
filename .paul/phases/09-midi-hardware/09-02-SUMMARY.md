---
phase: 09-midi-hardware
plan: 02
type: Summary
status: complete
completed: 2026-06-08
tests_before: 173
tests_after: 182
about: "BAQUE"
---

# 09-02 SUMMARY — MIDI Clock OUT Master

## What Was Built

**MidiClock class** (`src/audio/midi_clock.h/.cpp`):
- 24-ppqn timing clock (0xF8) generated from host transport (bpm/ppq_position)
- start (0xFA) / continue (0xFB) / stop (0xFC) on transport edges
- clock_master=false → absolute silence (no MIDI pollution)
- Monotonic tick guard (audit M1): `int64_t last_tick_` — emits only `n > last_tick_`; PPQ-regression resync on DAW loop (mirrors Sequencer Phase 5 guard)

**processBlock wiring** (`plugin_processor.h/.cpp`):
- `bool clock_master_ = false` — public member (like lane_routing_); UI/automation future
- `MidiClock midi_clock_` — private; `prepare()` in prepareToPlay, `reset()` in releaseResources
- Order: notas EXT (generate) → stop-flush → **clock** → midi_messages.clear()+addEvents(ext)

**CMakeLists.txt**: `src/audio/midi_clock.cpp` added to BAQUE sources.

## Acceptance Criteria Results

| AC | Test | Result |
|----|------|--------|
| AC-1 (24 ppqn) | MC1 | ✅ 24 × 0xF8 per quarter note at 120bpm/48k |
| AC-2 (start/stop/continue) | MC3/MC4/MC5 | ✅ Start at ppq~0; Continue at ppq>0; Stop on edge |
| AC-3 (off=silent) | MC6 | ✅ master=false → buf.isEmpty() |
| AC-4 (sample-accurate) | MC2 | ✅ Ticks at 1000-sample intervals (±2), all in [0, block) |
| AC-5 (wiring) | MC7 | ✅ processBlock emits 24 clocks; audio finite |
| AC-6 (monotonic M1) | MC8/MC9 | ✅ Two-block sum=24 exact; PPQ regression resyncs without history re-emit |

## Deviations from Plan

- **MC8 tick12_count check removed**: planned extra check for "tick 12 appears at pos 0 in block 2" was flawed — buf accumulates both blocks so tick 0 and tick 12 both hit pos 0. The `total==24` assertion is the direct proof of AC-6; extra check was redundant and broken. Removed; semantics unchanged.

## Decisions Made

- `was_playing_ = false` when master=false (not `is_playing && master`) — avoids spurious start edge when master is toggled on during playback. Consistent with plan invariants.
- PPQ regression threshold: `ppq_start < implied_ppq - (1.0/k_ppqn)` — one full tick window of hysteresis to avoid false regression trigger on minor float drift.

## Deferred (from audit)

| Item | Rationale |
|------|-----------|
| Slave/PLL (follow external clock) | Needs jitter filter; separate plan post-v1 |
| Song Position Pointer (SPP) | start/continue/stop sufficient; add if needed |
| Real TR-8 jitter measurement | Manual hardware verify in 09-04 DoD |
| sub-block transport-edge position | Emitted at offset 0 in v1 (SR2); `timeInSamples` refinement later |

## Files Created/Modified

- `src/audio/midi_clock.h` — NEW
- `src/audio/midi_clock.cpp` — NEW
- `src/plugin_processor.h` — clock_master_ public + MidiClock midi_clock_ private + #include
- `src/plugin_processor.cpp` — midi_clock_.prepare/reset + processBlock clock call
- `CMakeLists.txt` — midi_clock.cpp in BAQUE sources
- `tests/test_midi_clock.cpp` — NEW (MC1-MC9, 9 tests)
- `tests/CMakeLists.txt` — test_midi_clock.cpp added

## Test Count

173 → 182 (+9 MC tests, 0 regressions)

## Next

09-03: CC out (p-locks→CC) + MIDI in + MIDI learn
