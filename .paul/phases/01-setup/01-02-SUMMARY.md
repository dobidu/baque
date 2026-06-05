---
phase: 01-setup
plan: 02
subsystem: infra
tags: [catch2, github-actions, ci, pluginval, cmake, testing]

requires:
  - phase: 01-setup/01-01
    provides: CMakeLists.txt with JUCE 8.0.13, scripts/run_pluginval.sh, git repo on main

provides:
  - tests/ target with Catch2 v3.15.0 and processor smoke test
  - .github/workflows/ci.yml — 3-OS CI (ubuntu/macos/windows)
  - macOS: universal binary gate (lipo) + AU validation (auval)
  - pluginval v1.0.4 on all 3 OSes in CI
  - clang-format lint job in CI
affects: [all-phases, 12-hardening]

tech-stack:
  added: [Catch2 v3.15.0, GitHub Actions, pluginval v1.0.4]
  patterns:
    - "DISCOVERY_MODE PRE_TEST for Catch2 on WSL2 (avoids post-build binary race)"
    - "ASCII-only test names (Windows ctest UTF-8 filter bug)"
    - "xvfb-run for JUCE headless tests on Linux CI"
    - "auval codes from plugin identity: -v aumu Bqe1 Lvd0"

key-files:
  created:
    - tests/CMakeLists.txt
    - tests/test_smoke.cpp
    - .github/workflows/ci.yml
  modified:
    - CMakeLists.txt (add_subdirectory tests)

key-decisions:
  - "ASCII-only test names: Windows ctest mangles UTF-8 in PRE_TEST filter"
  - "DISCOVERY_MODE PRE_TEST: avoids post-build binary race on WSL2/CI"
  - "Node.js 20 action deprecation: deferred to cleanup (deadline June 16)"

patterns-established:
  - "Test fixture uses juce::ScopedJuceInitialiser_GUI — required for JUCE types in tests"
  - "Link test target against juce::juce_audio_processors + juce::juce_audio_utils explicitly"
  - "CI pluginval uses same pinned version + sha256 as scripts/run_pluginval.sh"

duration: ~3h (first CI run + fix)
started: 2026-06-04T21:00:00Z
completed: 2026-06-04T22:40:00Z
description: "Catch2 v3.15.0 smoke tests + 3-OS GitHub Actions CI green (ubuntu/macos/windows) with pluginval, lipo universal gate, auval AU validation"
type: Summary
about: "BAQUE"
---

# Phase 1 Plan 02: Catch2 + CI Summary

**Catch2 v3.15.0 smoke tests local and in 3-OS CI (ubuntu/macos/windows) — all green; pluginval, lipo universal gate, auval, lint all pass.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~3h (first run failure + fix + rerun) |
| Started | 2026-06-04T21:00:00Z |
| Completed | 2026-06-04T22:40:00Z |
| Tasks | 2 auto + 1 checkpoint — all complete |
| Files created | 3 |
| Files modified | 1 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Tests run locally | Pass | ctest 2/2 pass; `framework wiring` + `BaqueProcessor processBlock outputs silence` |
| AC-2: CI green all three OSes | Pass | Run 26983527116: ubuntu ✓ macos ✓ windows ✓ lint ✓ |
| AC-3: macOS universal binary | Pass | lipo -info asserts arm64 + x86_64; job-failing if missing |
| AC-4: AU validated on macOS | Pass | auval -v aumu Bqe1 Lvd0 passes in macOS CI leg |

## Accomplishments

