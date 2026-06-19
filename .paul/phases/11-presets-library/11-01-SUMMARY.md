---
phase: 11-presets-library
plan: 01
subsystem: audio
tags: [state-serialization, preset-manager, juce, valuetree, round-trip]

requires:
  - phase: 10-ui-ux
    provides: BrowserScreen v1, load_sample_from_file with stereo downmix, getStateInformation v4 (APVTS + routing + cc_out + cc_learn)

provides:
  - getStateInformation v5 (adds StepPattern, FeelPattern, PLockPattern, SamplePad params + source file paths)
  - setStateInformation v5 with backward-compat: v4 blobs silently default missing subtrees
  - SamplePad.source_file_ — stored on load, restored on setStateInformation
  - PresetManager class: save/save_to_file/load/list_user_presets; .bqpreset format with BQP1 magic header
  - set_feel_pattern() on BaqueProcessor for state-load and test use
  - P11D1–P11D5 round-trip tests (249/249)

affects: [11-02-presets-browser, 12-hardening, Phase 11 DoD]

tech-stack:
  added: [PresetManager (src/preset_manager.h, src/preset_manager.cpp)]
  patterns:
    - "State v5 subtrees: pattern_v5 / feel_v5 / plock_v5 / pads_v5 appended after existing v4 subtrees"
    - "Backward-compat via invalid-ValueTree guard: getChildWithName() returns invalid → if(valid) block skips"
    - "BQP1 magic header: 4-byte magic + 4-byte meta_len + metadata XML + binary ValueTree state"
    - "save_to_file() extracts core write logic; save() delegates to it — DRY without API change"

key-files:
  created: [src/preset_manager.h, src/preset_manager.cpp, tests/test_phase11_dod.cpp]
  modified: [src/audio/pad_bank.h, src/plugin_processor.h, src/plugin_processor.cpp, CMakeLists.txt, tests/CMakeLists.txt]

key-decisions:
  - "source_file_ placed in public section of SamplePad (consistent with gain/pan/etc public members)"
  - "PLock values clamped ±1e6 on restore — physical values not normalized (filter_cutoff=800, not 0-1)"
  - "P11D5 strips subtrees from a live v5 blob — tests actual compat path without needing a real v4 artifact"
  - "save_to_file() added by audit M1: PresetManager::save() delegates to it; tests use TemporaryFile"

patterns-established:
  - "State subtree naming: {name}_v{version} — enables future v6 subtrees without conflict"
  - "Bounds clamping on all ValueTree reads: jlimit for scalars, existsAsFile() guard for file paths"
  - "Non-vacuous round-trip tests: set non-default value before serialize, assert exact value after restore"

duration: ~2h
started: 2026-06-18T03:00:00Z
completed: 2026-06-19T00:00:00Z
description: "State v5 serialization (StepPattern + FeelPattern + PLockPattern + SamplePad paths) + PresetManager (.bqpreset) + 5 round-trip tests"
type: Summary
about: "BAQUE"
---

# Phase 11 Plan 01: State v5 + PresetManager Summary

**State v5 serialization covers full engine state; PresetManager saves/loads .bqpreset files with backward compat to v4 blobs; 249/249 tests pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~2h |
| Started | 2026-06-18T03:00:00Z |
| Completed | 2026-06-19T00:00:00Z |
| Tasks | 3/3 completed |
| Files modified | 8 (5 modified, 3 created) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Full engine state round-trips through DAW save/load | Pass | P11D1 verifies StepPattern (step 0/0 off, step 1/3 note=48); P11D2 verifies FeelPattern (enabled, timing_ms=25.5f, seed=42) |
| AC-2: PresetManager saves to file and loads faithfully | Pass | P11D3: master_gain=0.3f → save_to_file → fresh proc → load → master_gain restored within 0.01 |
| AC-3: SamplePad source_file_ persists through state round-trip | Pass | P11D4: load_sample_from_file(2) → round-trip → source_file() == original path AND num_samples() > 0 |
| AC-4: v4 state loads without crash | Pass | P11D5: strip v5 subtrees from live blob → setStateInformation → no throw, feel.enabled==false, source_file()==File{} |

## Accomplishments

- **State v5 is complete**: Every significant piece of engine state now survives a DAW project save/load cycle — step grid (16×16), feel settings (enable/humanize/timing offsets/seed), p-lock automation (all 15 params per step), and which sample file is loaded to each of the 16 pads.
- **PresetManager is live**: save/load/list for `.bqpreset` files in `userApplicationDataDirectory/BAQUE/presets/`. File format: BQP1 magic + length-prefixed XML metadata + binary ValueTree — forward-compatible for future factory preset reader (Phase 11-02).
- **5 non-vacuous round-trip tests**: Each test mutates state, serializes, deserializes into a fresh processor, and asserts exact values. P11D2 tests FeelPattern with non-default values (not just defaults-in/defaults-out). P11D3 uses TemporaryFile (no user-dir pollution in CI). P11D5 proves v4 blobs silently degrade rather than crash.

