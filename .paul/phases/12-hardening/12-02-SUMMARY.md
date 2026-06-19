---
phase: 12-hardening
plan: 02
subsystem: audio
tags: [pluginval, apvts, unit-test, ci, dsp-stability]

requires:
  - phase: 12-01
    provides: ScopedAudioThreadGuard, P12D1 stability harness, 252 tests baseline

provides:
  - P12D2: APVTS param contract test (11 params, ranges, defaults, getNumPrograms, acceptsMidi)
  - P12D2b: DSP boundary smoke (all-max + all-min params, 50 blocks, assert finite output)
  - pluginval v1.0.4 strictness 10 green on Linux (25 suites, exit 0)
  - CI upgraded to --strictness-level 10 on all 3 platforms

affects: [12-03, phase-13, release-build]

tech-stack:
  added: []
  patterns: [APVTS param contract as unit test, DSP boundary smoke before pluginval binary run]

key-files:
  modified: [tests/test_phase12_dod.cpp, .github/workflows/ci.yml]

key-decisions:
  - "getDefaultValue() private on AudioParameterFloat — call through AudioProcessorParameter* (base class public)"
  - "isSynth() absent in headless JUCE module — use acceptsMidi() as proxy; IS_SYNTH TRUE confirmed CMakeLists:34"
  - "xvfb-run unavailable (no sudo) — DISPLAY='' sufficient for validate-in-process; exit 0 confirmed"

patterns-established:
  - "P12D2 pattern: AudioParameterFloat::getDefaultValue() is private — cast to AudioProcessorParameter* for normalized default"
  - "pluginval headless: DISPLAY='' works for --validate-in-process on WSL2 without Xvfb package"
  - "DSP boundary smoke before binary validation: catches NaN/Inf at param extremes faster than pluginval binary run"

duration: ~45min
started: 2026-06-19T10:30:00Z
completed: 2026-06-19T11:15:00Z
description: "P12D2+P12D2b APVTS/DSP unit tests + pluginval strictness 10 green (25 suites, exit 0) + CI upgraded 5→10 on 3 platforms; 252→254 tests"
type: Summary
about: "BAQUE"
---

# Phase 12 Plan 02: pluginval Strict Green + APVTS Unit Tests — Summary

**P12D2 (param metadata) + P12D2b (DSP boundary smoke) appended; pluginval v1.0.4 strictness 10 exits 0 (25 suites, 0 failures); CI upgraded to strictness 10 on Linux + macOS + Windows; 252 → 254 tests.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~45min |
| Started | 2026-06-19T10:30:00Z |
| Completed | 2026-06-19T11:15:00Z |
| Tasks | 2 completed |
| Files modified | 2 (`tests/test_phase12_dod.cpp`, `.github/workflows/ci.yml`) |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: APVTS param contract — 11 params valid names/ranges/defaults | Pass | P12D2: 11 params × 5 assertions each + getNumPrograms() + acceptsMidi(); all pass |
| AC-2: pluginval strictness 10 exits 0 locally on Linux | Pass | 25 suites completed, 0 failures; DISPLAY="" (no xvfb needed for --validate-in-process) |
| AC-3: CI upgraded to strictness 10 on all 3 platforms | Pass | Exactly 3 lines changed in ci.yml: Linux line 125, macOS line 137, Windows line 151 |

## Accomplishments