- 3-OS GitHub Actions CI live and green — every push to main runs build + ctest + pluginval + lint
- macOS universal binary asserted by job-failing lipo step (audited requirement)
- auval AU validation in macOS CI leg — AU format tested every push (not just at release)
- Catch2 PRE_TEST discovery avoids WSL2 post-build race; tests run without xvfb locally

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Tasks 1+2: Catch2 + CI | `62a300f` | feat(01-setup): Catch2 smoke tests + GitHub Actions 3-OS CI |
| Fix: ASCII names | `1dc8584` | fix(tests): ASCII-only test names for Windows ctest compat |
| WIP .paul/ state | `f4f465b` | wip(01-setup): 01-02 APPLY done — CI green, awaiting UNIFY |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `tests/CMakeLists.txt` | Created | Catch2 v3.15.0, baque_tests target, PRE_TEST discovery |
| `tests/test_smoke.cpp` | Created | 2 smoke cases: framework wiring + processor silence check |
| `.github/workflows/ci.yml` | Created | 3-OS matrix, ctest, pluginval, lipo, auval, lint |
| `CMakeLists.txt` | Modified | add_subdirectory(tests) wired in |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| ASCII-only test names | Windows ctest PRE_TEST filter mangles UTF-8 chars (em-dash, ê) → "no tests ran" | All test names going forward must be ASCII-only |
| DISCOVERY_MODE PRE_TEST | Post-build discovery fails on WSL2 because binary doesn't exist yet at CMake post-build hook time | Standard pattern for JUCE+Catch2 on WSL2 and CI |
| Defer Node.js 20 action upgrade | Non-blocking today; actions/checkout@v5 + cache@v5 is trivial cleanup; deadline June 16 | Track in deferred issues; do before or during Phase 12 |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 2 | Essential correctness; no scope change |
| Deferred | 1 | Non-blocking maintenance item |

**Total impact:** Two essential fixes caught by qualify, one deferred maintenance item. No scope creep.

### Auto-fixed Issues

**1. Missing JUCE module links in test target**
- **Found during:** Task 1 qualify step (build failure)
- **Issue:** `tests/CMakeLists.txt` linked only against BAQUE but JUCE module include paths not transitively exposed; `juce_audio_processors/juce_audio_processors.h` not found
- **Fix:** Added `juce::juce_audio_processors` and `juce::juce_audio_utils` to `target_link_libraries(baque_tests)`
- **Files:** tests/CMakeLists.txt
- **Verification:** cmake --build succeeds, baque_tests binary produced
- **Commit:** 62a300f (part of initial commit, fixed before push)

**2. UTF-8 test names fail Windows ctest**
- **Found during:** Task 3 (checkpoint) — first CI run, Windows leg
- **Issue:** Test name "BaqueProcessor — processBlock produz silêncio" contains em-dash + accented char; Windows codepage mangles them in PRE_TEST filter → "no tests ran" → exit 1
- **Fix:** Renamed to ASCII-only: "BaqueProcessor processBlock outputs silence" + "framework wiring"
- **Files:** tests/test_smoke.cpp
- **Verification:** CI run 26983527116 Windows leg: 2/2 pass
- **Commit:** 1dc8584

### Deferred Items

- **Node.js 20 action deprecation:** actions/checkout@v4 + actions/cache@v4 flagged as deprecated (deadline June 16, 2026). Upgrade to v5 is mechanical. Deferred to Phase 12 (Hardening) or earlier cleanup commit.

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| First CI run: Windows ctest "no tests ran" (UTF-8 name mangling) | Diagnosed from CI log; fixed with ASCII-only names in 1dc8584 |
| Test target: juce headers not found at build | Added explicit JUCE module links to baque_tests target |

## Next Phase Readiness

**Ready:**
- CI gate live on every push — Phase 2+ can merge with confidence
- Catch2 framework wired; DSP tests can be added as DSP is built
- pluginval strictness 5 runs in CI — regression detection from day one
- GitHub repo: https://github.com/dobidu/baque

**Concerns:**
- Node.js 20 action deprecation deadline June 16, 2026 — track and upgrade before then
- LTO in CI makes first build slow (~8min Windows); caching mitigates subsequent runs

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 01-setup, Plan: 02*
*Completed: 2026-06-04*
