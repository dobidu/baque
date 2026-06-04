---
phase: 01-setup
plan: 01
subsystem: infra
tags: [cmake, juce, vst3, standalone, pluginval, gpl, cpp17]

requires: []
provides:
  - CMakeLists.txt with JUCE 8.0.13 (FetchContent, pinned tag)
  - BaqueProcessor / BaqueEditor skeleton (silence, versioned ValueTree state)
  - Repo hygiene: LICENSE (GPLv3), NOTICE, .gitignore, .clang-format, .clang-tidy, README.md
  - scripts/run_pluginval.sh (pinned v1.0.4, sha256-verified)
  - Git repo initialized on main branch with 2 commits
affects: [01-02, all-phases]

tech-stack:
  added: [JUCE 8.0.13, CMake 3.22+, clang-format, clang-tidy, pluginval v1.0.4]
  patterns:
    - Plugin identity locked at creation (bundle ID + 4-char codes)
    - JUCE via FetchContent with exact pinned tag
    - Versioned ValueTree state from day one (k_state_version = 1)
    - -Wall -Wextra on BAQUE targets only (not JUCE deps)

key-files:
  created:
    - CMakeLists.txt
    - src/plugin_processor.{h,cpp}
    - src/plugin_editor.{h,cpp}
    - LICENSE
    - NOTICE
    - .gitignore
    - .clang-format
    - .clang-tidy
    - README.md
    - scripts/run_pluginval.sh
  modified: []

key-decisions:
  - "Plugin identity permanent: br.ufpb.lavid.baque / Lvd0 / Bqe1 — audit-required, locked at skeleton"
  - "JUCE 8.0.13 exact tag: never a branch ref, must update deliberately"
  - "git init -b main: ensures CI workflow trigger in 01-02 matches default branch"
  - "Versioned ValueTree (version=1) instead of empty stubs: enables future migration without shim"

patterns-established:
  - "Source files formatted with clang-format before commit (LLVM-based, 4-space, column 120)"
  - "pluginval run under xvfb-run on Linux (no display requirement)"
  - "Scripts fetch tools pinned by version + sha256, not floating latest"

duration: ~6h (JUCE FetchContent + configure dominated)
started: 2026-06-04T00:00:00Z
completed: 2026-06-04T01:15:00Z
description: "JUCE 8.0.13 CMake skeleton builds VST3+Standalone on Linux; pluginval strictness 5 PASS; repo hygiene committed on main"
type: Summary
about: "BAQUE"
---

# Phase 1 Plan 01: CMake + JUCE Skeleton Summary

**JUCE 8.0.13 CMake skeleton builds VST3 + Standalone on Linux; pluginval strictness 5 passes; GPLv3 repo hygiene committed on main.**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~6h (FetchContent + configure ~330s, build ~20min LTO) |
| Started | 2026-06-04T00:00:00Z |
| Completed | 2026-06-04T01:15:00Z |
| Tasks | 3 completed |
| Files created | 10 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: Project builds | Pass | cmake --build exits 0; Standalone (5.6M) + BAQUE.vst3 (4.7M) present |
| AC-2: Plugin loads empty | Pass | pluginval strictness 5: ALL TESTS PASSED — all bus, state, editor, audio, automation suites |
| AC-3: Repo hygiene | Pass | LICENSE/NOTICE/.gitignore/.clang-format/.clang-tidy/README.md all present; git log shows initial commit on main |

## Accomplishments

