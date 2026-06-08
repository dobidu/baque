# Enterprise Plan Audit Report

**Plan:** .paul/phases/07-lo-fi-granular/07-02-PLAN.md
**Audited:** 2026-06-07
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. Plan is well-specified with clean architectural integration. One task-verification gap (PL6 regression not caught by T1 verify) is release-blocking. Two documentation gaps (lo-fi params not smoothed, apply_plock_batch test limitation) are strongly recommended. After applied fixes, plan is approved for APPLY.

---

## 2. What Is Solid

- **LoFiProcessor as FIRST stage** — correct. Matches hardware ADC: lo-fi coloration happens at capture, before FX processing. Order cannot be reversed without breaking hardware fidelity.
- **Append-only PLockParam** — bit_depth=6, sr_factor=7 appended after existing values (0–5). No existing p-lock event routing affected.
- **Transparent defaults** — bit_depth=16.0f, sr_factor=1.0f: existing audio unaffected without explicit FxParams change. Backward-compatible.
- **Separate FxChain instances per test** — pattern from Phase 6 audit SR1 correctly carried forward.
- **sidechain_threshold=0.0f in tests** — established pattern for disabling compression without state interference.
- **PL6 update mandated** — plan explicitly requires updating count 6→8. Would fail at test suite if missed.
- **max_block_size_ stored for reset()** — LoFiProcessor::prepare() ignores the value (commented out params), so the default 512 is harmless if reset() is called before prepare().
- **LC2/LC3 use fresh instances** — no filter state carryover between 8-bit and 16-bit comparison runs.
- **Boundaries protect existing FxChain stage order** — no risk of LP→reverb→delay→sidechain reordering.

---

## 3. Enterprise Gaps Identified

### Gap 1 (M1): T1 verify only runs LC tests — PL6 regression uncaught until T2
T1 task verify command: `ctest -R "LC"`. When PLockParam::count changes from 6 to 8, `test_plock.cpp` PL6 asserts `k_plock_param_count == 6` which will FAIL. But T1 verify only runs LC tests. A developer completing T1 would see "LC tests pass" = DONE, then discover PL6 failure in T2 after full suite. This breaks the E/Q loop: T1 verify should catch the regression it caused. Must add PL6 to T1 verify.

### Gap 2 (SR1): bit_depth/sr_factor applied raw — non-obvious, maintenance hazard
`lo_fi_.process(buffer, params.bit_depth, params.sr_factor)` uses params fields directly (no SmoothedValue). This is correct: lo-fi is a discrete-switch effect (instantaneous preset change matches hardware behavior). However, without a comment, a future maintainer will "fix" this by adding SmoothedValues for bit_depth/sr_factor — which would incorrectly crossfade between lo-fi states, creating a smooth transition that sounds nothing like switching a hardware ADC. Must document in fx_chain.cpp comment.

### Gap 3 (SR2): LC4 test limitation undocumented
LC4 verifies PLockParam enum indices and FxParams field assignment. It CANNOT test `apply_plock_batch()` switch dispatch because `BaqueProcessor` requires full JUCE plugin infrastructure (not instantiable in unit tests). Without documentation, future maintainers will assume "LC4 covers p-lock dispatch" when it doesn't. The dispatch IS implicitly verified by smoke test crash behavior (same pattern as PL6 audit-SR1 from Phase 6-01), but this must be stated explicitly in the test.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | T1 verify misses PL6 regression (count 6→8) | T1 `<verify>` | Changed `ctest -R "LC"` to `ctest -R "(LC\|PL6)"` — catches both new LC tests and PL6 count regression in the same T1 verify pass |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | bit_depth/sr_factor not smoothed — maintenance hazard | T1 action — fx_chain.cpp spec | Added comment requirement: document no-SmoothedValue rationale inline (discrete switch, not crossfade — matches hardware ADC behavior) |
| 2 | LC4 test limitation undocumented | T1 action — LC4 test spec | Added NOTE comment requirement: apply_plock_batch() untestable from unit tests; dispatch verified by smoke test crash behavior (audit-SR1 pattern) |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | SmoothedValue for bit_depth/sr_factor — no transition ramp on p-lock changes | Discrete-switch behavior is correct for lo-fi. If smooth transition is ever desired, add crossfader wrapper in 07-02+ or Phase 10. |
| 2 | jlimit guards for bit_depth [4,24] and sr_factor [1.0,4.0] | Presets enforce sensible values. UI sliders in Phase 10 will enforce valid range at entry point. |
| 3 | apply_plock_batch() dispatch coverage — BaqueProcessor not testable from unit tests | Architectural limitation accepted (same as Phase 6-01 PL6 pattern). Smoke test verifies no crash. |

---

## 5. Audit & Compliance Readiness

- **Regression detection**: M1 fix ensures PLockParam count change is caught in T1 verify, not deferred to T2. Prevents false "DONE" completion.
- **Documentation**: SR1 comment prevents future misguided "fix" that would corrupt lo-fi behavior. SR2 comment prevents false assumption about dispatch coverage.
- **Backward compatibility**: Default FxParams values (16.0f, 1.0f) are transparent — no existing audio affected without explicit param change.
- **No silent failures**: PL6 test is a hard count check; any future PLockParam addition without a case in apply_plock_batch will fail PL6 before shipping.

---

## 6. Final Release Bar

**Must be true before shipping:**
- T1 verify passes with `ctest -R "(LC|PL6)"` — no PL6 regression
- LC1–LC5 all pass; 119/119 total suite
- fx_chain.cpp has no-SmoothedValue comment for lo-fi stage
- LC4 test has note about apply_plock_batch dispatch limitation
- clang-format clean on all 6+ modified files

**Remaining risk after fixes:**
- Instantaneous bit_depth/sr_factor transitions may cause brief audible artifact on p-lock step boundary (no ramp). Acceptable for Phase 7; addressable in Phase 10 if needed.

**Sign-off:** I would approve this plan for APPLY with the three fixes applied above.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