## Task Commits

All 3 tasks completed in a single atomic commit (autonomous plan, no checkpoints):

| Task | Commit | Description |
|------|--------|-------------|
| Task 1: source_file_ + state v5 | `e1e2fe1` | pad_bank.h + plugin_processor.h/.cpp |
| Task 2: PresetManager | `e1e2fe1` | preset_manager.h/.cpp + CMakeLists.txt |
| Task 3: P11D1–P11D5 tests | `e1e2fe1` | test_phase11_dod.cpp + tests/CMakeLists.txt |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `src/audio/pad_bank.h` | Modified | Added `source_file_{}` + `source_file()` + `set_source_file()` to SamplePad public section; added `<juce_core/juce_core.h>` |
| `src/plugin_processor.h` | Modified | `k_state_version=5`; added `set_feel_pattern(const FeelPattern&)` public method |
| `src/plugin_processor.cpp` | Modified | `load_sample_from_file()` calls `set_source_file(file)`; `getStateInformation` appends 4 v5 subtrees; `setStateInformation` restores 4 subtrees with bounds clamping; `set_feel_pattern()` implementation |
| `src/preset_manager.h` | Created | PresetManager class declaration: save/save_to_file/load/list_user_presets/user_preset_dir/k_extension |
| `src/preset_manager.cpp` | Created | Full implementation: BQP1 magic header, save_to_file core write, save delegates, load strips header, list sorts by name |
| `CMakeLists.txt` | Modified | Added `src/preset_manager.cpp` to target_sources |
| `tests/test_phase11_dod.cpp` | Created | P11D1–P11D5 round-trip tests |
| `tests/CMakeLists.txt` | Modified | Added `test_phase11_dod.cpp` to baque_tests |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `source_file_` in public section of SamplePad | Consistent with gain/pan/adsr/play_mode which are all public; underscore naming follows buffer_ convention but placement matches class style | Matches existing API surface — no extra getter needed for direct writes in load path |
| PLock values clamped `±1e6` not `±1.0` | P-lock values are physical (filter_cutoff=800Hz, not 0.0–1.0 normalized); tight clamp would break legitimate values | Protects against NaN/Inf from corrupted presets without constraining real param range |
| `save_to_file()` extracts core write logic | Audit M1: original `save()` wrote to `user_preset_dir()` only — no way to use TemporaryFile in tests without polluting real preset dir | P11D3 deterministically passes/fails in CI with no graceful-skip path |
| `set_feel_pattern()` not RT-safe | Added for state-load and test use only (analogous to `set_pattern()` from 05-01); live edits go through UiCommandQueue in Phase 11-02 | Tests can set non-default FeelPattern without spawning audio thread; clearly documented in header |
| v5 subtrees appended after v4 subtrees | Order in ValueTree doesn't matter for `getChildWithName()`; appending preserves v4 structure for any tools reading the binary directly | v4 blobs load with no action required — missing subtrees → invalid ValueTree → branches skipped |
| P11D5 strips subtrees from live v5 blob | Constructing a valid v4 blob from scratch requires a real v4 build; stripping v5 subtrees achieves the same test without that dependency | Immediate CI coverage of AC-4 without needing historical build artifact |

## Deviations from Plan

None — all audit changes were incorporated into the PLAN before APPLY began. Implementation followed plan exactly.

## Issues Encountered

None. Build: 0 errors. Tests: 249/249. All 5 P11D pass including P11D4 (requires test_kick.wav binary data which was already in BinaryData from Phase 10-07).

## Next Phase Readiness

**Ready:**
- getStateInformation/setStateInformation v5 foundation: Phase 11-02 preset browser and factory presets use the same load path (pm2.load → setStateInformation)
- PresetManager.load() accepts arbitrary files: factory presets shipped as BinaryData can write to TemporaryFile then call load() — same code path as user presets
- list_user_presets() returns sorted array: 11-02 browser can call this directly for the Presets tab
- `.bqpreset` format is fixed: BQP1 magic + meta XML + binary state. Backward-compat for future v6 state is free (same invalid-ValueTree guard pattern)

**Concerns:**
- `set_feel_pattern()` bypasses UiCommandQueue — safe under suspend or in tests (no audio thread), but risky if called live. Phase 11-02 should add a `set_feel_pattern` UiCommand before any live UI controls for FeelPattern are wired up.
- Velocity layer buffers NOT serialized — only primary buffer path persisted via `source_file_`. Pads set up via chop-to-pads workflow lose their velocity layers on preset load. Noted in plan boundaries; document as limitation in Phase 11-02 preset browser UI.
- APVTS mute/solo still not round-tripping through engine PerfState — existing deferred issue from Phase 10-05. Does not block Phase 11-02 but should be addressed before v1.0 release.

**Blockers:**
None.

---
*Built with PAUL Framework v1.4*
*Phase: 11-presets-library, Plan: 01*
*Completed: 2026-06-19*