- JUCE 8.0.13 fetched and built successfully on WSL2 Linux (x86_64)
- Plugin identity locked permanently: `br.ufpb.lavid.baque` / `'Lvd0'` / `'Bqe1'` — safe for DAW session state
- Versioned ValueTree state serialization in place from day one (no migration shim needed later)
- pluginval v1.0.4 script: pinned version + sha256 verification, xvfb-run for headless Linux

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| Tasks 1+2: skeleton + hygiene | `c74dee5` | feat(setup): JUCE 8.0.13 plugin skeleton + repo hygiene |
| Task 3: pluginval script | `498f400` | feat(setup): add pluginval v1.0.4 validation script |
| WIP .paul/ state | `d865c00` | wip(01-setup): paused at 01-01 UNIFY |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `CMakeLists.txt` | Created | JUCE 8.0.13 FetchContent, plugin targets, warning flags |
| `src/plugin_processor.h` | Created | BaqueProcessor declaration, state versioning constant |
| `src/plugin_processor.cpp` | Created | Silence processBlock, versioned getState/setState, createPluginFilter |
| `src/plugin_editor.h` | Created | BaqueEditor placeholder declaration |
| `src/plugin_editor.cpp` | Created | 800×600 resizable editor, dark "Barro" background, ember "BAQUE" label |
| `LICENSE` | Created | GPLv3 full text |
| `NOTICE` | Created | JUCE (GPLv3) + SoundTouch placeholder (LGPL) attributions |
| `.gitignore` | Created | build/, IDE dirs, FetchContent artifacts, pluginval binary |
| `.clang-format` | Created | LLVM-based, 4-space, column 120, IncludeCategories for JUCE |
| `.clang-tidy` | Created | modernize/readability/performance/bugprone; snake_case + `_` suffix naming |
| `README.md` | Created | Build instructions, plugin identity table, doc links, license |
| `scripts/run_pluginval.sh` | Created | Pinned v1.0.4, sha256 verification, xvfb-run, strictness 5 |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| Plugin identity locked at skeleton (audit finding) | DAW sessions encode bundle ID + 4-char codes permanently; changing post-save corrupts sessions | Must never change: br.ufpb.lavid.baque / Lvd0 / Bqe1 |
| Versioned ValueTree state (not empty stubs) | ESCOPO §6 requires preset versioning; stubs force a migration shim at zero savings | setState tolerates missing/older version today; future changes just increment k_state_version |
| git init -b main (audit finding) | CI workflow in 01-02 triggers on main; mismatched default branch = CI silently never runs | 01-02 CI wiring will work correctly |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 1 | No scope change; quality gate maintained |
| Scope additions | 0 | — |
| Deferred | 0 | — |

**Total impact:** Single auto-fix, no scope creep.

### Auto-fixed Issues

**1. clang-format violations in initial source files**
- **Found during:** Task 2 qualify step (clang-format --dry-run --Werror)
- **Issue:** LLVM brace style and alignment rules differed from hand-written formatting (open brace on same line, public: indent, alignment of &&)
- **Fix:** `clang-format -i src/*.cpp src/*.h` + amended the initial commit
- **Files:** src/plugin_processor.{h,cpp}, src/plugin_editor.{h,cpp}
- **Verification:** clang-format --dry-run exits 0 on all 4 files
- **Commit:** c74dee5 (amended)

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| WSL2 /mnt/d 9P filesystem clock skew warnings during build | Cosmetic only — gmake warns but build completes 100%. Build artifacts correct. Deferred to audit watchlist. |
| Configure time ~330s (FetchContent fetches JUCE + builds juceaide) | Expected for first run; subsequent incremental builds are fast. Mitigated in 01-02 by CI caching. |

## Next Phase Readiness

**Ready:**
- CMakeLists.txt extensible: add_subdirectory(tests) already present for Catch2 (01-02)
- Plugin identity permanently set — 01-02 can reference `Bqe1`/`Lvd0` in auval call
- scripts/pluginval.sh ready for CI integration in 01-02
- Git branch `main` confirmed — CI workflow trigger will match

**Concerns:**
- LTO serial compilation (66–79 LTRANS jobs) makes first CI builds slow; 01-02 cache config critical
- AU format only builds on macOS — auval validation in 01-02 is the first gate for it

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 01-setup, Plan: 01*
*Completed: 2026-06-04*
