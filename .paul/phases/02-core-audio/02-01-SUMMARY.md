---
phase: 02-core-audio
plan: 01
subsystem: dsp
tags: [juce, apvts, voice-pool, midi, rt-safety, catch2, wav]

requires:
  - phase: 01-setup
    provides: CMake + JUCE 8.0.13, BaqueProcessor skeleton, Catch2 test infrastructure

provides:
  - SampleVoice class (fade-in/out, start_offset_ for 02-02 compat)
  - VoicePool (64-voice std::array, mandatory steal, RT-safe)
  - BaqueProcessor with APVTS master_gain (SmoothedValue), WAV decode, MIDI trigger
  - BaqueAssets BinaryData (440Hz test WAV via juce_add_binary_data)
  - 5 DSP unit tests incl. RT-safety (zero allocs in processBlock)
affects: [02-02, 03-sequencer-base, all-phases]

tech-stack:
  added: [juce_audio_formats (WavAudioFormat), juce::SmoothedValue, juce::AudioProcessorValueTreeState]
  patterns:
    - "VoicePool: pre-allocated std::array<SampleVoice, 64>; allocate() never returns nullptr (mandatory steal)"
    - "WAV decode once in prepareToPlay with sample_loaded_ guard (prevents realloc + dangling ptr)"
    - "Mix buffers (mix_left_, mix_right_) pre-allocated in prepareToPlay; processBlock is zero-alloc"
    - "processBlock RT-safety verified by thread_local operator new override in tests"
    - "start_offset_ in SampleVoice (set to 0 by trigger()): forward-compatible with 02-02 sample-accurate dispatch"

key-files:
  created:
    - src/audio/sample_voice.h
    - src/audio/sample_voice.cpp
    - src/audio/voice_pool.h
    - src/audio/voice_pool.cpp
    - assets/test_kick.wav
    - tests/test_voice.cpp
  modified:
    - CMakeLists.txt (BaqueAssets, juce_audio_formats, audio/ sources, tests link)
    - src/plugin_processor.h (APVTS, VoicePool, SmoothedValue, sample_buffer_)
    - src/plugin_processor.cpp (prepareToPlay WAV decode, processBlock MIDI dispatch)
    - tests/CMakeLists.txt (test_voice.cpp, BaqueAssets link)

key-decisions:
  - "juce_audio_formats needed explicitly: WavAudioFormat not in juce_audio_utils"
  - "440Hz sine WAV committed as test_kick.wav: simpler than kick synthesis, functionally equivalent"
  - "mix_left_/mix_right_ as std::vector pre-allocated in prepareToPlay: no stack VLA, clean resize"
  - "APVTS apvts_ declared public: required for editor access in future phases"

patterns-established:
  - "All new DSP files in src/audio/ subdirectory"
  - "RT-safety test pattern: thread_local g_count_allocs flag + operator new override in test file"
  - "ESCOPO §8: snake_case English IDs, Portuguese comments, _ suffix privates — confirmed in all audio/ files"

duration: ~4h
started: 2026-06-05T00:00:00Z
completed: 2026-06-05T02:00:00Z
description: "APVTS master_gain + 64-voice pre-allocated VoicePool + WAV decode + MIDI note-on trigger; RT-safety verified by zero-alloc test (7/7 pass)"
type: Summary
about: "BAQUE"
---

# Phase 2 Plan 01: Voice Pool + APVTS Summary

**APVTS master_gain + 64-voice pre-allocated VoicePool + WAV decode + MIDI note-on trigger; RT-safety verified by zero-alloc test (7/7 pass).**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~4h |
| Started | 2026-06-05T00:00:00Z |
| Completed | 2026-06-05T02:00:00Z |
| Tasks | 3 completed |
| Files created | 6 |
| Files modified | 4 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: MIDI note-on triggers audible sample | Pass | Note-on dispatches voice via VoicePool; WAV decoded once; fade-in prevents click |
| AC-2: RT-safe — zero allocations on audio thread | Pass | Test #7 "processBlock makes zero allocations": PASSED (operator new override) |
| AC-3: APVTS gain parameter controls output level | Pass | master_gain SmoothedValue applied per-sample in processBlock; 10ms ramp |
| AC-4: Smoke tests pass | Pass | ctest 7/7: all existing + new voice tests pass |

## Accomplishments

