---
phase: 13-release
plan: 02
subsystem: infra
tags: [cpack, github-actions, release, packaging, codesign, zip, cross-platform]

requires:
  - phase: 13-01
    provides: install() rules for VST3/Standalone/AU + README + NOTICE + v1.0.0 version bump

provides:
  - CPack ZIP packaging for all 3 platforms (component-aware, JUCE-excluded)
  - GitHub Actions release.yml (3-platform build matrix + release job)
  - GitHub Release v1.0.0 with BAQUE-1.0.0-Linux/macOS/Windows.zip
  - Phase 13 DoD closed — Milestone v1.0 COMPLETE

affects: []

tech-stack:
  added: [CPack, github-actions-release]
  patterns:
    - "CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ExternalDeps before FetchContent → excludes JUCE from ZIP"
    - "CPACK_ARCHIVE_COMPONENT_INSTALL ON + CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE — required for ZIP generator to filter components"
    - "GHA secrets context invalid in step if: — map to job-level env, check env.VAR != ''"
    - "YAML run:| block scalar terminated by blank lines at col 0 — use heredoc + --notes-file"
    - "gh release create needs --repo when job has no actions/checkout"

key-files:
  created: [.github/workflows/release.yml]
  modified: [CMakeLists.txt, tests/rt_alloc_counter.cpp, tests/CMakeLists.txt, src/preset_manager.h, src/rt_safety.h]

key-decisions:
  - "No Apple codesign: unsigned macOS ZIP ships; users bypass Gatekeeper or compile locally"
  - "CPACK_ARCHIVE_COMPONENT_INSTALL ON required — CPACK_COMPONENT_UNSPECIFIED_HIDDEN insufficient for ZIP generator"
  - "release job has no checkout: --repo flag required on gh release create"

patterns-established:
  - "Route FetchContent deps to ExternalDeps component before FetchContent_MakeAvailable(), reset after"
  - "GHA codesign: gate on job-level env, not secrets context in step if:"

duration: ~5 days (2026-06-19 → 2026-06-24; 9 GHA iterations, macOS 2h build)
started: 2026-06-19T13:00:00Z
completed: 2026-06-24T00:00:00Z
description: "CPack ZIP packaging + GitHub Actions 3-platform release.yml + v1.0.0 GitHub Release with 3 downloadable artifacts"
type: Summary
about: "BAQUE"
---

# Phase 13 Plan 02: Release Packaging Summary

**CPack ZIP packaging + GitHub Actions 3-platform release.yml + v1.0.0 GitHub Release live with 3 downloadable artifacts — Milestone v1.0 COMPLETE**

## Performance

| Metric | Value |
|--------|-------|
| Duration | ~5 days (2026-06-19 → 2026-06-24) |
| Started | 2026-06-19T13:00:00Z |
| Completed | 2026-06-24T00:00:00Z |
| GHA iterations | 9 runs until green |
| macOS build time | 1h15m (JUCE arm64+x86_64 cold) |
| Files modified | 6 |

## Acceptance Criteria Results

| Criterion | Status | Notes |
|-----------|--------|-------|
| AC-1: CPack produces plugin-only ZIP artifacts | Pass | Linux 6.2MB, 9 files, 0 JUCE headers; CPACK_ARCHIVE_COMPONENT_INSTALL ON required |
| AC-2: release.yml triggers on v* tags and creates GitHub Release | Pass | Run 28000487842: Linux ✓ Windows ✓ macOS ✓ Release job ✓ |
| AC-3: macOS codesign handled (signed or documented unsigned) | Pass | Unsigned — user decision: no Apple cert; Gatekeeper workaround documented |
| AC-4: v1.0.0 tag pushed and GitHub Release live | Pass | https://github.com/dobidu/baque/releases/tag/v1.0.0 |
| AC-5: Phase 13 DoD | Pass | Linux 6.2MB ✓ macOS 10.1MB ✓ Windows 6.2MB ✓ all downloadable |