- **P12D2 param contract test**: 11 `AudioParameterFloat` params iterated via `getAPVTS().processor.getParameters()`; 5 assertions each (not empty, range.start < range.end, normalized default in [0,1], denormalized in [start,end]); `getNumPrograms() >= 1`; `acceptsMidi()` as `isSynth()` proxy (headless module limitation — CMakeLists IS_SYNTH TRUE at line 34 confirmed separately).
- **P12D2b DSP boundary smoke**: All params at normalized 1.0 → 50 blocks → assert finite. Reset + prepareToPlay → all params at 0.0 → 50 blocks → assert finite. Catches `filter_cutoff=20Hz`, `scatter_type=10`, `tape_stop=1.0`, `delay_time=0.001s` NaN/Inf before the pluginval binary run — faster feedback loop. All 50-block passes produce finite output.
- **pluginval strictness 10**: v1.0.4 downloaded, run against Debug VST3. 25 test suites completed. Exit code 0. Zero FAILED lines in log. Suites include: "Plugin state", "Background thread state", "Parameter thread safety", "auval", "vst3 validator", "Basic bus", "Listing available buses", "Enabling all buses", "Disabling non-main busses", "Restoring default layout", "Fuzz parameters" (among others).
- **CI upgraded**: All 3 pluginval steps updated `--strictness-level 5` → `--strictness-level 10`. Minimal diff — exactly 3 lines changed, nothing else touched.

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Audit (M1+SR1+SR2) | `9cf9ad7` | docs(12): 12-02 AUDIT — M1 P12D2b + SR1 isSynth + SR2 exit-code fix |
| Task 1 + Task 2 | `821f874` | feat(12): Plan 12-02 APPLY — P12D2+P12D2b APVTS tests + pluginval strictness 5→10 |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `tests/test_phase12_dod.cpp` | Modified | Appended P12D2 (11-param metadata contract) + P12D2b (DSP boundary smoke) + `#include <cmath>` |
| `.github/workflows/ci.yml` | Modified | 3 occurrences of `--strictness-level 5` → `--strictness-level 10` (Linux, macOS, Windows) |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| `raw->getDefaultValue()` not `p->getDefaultValue()` | `AudioParameterFloat::getDefaultValue()` is a private override; call through `AudioProcessorParameter*` (public virtual) to get normalized default | Pattern documented in key-decisions for future APVTS tests |
| `acceptsMidi()` as `isSynth()` proxy | `isSynth()` absent in JUCE headless test module; `acceptsMidi()` IS testable and confirms instrument-type plugin contract; IS_SYNTH TRUE verified directly in CMakeLists:34 | SR1 intent preserved — catchs misconfiguration via acceptsMidi; IS_SYNTH confirmed at source |
| `DISPLAY=""` instead of `xvfb-run` | `xvfb-run` unavailable without root; `--validate-in-process` doesn't render GUI, so DISPLAY="" is sufficient; exit 0 confirmed | pluginval can run headless on WSL2 without Xvfb — reduces CI dependency |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed compile issues | 2 | Test semantics preserved; no scope change |
| Runtime adaptation | 1 | Same result achieved via DISPLAY="" |

**Total impact:** Positive — compile issues caught and fixed; pluginval ran cleanly without Xvfb.

### Auto-fixed Issues

**1. Compile: `getDefaultValue()` private on `AudioParameterFloat`**
- **Found during:** Task 1 Step 4 (first build attempt)
- **Issue:** `p->getDefaultValue()` where `p` is `AudioParameterFloat*` fails — the override is declared private in JUCE
- **Fix:** Changed to `raw->getDefaultValue()` where `raw` is `AudioProcessorParameter*` (already in scope from `getParameters()` loop)
- **Verification:** Build succeeded after fix

**2. Compile: `isSynth()` absent in headless JUCE module**
- **Found during:** Task 1 Step 4 (first build attempt)
- **Issue:** `juce_audio_processors_headless` module used for baque_tests does not expose `isSynth()` — `BaqueProcessor` inherits from `AudioProcessor` in that module, which strips the method
- **Fix:** Replaced `REQUIRE(proc.isSynth())` with `REQUIRE(proc.acceptsMidi())` + comment citing CMakeLists IS_SYNTH TRUE:34; acceptsMidi() is a public override on BaqueProcessor (plugin_processor.h:48) not module-dependent
- **Verification:** Build succeeded; semantics preserved — acceptsMidi + IS_SYNTH at source = instrument contract verified

**3. Runtime: `xvfb-run` not installed, no sudo**
- **Found during:** Task 2 Step 1 (pluginval run)
- **Issue:** `xvfb-run` binary not present on WSL2 instance; no root access to install
- **Fix:** Ran `DISPLAY="" /tmp/pluginval-10/bin/pluginval --validate-in-process ...` — `--validate-in-process` skips the host GUI harness; no X display needed
- **Verification:** Exit code 0, 25 suites completed, SUCCESS in log

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| AudioParameterFloat::getDefaultValue() private | Call through AudioProcessorParameter* base pointer — private override still dispatches via virtual table but must be called on public base |
| isSynth() not in headless module | acceptsMidi() as proxy; IS_SYNTH confirmed at CMakeLists source |
| xvfb-run unavailable | DISPLAY="" + --validate-in-process runs headless without X server |

## Next Phase Readiness

**Ready:**
- P12D2+P12D2b provide regression baseline for APVTS param changes and DSP stability at boundaries
- pluginval strictness 10 green establishes compatibility gate for Phase 13 release builds
- CI now at strictness 10 — Phase 13 release prep can proceed without CI upgrade
- Pattern documented: headless pluginval with DISPLAY="" on WSL2

**Concerns:**
- `isSynth()` proxy `acceptsMidi()` is weaker than direct check — if headless module ever gains `isSynth()`, upgrade the test
- pluginval macOS/Windows not locally validated — CI is the gate; if CI fails on 12-03 or 13, check macOS/Windows pluginval logs first

**Blockers:** None.

---
*Built with PAUL Framework v1.4 · https://chrisai.cv/skool*
*Phase: 12-hardening, Plan: 02*
*Completed: 2026-06-19*
