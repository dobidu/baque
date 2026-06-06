# Enterprise Plan Audit Report

**Plan:** .paul/phases/06-fx-plocks/06-01-PLAN.md
**Audited:** 2026-06-06
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. Architecture is sound: PLockBatch is stack-allocated, APVTS
snapshot pattern mirrors the established FeelPattern approach, apply_plock_batch() is
bounded O(k_max). One must-have: ambiguous placement instruction would cause a
double-generate() bug if followed literally. Two strongly-recommended: PL6 doesn't
actually test AC-6 (APVTS params), and the switch default silently swallows future
PLockParam additions. All three fixed. Would approve after upgrades applied.

---

## 2. What Is Solid

- **PLockBatch stack-allocated**: no heap in audio thread, bounded at 32 events — RT-safe.
- **FxParams snapshot before generate()**: prevents mid-chain param change; consistent
  with FeelPattern precedent (passed by const pointer, not queried inline).
- **Optional params default to nullptr**: backward compat with all 84 existing tests
  preserved without changes to callers.
- **PLockPattern.enabled guard**: single boolean short-circuits before any loop — clean,
  matches FeelPattern.enabled convention.
- **PLockParam enum with count sentinel**: k_plock_param_count derived from enum —
  array sizes and loop bounds stay in sync without magic numbers.
- **apply_plock_batch() placement**: APVTS snapshot → generate() → apply_plock_batch()
  is the correct ordering: base values set from knobs, then p-lock overrides applied
  on top. This matches the expected "p-lock wins over knob" semantics.

---

## 3. Enterprise Gaps Identified

### M1 — Double-generate() trap: "insert after" placement instruction

**Location:** Task 3, processBlock() instruction
**Risk:** The instruction says "after sequencer.generate() call, **insert:**" — and then
the inserted block contains another `sequencer_.generate()` call. If followed literally:
the existing generate() runs, then the new block (containing a second generate()) runs.
Result: every step fires twice per block — double MIDI events, double p-lock events.
The note at the bottom ("Remove any existing separate calls to generate()") is a separate
sentence that APPLY could miss.
**Fix:** Change "insert" to "REPLACE the existing call with", and add a comment inside
the code block making the replacement explicit.

### SR1 — PL6 doesn't test AC-6 (APVTS FX params exist)

**Location:** Task 4, PL6 test body
**Risk:** AC-6 states "BaqueProcessor has APVTS FX params accessible". PL6 only checks
`k_plock_param_count == 6` — a static enum fact that is always true regardless of whether
the APVTS params exist. If `create_parameter_layout()` omits all 6 new params, AC-6 is
not met but PL6 still passes. The test comment says "smoke test covers it" but the smoke
test does not explicitly assert param IDs.
**Coverage reality:** processBlock() dereferences `getRawParameterValue()` without a
null-check — a missing param ID causes a crash in the smoke test. This is fragile but
real crash-on-null coverage for Phase 6-01.
**Fix:** Make PL6 honest about its test strategy. Document that AC-6 is implicitly covered
via smoke test crash semantics, and note that explicit assertions are a Phase 12 hardening
item. Update test name to match what it actually tests.

### SR2 — switch default: break in apply_plock_batch silently swallows future params

**Location:** Task 3, apply_plock_batch() implementation
**Risk:** If a new `PLockParam` enum value is added in a later phase, the `default: break`
prevents the compiler from warning about the unhandled case. The new p-lock param would be
silently ignored with no error or log. This is a common source of "why is my p-lock not
working?" bugs.
**Fix:** Add an invariant comment naming `count` as the only valid default case, and a
WARNING comment reminding maintainers to extend the switch when adding new PLockParam values.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Double-generate() trap — "insert after" → must be "replace" | Task 3 processBlock() instruction | Changed to "REPLACE the existing call"; added comment inside code block; removed ambiguous note |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | PL6 vacuous for AC-6 — APVTS params untested | Task 4 PL6 test body | Updated test name + comment to document smoke-test crash coverage strategy; noted Phase 12 hardening item |
| SR2 | switch default silently swallows new PLockParam values | Task 3 apply_plock_batch() | Added INVARIANT + WARNING comment to default: break case |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | `static constexpr` at namespace scope deprecated (clang-tidy) | Valid C++17; no compile error; `inline constexpr` preferred style, cleanup pass |

---

## 5. Audit & Compliance Readiness

**RT-safety**: PLockBatch stack-allocated, bounded loop, APVTS atomic reads — clean.
**Thread safety**: plock_pattern_ is single-writer (prepareToPlay only until Phase 10 UI).
Consistent with existing feel_pattern_ posture; same upgrade path (atomics/command queue) noted in STATE.md Decisions.
**Backward compat**: All 84 prior tests will pass — no API breaking changes (new params default to nullptr).
**Evidence gap**: AC-6 APVTS verification is implicit (crash coverage). Logged for Phase 12 hardening.

---

## 6. Final Release Bar

**Must be true before shipping:**
- M1 fixed (generate() called exactly once per processBlock) ✓ applied
- SR1/SR2 applied ✓ applied
- 92/92 tests pass
- All 84 prior tests still pass (backward compat)
- clang-format clean

**Remaining risks:**
- AC-6 APVTS param presence is crash-on-null, not assertion-on-null (Phase 12 item)
- PLockPattern is not thread-safe if UI writes it (Phase 10 must upgrade to command queue or atomic copy)

**Sign-off statement:** Would approve for production after M1+SR1+SR2 applied and CI green.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 1 item.
**Plan status:** Updated and ready for APPLY. Test count: 92 (8 new PL1-PL8).

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