## Accomplishments

- CPack component-aware ZIP packaging: only VST3/Standalone(/AU) in artifacts, JUCE headers excluded via `CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "ExternalDeps"` before FetchContent + `CPACK_ARCHIVE_COMPONENT_INSTALL ON` + `CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE`
- 3-platform GitHub Actions release.yml: Linux (6m), Windows (9m), macOS universal ARM64+x86_64 (1h15m); release job gated on all 3 builds
- GitHub Release v1.0.0 live with 3 ZIPs: BAQUE-1.0.0-Linux.zip (6.2MB), BAQUE-1.0.0-macOS.zip (10.1MB), BAQUE-1.0.0-Windows.zip (6.2MB)
- 8 platform-portability and CI bugs found and fixed during GHA iteration

## Task Commits

| Task | Commit | Type | Description |
|------|--------|------|-------------|
| Task 1+2: CPack + release.yml | `2bb82ab` | feat | Plan 13-02 APPLY — CPack config + GitHub Actions release.yml |
| fix: YAML block scalar | `10729c1` | fix | release.yml — fix YAML block scalar termination in release notes |
| fix: codesign if: condition | `f3b946e` | fix | release.yml — use env context for codesign if-condition |
| fix: Windows dlfcn.h | `aef31e3` | fix | Windows build — guard dlfcn.h and dl link behind !_WIN32 |
| fix: macOS free() noexcept | `bd533fa` | fix | macOS build — drop noexcept from free() override |
| fix: clang-format lint | `e0fdd41` | style | clang-format preset_manager.h and rt_safety.h |
| fix: AU optional install | `db516c9` | fix | macOS CPack — make AU component install optional |
| fix: gh release --repo | `11f8605` | fix | release job — pass --repo to gh release create |

## Files Created/Modified

| File | Change | Purpose |
|------|--------|---------|
| `.github/workflows/release.yml` | Created | 3-platform build matrix + release job; triggers on v* tags |
| `CMakeLists.txt` | Modified | CPack config (ZIP, component-aware, ExternalDeps exclusion) |
| `tests/rt_alloc_counter.cpp` | Modified | #ifndef _WIN32 guard for dlfcn.h + free() noexcept removed |
| `tests/CMakeLists.txt` | Modified | dl link guarded: `$<$<NOT:$<PLATFORM_ID:Windows>>:dl>` |
| `src/preset_manager.h` | Modified | clang-format (CI lint fix) |
| `src/rt_safety.h` | Modified | clang-format (CI lint fix) |

## Decisions Made

| Decision | Rationale | Impact |
|----------|-----------|--------|
| No Apple codesign secrets | User: no Apple cert available; macOS users compile locally or bypass Gatekeeper | macOS ZIP ships unsigned; codesign step skipped cleanly |
| CPACK_ARCHIVE_COMPONENT_INSTALL ON | CPACK_COMPONENT_UNSPECIFIED_HIDDEN alone insufficient for ZIP generator filtering | Required to exclude JUCE headers from ZIP |
| CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "ExternalDeps" before FetchContent | Routes all FetchContent dep install() rules to hidden component | JUCE/SoundTouch headers excluded without modifying their CMakeLists |
| --repo flag on gh release create | Release job has no actions/checkout; gh CLI can't detect repo from .git | Required for gh release create to work without checkout |

## Deviations from Plan

### Summary

| Type | Count | Impact |
|------|-------|--------|
| Auto-fixed | 8 | GHA portability + YAML + CMake bugs; all essential |
| Scope additions | 0 | — |
| Deferred | 0 | — |

**Total impact:** Essential fixes; 9 GHA iterations consumed; no scope creep.

### Auto-fixed Issues

