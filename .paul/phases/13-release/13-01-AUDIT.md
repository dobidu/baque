---
phase: 13-release
plan: 01
audit_type: enterprise
verdict: conditionally_acceptable_upgraded
findings_applied: 3
findings_deferred: 0
type: Audit
about: "BAQUE"
---

# Audit: Plan 13-01 — README v1.0 + NOTICE + Version Bump + CMake install()

**Verdict: Conditionally acceptable — upgraded. 1 must-have + 2 strongly-recommended applied.**

## Auditor Role
Senior principal engineer + LGPL compliance reviewer

## Executive Summary

Plan structurally sound: sequencing correct, boundaries appropriate, verify steps cover functional correctness. Three gaps found and patched: one LGPL §6 compliance hole (M1), one silent macOS packaging failure (SR1), one vacuous README verify step (SR2).

---

## What Is Solid

- `PLUGIN_VERSION` absent from `juce_add_plugin()` — JUCE inherits `${PROJECT_VERSION}` automatically. Version bump propagates correctly to plugin binary without explicit field.
- Task sequence (README → NOTICE → version → install()) correct. No dependency inversion.
- 256/256 regression gate after version bump — correct. Version metadata change can affect APVTS cache keys in rare JUCE versions.
- `install()` using hardcoded `Release` appropriate — packaging is always Release config.
- Boundaries protect LICENSE, src/, tests/ — correct scope isolation.
- Catch2 and pluginval attribution distinction (tests-only vs distributed) — legally correct.

---

## Findings

### M1 — Must-Have: LGPL §6 compliance gap in NOTICE

**Issue:** NOTICE says SoundTouch fork "maintained in a separate repository" with no URL. LGPL-2.1 §6 requires that users receiving a binary combined with an LGPL library can obtain the library source. A user receiving BAQUE v1.0 cannot locate the fork source without the URL — non-compliant at distribution time.

**Root cause:** Fork URL is not known at plan authoring time. Plan boundaries correctly defer URL to Plan 13-02, but no compliance placeholder was added to NOTICE to ensure the gap is resolved before shipping.

**Fix applied to PLAN.md:** Task 2 Step 1 NOTICE replacement text changed to include `[DISTRIBUTION_GATE: insert fork repository URL here before public v1.0 release]`. Task 2 verify checks `grep -c "DISTRIBUTION_GATE" NOTICE` → Expected: 1. Plan 13-02 must replace DISTRIBUTION_GATE with actual fork URL before creating GitHub Release.

**Risk if skipped:** LGPL-2.1 violation on first public v1.0 download.

---

### SR1 — Strongly Recommended: macOS Standalone install() targets wrong artifact name

**Issue:** Plan's Standalone install() `else()` branch uses `install(PROGRAMS .../BAQUE ...)` which catches both macOS and Linux. On macOS, JUCE Standalone produces `BAQUE.app` (a directory bundle), not `BAQUE` (a file). `install(PROGRAMS)` silently does nothing for directories — CPack in Plan 13-02 would produce a macOS installer with no Standalone.

**This plan is developed and verified on Linux (WSL2)**, so the silent failure would not be caught here. It surfaces only in Plan 13-02 macOS CPack.

**Fix applied to PLAN.md:** `else()` split into `elseif(APPLE)` + `else()`. macOS uses `install(DIRECTORY .../BAQUE.app ...)`. Linux retains `install(PROGRAMS .../BAQUE ...)`.

**Risk if skipped:** macOS installer missing Standalone — Plan 13-02 CPack failure or silent omission.

---

### SR2 — Strongly Recommended: Task 1 verify is vacuous (line count only)

**Issue:** Task 1 verify checks `wc -l ≥ 150`. A 200-line README missing the Windows install path passes this check. AC-1 requires per-platform install paths — the verify does not confirm it.

**Fix applied to PLAN.md:** Added 3 grep checks to Task 1 verify:
```bash
grep -c "Library/Audio/Plug-Ins/VST3" README.md   # macOS path
grep -c "Common Files" README.md                   # Windows path
grep -c "\.vst3" README.md                         # Linux/general .vst3 reference
```
Expected: each ≥ 1.

**Risk if skipped:** AC-1 accepted with incomplete install instructions — users on one or more platforms blocked.

---

## What Was Deferred (Zero)

No findings deferred. All gaps either applied or confirmed out of scope.

---

## Changes Applied to PLAN.md

| Finding | Location | Change |
|---------|----------|--------|
| M1 | Task 2 Step 1 (NOTICE text) | Added DISTRIBUTION_GATE placeholder for LGPL §6 fork URL |
| M1 | Task 2 verify | Added `grep -c "DISTRIBUTION_GATE" NOTICE` check |
| M1 | `<verification>` checklist | Added DISTRIBUTION_GATE check item |
| SR1 | Task 2 Step 3 (install() rules) | Split `else()` → `elseif(APPLE)` + `else()`; macOS uses `install(DIRECTORY .../BAQUE.app ...)` |
| SR2 | Task 1 verify | Added 3 grep checks for macOS/Windows/Linux VST3 install paths |

---

## Next Action

APPLY. All findings applied. Plan ready for execution.

---
*Audit completed: 2026-06-19*
*PAUL Framework v1.4 — Phase 13, Plan 01*
