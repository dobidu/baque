---
phase: 13-release
plan: 01
subsystem: docs-packaging
tags: [readme, notice, lgpl, cmake-install, version-bump, phase13]

requires:
  - phase: 12-03
    provides: Phase 12 DoD closed — pluginval strictness 10 + 64-voice + zero RT-alloc

provides:
  - README.md v1.0: 9-section user-facing README, 176 lines, per-platform install paths
  - NOTICE: accurate attribution — SoundTouch LGPL-2.1 §6 compliant with fork URL, pluginval ISC added
  - CMakeLists.txt: VERSION 1.0.0; install() rules for VST3/Standalone/AU
  - cmake --install: VST3/ + Standalone/ populated correctly on Linux

affects: [Plan 13-02 — CPack uses install() rules from this plan; LGPL §6 resolved]

key-files:
  modified: [README.md, NOTICE, CMakeLists.txt]
  created: [.paul/phases/13-release/13-01-PLAN.md, .paul/phases/13-release/13-01-AUDIT.md]

key-decisions:
  - "Used actual SoundTouch fork URL in NOTICE instead of DISTRIBUTION_GATE placeholder — URL found in CMakeLists.txt line 21"
  - "macOS Standalone install() uses install(DIRECTORY .../BAQUE.app) per audit SR1 — not install(PROGRAMS)"
  - "Reconfigure required: install() rules added after initial cmake -B; cmake --install from old cache showed only JUCE headers"

duration: ~30min
started: 2026-06-19T18:00:00Z
completed: 2026-06-19T19:00:00Z
description: "README v1.0 + NOTICE LGPL compliance + VERSION 1.0.0 + CMake install() rules — 256/256 tests green"
type: Summary
about: "BAQUE"
---

# Phase 13 Plan 01: README v1.0 + NOTICE + Version Bump + CMake install() Summary

**README v1.0 + LGPL §6 compliant NOTICE + VERSION 1.0.0 + cmake --install working — 256/256 green**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~30min |
| Started | 2026-06-19T18:00:00Z |
| Completed | 2026-06-19T19:00:00Z |
| Tasks | 2 completed |
| Files modified | 3 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: README.md v1.0 release quality | Pass | 176 lines (was 88); 9 sections; per-platform install paths; Features/Contributing/Identity/License all present |
| AC-2: NOTICE accurate — no stale placeholders | Pass | SoundTouch LGPL-2.1 §6 compliant with actual fork URL; pluginval ISC added; no "not integrated yet" text |
| AC-3: Version 1.0.0 + install() functional | Pass | project(BAQUE VERSION 1.0.0); 256/256 pass; cmake --install → VST3/BAQUE.vst3 + Standalone/BAQUE |

## Accomplishments

- README.md: 88→176 lines, 9 sections structured for GitHub project page (What/Features/Install/Build/Test/Identity/Contributing/License)
- NOTICE: SoundTouch LGPL-2.1 §6 fully resolved — actual fork URL `https://github.com/dobidu/baque-soundtouch` present; pluginval ISC entry added
- CMakeLists.txt: VERSION 0.0.1→1.0.0; install() rules for VST3/Standalone/AU with platform-specific Standalone handling (WIN32/APPLE/Linux)
- cmake --install verified: VST3/BAQUE.vst3 and Standalone/BAQUE present in prefix after reconfigure
- 256/256 tests green — version bump is pure metadata, no DSP change

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Task 1+2: All changes | `51a4ff2` | feat(13): Plan 13-01 APPLY — README v1.0 + NOTICE cleanup + version 1.0.0 + CMake install() rules |

## Files Modified

| File | Change | Purpose |
|------|--------|---------|
| `README.md` | Rewritten 88→176 lines | v1.0 user-facing README; 9 sections; all platform install paths |
| `NOTICE` | Updated 44→55 lines | SoundTouch LGPL-2.1 §6 compliant (fork URL added); pluginval ISC entry added; stale placeholder removed |
| `CMakeLists.txt` | VERSION bump + install() rules | VERSION 0.0.1→1.0.0; install(DIRECTORY) for VST3/AU bundles; install(PROGRAMS) for Linux Standalone; install(DIRECTORY .../BAQUE.app) for macOS Standalone |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| Used actual fork URL in NOTICE instead of DISTRIBUTION_GATE | Fork URL `https://github.com/dobidu/baque-soundtouch` found in CMakeLists.txt line 21 during APPLY; real compliance better than placeholder | LGPL §6 fully resolved for v1.0 — no deferred compliance debt for Plan 13-02 |
| Reconfigure after install() rules added | cmake --install from old cache installed only JUCE headers — build directory didn't know about new install() targets | `cmake -B build` reconfigure required before `cmake --install` picks up new rules |
| macOS Standalone: `install(DIRECTORY .../BAQUE.app)` | Audit SR1: BAQUE.app is a directory bundle; `install(PROGRAMS)` silently does nothing for directories | macOS CPack in Plan 13-02 will correctly package the Standalone .app bundle |

## Deviations from Plan

| Type | Detail | Impact |
|------|--------|--------|
| Improvement | DISTRIBUTION_GATE placeholder → actual fork URL | Better: LGPL §6 fully resolved vs. deferred to Plan 13-02; URL found in CMakeLists.txt line 21 |
| Scope extension | cmake reconfigure required after install() rules added | Necessary step not in plan; standard cmake behavior when rules added post-configure |

## Next Phase Readiness

**Ready:**
- Plan 13-02 can now use install() rules from CMakeLists.txt for CPack configuration
- VERSION 1.0.0 propagated through JUCE (PLUGIN_VERSION auto-inherits PROJECT_VERSION)
- NOTICE LGPL-2.1 §6 compliance complete — no deferred compliance items

**Concerns:**
- None — docs/versioning plan with no DSP changes; 256/256 green confirms no regressions

**Blockers:**
- None

---
*Built with PAUL Framework v1.4*
*Phase: 13-release, Plan: 01*
*Completed: 2026-06-19*