**1. CPack ZIP included JUCE headers (3443+ entries)**
- Found during: Task 1 local verification
- Issue: `CPACK_COMPONENT_UNSPECIFIED_HIDDEN TRUE` insufficient for ZIP generator; JUCE install() rules route to default "Unspecified" component which ZIP generator didn't honor
- Fix: `CPACK_ARCHIVE_COMPONENT_INSTALL ON` + `CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE` + `CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "ExternalDeps"` before FetchContent calls
- Verification: Local cpack: 9 files, 0 JUCE headers, 16MB
- Commits: `2bb82ab`

**2. YAML block scalar terminated by blank lines**
- Found during: First GHA push (runs 27862814078, 27862867354)
- Issue: `--notes "..."` with blank lines at column 0 terminated the `run: |` block scalar; `**Install:**` parsed as YAML alias
- Fix: Write notes to `/tmp/release-notes.md` via heredoc; use `--notes-file`
- Commits: `10729c1`

**3. `secrets` context invalid in step `if:` condition**
- Found during: GHA runs 27863159048, 27863213803
- Issue: GitHub validates secrets context availability at parse time; step if: only allows `env`, `github`, `vars` contexts
- Fix: Map secret to job-level `env: APPLE_SIGNING_IDENTITY: ${{ secrets.APPLE_SIGNING_IDENTITY }}`; check `if: env.APPLE_SIGNING_IDENTITY != ''`
- Commits: `f3b946e`

**4. Windows build: `dlfcn.h` not found**
- Found during: GHA run 27863697675
- Issue: `dlfcn.h` is POSIX-only; Windows has no `dlsym(RTLD_NEXT)`
- Fix: `#ifndef _WIN32 #include <dlfcn.h> #endif`; `#ifdef _WIN32` stub (rt_alloc_count stays 0); generator expression `$<$<NOT:$<PLATFORM_ID:Windows>>:dl>` for link
- Commits: `aef31e3`

**5. macOS build: `free(void* p) noexcept` rejected by Apple Clang**
- Found during: GHA run 27864399007
- Issue: Apple SDK declares `free()` with `__attribute__((__nothrow__))` not `noexcept`; Apple Clang rejects exception spec mismatch
- Fix: Remove `noexcept` from `free()` override
- Commits: `bd533fa`

**6. CI lint: clang-format violations in preset_manager.h + rt_safety.h**
- Found during: GHA run 27864399007
- Issue: CI lint step fails on malformatted headers
- Fix: `clang-format -i` on both files
- Commits: `e0fdd41`

**7. macOS CPack: `BAQUE.component` not found**
- Found during: GHA run 27936604146
- Issue: `install(DIRECTORY)` has no OPTIONAL flag; AU not always present (no Apple credentials)
- Fix: `install(CODE "if(EXISTS ...) file(INSTALL ...)")` guard
- Commits: `db516c9`

**8. Release job: `fatal: not a git repository`**
- Found during: GHA run 27993333320 (all 3 builds passed; only release job failed)
- Issue: Release job has no `actions/checkout`; `gh release create` tried to detect repo from `.git`, failed
- Fix: Add `--repo "${{ github.repository }}"` flag
- Commits: `11f8605`

## Issues Encountered

| Issue | Resolution |
|-------|------------|
| macOS universal build ~2h cold (JUCE arm64+x86_64) | Accepted — cache hits on subsequent runs; FetchContent cache key set |
| GHA secondary rate limit on gh run watch | Switched to manual polling with gh run view |
| Old CI run blocked macOS runner for 1.5h | Cancelled stale CI run to free runner |

## Next Phase Readiness

**Ready:**
- v1.0.0 public release live: https://github.com/dobidu/baque/releases/tag/v1.0.0
- All 3 platform ZIPs downloadable and correct
- Milestone v1.0 COMPLETE — 13/13 phases done

**Concerns:**
- macOS artifacts unsigned — Gatekeeper requires right-click Open on first launch
- Node.js 20 deprecation warnings in GHA (upload-artifact@v4 / download-artifact@v4) — functional but will need @v5 upgrade post-v1

**Blockers:** None

---
*Built with PAUL Framework v1.4*
*Phase: 13-release, Plan: 02*
*Completed: 2026-06-24*
