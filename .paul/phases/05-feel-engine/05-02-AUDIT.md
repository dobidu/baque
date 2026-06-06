# Enterprise Plan Audit Report

**Plan:** .paul/phases/05-feel-engine/05-02-PLAN.md
**Audited:** 2026-06-06
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable before audit. Architecture is correct and RT-safe at the core:
xorshift32 PRNG, Box-Muller Gaussian, noexcept throughout, note-on-only humanize, prepare()
restores PRNG seed. One crash-class null dereference on the audio thread. One invisible
invariant (PRNG call order) that silently breaks all presets if changed. Both fixed. Two test
gaps added.

Would not approve as-is. Would approve after the 2 must-have + 2 strongly-recommended upgrades
applied here.

---

## 2. What Is Solid

- **xorshift32 + state=0 guard**: Correct PRNG for RT. Zero allocation. Deterministic.
  seed=0→1 guard documented and enforced.
- **prepare() restores seed_**: Correct model for transport-restart reproducibility.
  Same seed = same groove every playback.
- **Humanize note-on only**: Explicit and justified. Jitter on note-off creates wrong gate
  length. Not a gap — a deliberate, documented decision.
- **feel_engine null guard in add_with_feel lambda**: Already present in 05-01. Backward
  compatibility with `generate(transport, buf, n, sr)` (no feel params) preserved.
- **Box-Muller epsilon (+1e-7f)**: Prevents log(0) without branching. RT-safe.
- **Test helpers verified**: `all_steps_pattern()`, `single_step_pattern()`, `make_transport()`
  all exist in test_feel_engine.cpp — FE9-FE15 compile as written.

---

## 3. Enterprise Gaps Identified

### M1 — NULL DEREFERENCE on audio thread (crash-class)

**Location:** Task 2.1, velocity section:
```cpp
if (feel && feel->enabled) {
    ...
    if (feel->humanize_vel_pct > 0.0f) {
        vel = feel_engine->apply_vel_jitter(vel, feel->humanize_vel_pct);  // feel_engine unchecked
    }
}
```

**Risk:** `feel_engine` is a nullable default parameter. The add_with_feel lambda already guards
`!feel_engine`, but the velocity section does not. Calling `generate(transport, buf, n, sr, &feel,
nullptr, 0)` with `feel.humanize_vel_pct > 0` dereferences null on the audio thread → hard crash
in any DAW host, unrecoverable state. This is a regression from the 05-01 null-safe pattern.

**Required fix:** `if (feel->humanize_vel_pct > 0.0f && feel_engine) {`

### M2 — PRNG call order is an invisible invariant with no documentation

**Risk:** With both humanize flags active, each note-on advances PRNG × 4:
1. `apply_vel_jitter()` → PRNG × 2 (velocity, BEFORE add_with_feel)
2. `next_timing_jitter_samples()` inside add_with_feel → PRNG × 2 (timing, AFTER vel)

This ordering (vel-first → timing-second) is invisible to future maintainers. If swapped during
a refactor, all user-saved seeds silently produce different grooves — a reproducibility regression
with no compile error, no test failure on new code, and no warning. The plan shows the two code
blocks in isolation without stating the invariant.

**Required fix:** Document call order as an explicit invariant in the code at the site of
application, and in Task 2.1 description.

### SR1 — No test for negative Gaussian jitter clamping to block start

Box-Muller produces negative values. When negative jitter pushes `abs_target < block_start_sample`,
existing clamp-to-0 logic fires (from 05-01). FE2 covers deterministic negative offset only. No
test verifies that probabilistic negative jitter (a) doesn't drop the note, (b) doesn't crash,
(c) clamps to position 0. This path is exercised in production for every note at block start with
large stddev — it must be verified.

### SR2 — No test with BOTH timing + velocity humanize for prepare() reproducibility

FE13 (prepare() reproducibility) uses only timing humanize. FE9 (same-seed) uses only timing
humanize. The critical combined path (4 PRNG advances per note-on, vel + timing both active,
prepare() reset verified for positions AND velocities) has no dedicated test. A bug that resets
only one subsystem's state would go undetected.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | feel_engine null dereference in velocity section | Task 2.1 velocity code | Added `&& feel_engine` guard on humanize_vel_pct branch |
| M2 | PRNG call order invariant undocumented | Task 2.1 velocity code | Added `// INVARIANT (M2): vel jitter MUST be applied before add_with_feel()` comment + ordering explanation |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | No test for negative jitter clamp path | AC, Task 3.2 | Added AC-9 (Given/When/Then); added FE16 — iterates 64 seeds, verifies at least one produces clamped-to-0 note-on |
| SR2 | No test for combined humanize + prepare() reproducibility | AC, Task 3.2 | Added AC-10 (Given/When/Then); added FE17 — both flags active, prepare() reset, positions AND velocities verified identical |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Deferred note-off may fire before note-on with extreme positive jitter | Edge case at high stddev; zero-length gate is correct RT-safe fallback; not a regression from 05-01 |
| D2 | FeelPattern not thread-safe (single-writer by convention) | Inherited posture from 05-01; Phase 10 UI will upgrade to command queue or atomics |
| D3 | xorshift32 period = 2^32 − 1; not cryptographic | Adequate for audio humanize; users would never notice period at any musical tempo |

---

## 5. Audit & Compliance Readiness

**Evidence produced:** 76 tests (FE1-FE17 + 59 prior). Reproducibility (AC-1, AC-7, AC-10) and
clamp behavior (AC-2, AC-9) are machine-verified. PRNG stuck-state (AC-8) is verified.

**Silent failure prevention:** M2 invariant now documented in-code. M1 null guard prevents
crash-class failure. Both are now enforced at the code site.

**Post-incident reconstruction:** seed-based determinism means any groove can be reproduced
exactly from the preset seed. PRNG call order invariant is documented — future maintainers can
reason about ordering without reading the commit history.

**Ownership:** Single-writer contract on FeelPattern (from 05-01) is preserved. Phase 10 must
upgrade before live UI edits.

---

## 6. Final Release Bar

**Must be true before shipping:**
- M1 fixed (null guard in velocity section) ✓ applied
- M2 invariant documented ✓ applied
- 76/76 tests pass
- clang-format clean

**Remaining risks if shipped after these fixes:**
- FeelPattern thread safety (mitigated by single-writer contract; UI phase owns the fix)
- Extreme positive jitter can defer note-off after note-on → zero-length gate (documented, acceptable v1)

**Sign-off statement:** Would approve for production after M1 + M2 applied and CI green.

---

**Summary:** Applied 2 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY. Test count: 76 (9 new: FE9-FE17).

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