- Pre-allocated 64-voice pool with mandatory steal — never drops notes silently
- processBlock is zero-allocation: verified by instrumented test with operator new override
- APVTS wired with SmoothedValue gain — automation-ready, zipper-free
- WAV decoded once at startup via JUCE WavAudioFormat, guarded against double-decode
- start_offset_ in SampleVoice prepares exact 02-02 sample-accurate dispatch at zero cost in 02-01

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Tasks 1+2+3: voice pool + APVTS + tests | `12ffa6f` | feat(02-core-audio): APVTS + pre-allocated voice pool + MIDI sample playback |
| Format fix | `c1f4d48` | style: clang-format processor files after Phase 2 changes |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/sample_voice.h` | Created | SampleVoice: data ptr, fade-in/out, start_offset_, trigger/process/note_off |
| `src/audio/sample_voice.cpp` | Created | fade-in first 32 frames, fade-out last 32 frames, note_off graceful stop |
| `src/audio/voice_pool.h` | Created | VoicePool: k_pool_size=64, allocate/process_all/reset_all |
| `src/audio/voice_pool.cpp` | Created | allocate() mandatory steal (highest position), process_all zero-inits then mixes |
| `assets/test_kick.wav` | Created | 440Hz 500ms 44.1kHz mono 16-bit WAV test sample |
| `tests/test_voice.cpp` | Created | 5 tests: silence, playback, steal, fade-in, zero-alloc RT-safety |
| `CMakeLists.txt` | Modified | BaqueAssets, juce_audio_formats link, src/audio/ sources |
| `src/plugin_processor.h` | Modified | APVTS, VoicePool, SmoothedValue, sample_buffer_, mix vectors |
| `src/plugin_processor.cpp` | Modified | prepareToPlay WAV decode + guard, processBlock MIDI dispatch, gain smoother |
| `tests/CMakeLists.txt` | Modified | test_voice.cpp source, BaqueAssets link |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| juce_audio_formats linked explicitly | WavAudioFormat not in juce_audio_utils; plan had implicit assumption | All future WAV/audio format use needs this link |
| 440Hz sine as test asset | Kick synthesis added complexity; sine is functionally equivalent for DSP testing | Factory library with real samples deferred to Phase 11 |
| std::vector for mix buffers | std::array needs compile-time size; block size varies; pre-alloc in prepareToPlay is correct | Consistent with JUCE convention |
| apvts_ public member | Editor and future components need access to parameter tree | Follow-on phases can bind UI to APVTS directly |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 3 | Essential; no scope change |
| Deferred | 0 | — |

**Total impact:** Three auto-fixes; no scope creep.

### Auto-fixed Issues

**1. Missing juce_audio_formats module link**
- **Found during:** Task 2 qualify (build error)
- **Issue:** `juce::WavAudioFormat` not available; `juce_audio_formats` not linked
- **Fix:** Added `juce::juce_audio_formats` to target_link_libraries + `#include <juce_audio_formats/juce_audio_formats.h>`
- **Files:** CMakeLists.txt, plugin_processor.cpp
- **Verification:** Build passes

**2. clang-format violations in new files (round 1)**
- **Found during:** Task 3 qualify
- **Fix:** `clang-format -i src/audio/*.cpp src/audio/*.h tests/test_voice.cpp`
- **Commit:** 12ffa6f (included)

**3. clang-format violations in plugin_processor.cpp (round 2)**
- **Found during:** Final verification
- **Fix:** `clang-format -i src/plugin_processor.cpp src/plugin_processor.h`
- **Commit:** c1f4d48

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| mix buffers: pre-allocate or stack VLA? | std::vector pre-allocated in prepareToPlay — safe + JUCE convention |
| Note-off not yet connected to voice fade-out | SampleVoice::note_off() implemented and tested; 02-02 connects per-note tracking |

## Next Phase Readiness

**Ready:**
- VoicePool::trigger_at(offset) slot available for 02-02 sample-accurate dispatch
- start_offset_ field already in SampleVoice (trigger() sets 0; trigger_at() will set N)
- APVTS structure extensible: adding parameters is additive
- processBlock RT-safety confirmed — any further DSP added to this path must maintain zero-alloc invariant

**Concerns:**
- Note-off not yet per-note (any note-off is ignored until 02-02 wires scheduler)
- mix_left_/mix_right_ vectors: resize in prepareToPlay is an allocation — acceptable (not audio thread), but must not be called from processBlock

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 02-core-audio, Plan: 01*
*Completed: 2026-06-05*
