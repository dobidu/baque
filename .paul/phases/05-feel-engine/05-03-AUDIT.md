# Enterprise Plan Audit Report

**Plan:** .paul/phases/05-feel-engine/05-03-PLAN.md
**Audited:** 2026-06-06
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. Architecture is correct and minimal: pure data, static factory
functions, no allocation, RT-safe. One compile error (missing `#include <array>`). Two test gaps
that allow DoD tests to pass vacuously if the humanize pipeline silently regresses. All three
fixed. Would approve after upgrades applied.

---

## 2. What Is Solid

- **Static factory functions, FeelPattern by value**: No allocation, trivially copyable, RT-safe
  to call from any thread. Correct layering.
- **`[[nodiscard]]` on all factories**: Discarded presets are a footgun (silent misconfiguration).
  Guard is correct.
- **Data-level DoD verification (FP3/FP5)**: Cleanly separates data correctness (preset has the
  right timing values) from pipeline correctness (FE9-FE17 already cover the pipeline).
- **Preset data pre-verified in plan notes**: avg|timing|=23.75ms for Dilla, range=160ms for
  Burial, Bonobo < Burial ordering — all checked mathematically before APPLY. Prevents
  silent data errors.
- **Non-zero seeds by design**: 808, 313, 666, 42, 2024 all ≥ 1. xorshift32 stuck-state
  invariant satisfied without runtime guard.

---

## 3. Enterprise Gaps Identified

### M1 — Compile error on MSVC: missing `#include <array>`

**Location:** Task 5, test_feel_presets.cpp includes
**Risk:** FP7 uses `std::array<FeelPattern, 6>`. Test file includes `<algorithm>`, `<cmath>`,
`<vector>` but NOT `<array>`. GCC/Clang sometimes provide `std::array` transitively; MSVC does
not. Project has Windows CI (Phase 1). This is a guaranteed build failure on Windows CI.
**Fix:** Add `#include <array>` to test includes.

### SR1 — FP2 passes vacuously if generate() produces zero note-ons

**Location:** Task 5, FP2 test body
**Risk:** FP2 verifies `getVelocity() == 100` for all note-ons. If generate() produces zero
note-ons (e.g., misuse of `fp_all_steps()` or pattern bug), the for-loop body never executes
and the test passes. No note-on was actually verified.
**Fix:** Add `bool found = false` + `found = true` in loop + `REQUIRE(found)` after loop.

### SR2 — FP4 and FP6 (DoD reproducibility) pass vacuously if humanize pipeline is silently disabled

**Location:** Task 5, FP4 and FP6 test bodies
**Risk:** FP4 verifies run1 == run2 with same seed. If `humanize_timing_ms` branch were silently
disabled (regression in sequencer.cpp, e.g. condition changed from `> 0.0f` to `> 100.0f`),
both runs produce identical deterministic-only positions. The test passes but the DoD guarantee
is hollow — "Dilla Drunk" and "Burial Broken" are not actually humanized.
FP3/FP5 only check the PRESET DATA; they would still pass even if the pipeline were broken.
**Fix:** After the same-seed assertion, also run a deterministic-only baseline (same preset,
humanize_timing_ms=0) and verify at least one position from the humanized run differs. This
ensures the humanize pipeline actually ran.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | `#include <array>` missing; compile error on MSVC | Task 5 includes | Added `#include <array>` with explanatory comment |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | FP2 vacuous pass if no note-ons | Task 5 FP2 | Added `bool found` guard + `REQUIRE(found)` after loop |
| SR2 | FP4/FP6 DoD reproducibility vacuous if humanize pipeline disabled | AC, Task 5 FP4/FP6 | Added AC-9; extended FP4 and FP6 with deterministic-only baseline comparison |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Integer `0` literals in float constexpr arrays (implicit conversion) | Valid C++; no compile error; clang-tidy warning only |
| D2 | `REQUIRE(FeelPattern::k_steps == 16)` in FP7 is a dead assertion (compile-time constant) | Never fails; harmless; remove in cleanup pass |
| D3 | Plan doesn't explain why Straight=baseline in prose | Future maintainability note only; analysis shows equivalence holds |

---

## 5. Audit & Compliance Readiness

**Evidence produced:** 84 tests (FP1-FP8 + 76 prior). Phase 5 DoD explicitly tagged `[dod]`
in Catch2 test names — CI can run `ctest -L dod` to verify DoD separately from regression suite.

**Silent failure prevention:** SR2 extension ensures DoD reproducibility tests cannot pass
on a broken humanize pipeline. M1 prevents silent CI skip on Windows.

**Post-incident reconstruction:** Preset data is static constexpr — any future preset change
is visible in diff. Seed values are musical constants (808, 313, 666, 42, 2024) memorable to
maintainers.

**Ownership:** Preset data is artistic; changes must be treated as product decisions, not
refactors. The `static constexpr` keyword signals immutability intent.

---

## 6. Final Release Bar

**Must be true before shipping:**
- M1 fixed (`#include <array>`) ✓ applied
- SR1/SR2 applied ✓ applied
- 84/84 tests pass (including DoD tests FP3-FP6)
- clang-format clean

**Remaining risks:**
- Preset data is artistic/subjective — "perceptible" is not formally defined beyond the
  data thresholds (avg > 20ms, range > 100ms). Acceptable for this phase.
- No regression test for preset immutability — if someone changes timing values, no test
  catches it unless thresholds are violated. Acceptable v1 posture.

**Sign-off statement:** Would approve for production after M1+SR1+SR2 applied and CI green.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY. Test count: 84 (8 new: FP1-FP8).

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
