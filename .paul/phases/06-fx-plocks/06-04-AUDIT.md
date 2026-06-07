# Enterprise Plan Audit Report

**Plan:** .paul/phases/06-fx-plocks/06-04-PLAN.md
**Audited:** 2026-06-07
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable — upgraded after audit.**

The plan's architecture is sound: testing at FxChain level (not BaqueProcessor) is the correct abstraction boundary, and the subsystem-level tests (PD1, PD2, PD4) follow established patterns from prior audits. However, PD3 contained a critical vacuous-pass defect, and both PD1 and PD2 had a state-contamination risk across multiple runs on the same instance. These were applied before APPLY.

With the three applied upgrades, the plan is enterprise-ready for execution.

---

## 2. What Is Solid

- **PD1 spectral measurement window is correct.** 1024-sample skip accounts for the 882-sample SmoothedValue convergence. Measuring tail RMS after convergence is the right approach for a filter attenuation test.
- **PD2 contrast test follows validated SC5 pattern.** 88200-sample (2s) buffers with 8192-sample IIR convergence skip is the established pattern from 06-03 audit — correct approach.
- **PD4 boundary coverage is appropriate.** min/max FxParams values with isfinite() covers the NaN/Inf guard installed in 06-01 (jlimit on threshold_db). Correct defensive test.
- **PD5 DoD marker with [dod] tag follows Phase 5 pattern.** Named marker tests with targeted CI tag is an established convention.
- **No BaqueProcessor in unit tests.** Correct constraint — BaqueProcessor requires full JUCE plugin infrastructure; FxChain-level testing is the right isolation.
- **ASCII-only test names.** Consistent with Phase 1 decision; prevents Windows ctest PRE_TEST mangling.

---

## 3. Enterprise Gaps Identified

### Gap 1 — PD3 Vacuous Pass (Critical)
`sum > 0.0f after sample 100` is trivially satisfied for any non-silent signal. A 0.5f impulse passes through LP filter at cutoff=20kHz unchanged. Sum of output after sample 100 would be > 0.0f regardless of reverb_mix and delay_mix — even if both were hardcoded to 0.0f. A completely broken FxChain that ignores reverb and delay entirely would pass this test. Identical failure mode to SC5 in 06-03 audit (SR1 applied there).

Secondary issue: measurement "after sample 100" is before the 882-sample SmoothedValue ramp completes; wet mix is only ~11% of target at that point. First delay echo (delay_time=50ms) arrives at sample 2205 — no echo verification is possible from samples 0–2205.

### Gap 2 — FxChain State Cross-Contamination Across Runs
PD1 and PD2 specify two processing runs (low/high cutoff; tight/loose threshold) without constraining whether the same FxChain instance is reused.

- **PD1:** StateVariableTPTFilter internal state carries over from low-cutoff run. Mitigated by 1024-sample measurement skip, but not specified.
- **PD2:** SidechainCompressor IIR envelope begins charged at near-signal amplitude after the tight-threshold run (-3dBFS). Starting the loose-threshold run (-60dBFS) on the same charged instance means the envelope starts high and decays during the measurement window, inflating apparent compression in the loose run. This could cause `peak_tight < peak_loose * 0.7f` to fail if the envelope hasn't decayed far enough by sample 8192.

### Gap 3 — PD4 Silent-Output False Pass
`std::isfinite(sample) == true` catches NaN/Inf but not silent output. 0.0f is a valid finite value. If FxChain::process() has a regression that zeroes the buffer (e.g., gain applied as 0.0f, or buffer.clear() accidentally retained), all samples are 0.0f — isfinite passes, but the test is a false pass.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | PD3 vacuous-pass: "sum > 0.0f" trivially satisfied regardless of reverb/delay | T1 action (PD3) + AC-3 | Replaced with contrast test: separate chain_dry and chain_wet instances; measure delay echo window [2205, 4096]; assert energy_wet > energy_dry × 2.0f |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | FxChain state contamination across multi-run tests | T1 action (PD1, PD2, PD3) + test infra note | Added requirement: each test uses SEPARATE FxChain instances per run; named instances (chain_lo/chain_hi, chain_tight/chain_loose, chain_dry/chain_wet) |
| SR2 | PD4 silent-output false-pass via isfinite-only check | T1 action (PD4) | Added: max-boundary run uses 440Hz sine (below sidechain threshold); additional assertion max(|output|) > 0.001f; rationale documented inline |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | NaN/Inf input propagation (FxParams or input buffer contains NaN) | Not a FxChain contract boundary; input validation belongs at APVTS boundary (already tested via jlimit guards in 06-01/06-03); Phase 12 Hardening |
| D2 | process()-before-prepare() guard test | FxChain::process() already has `if (!is_prepared_) return;` guard; testing this edge case is defensive and outside Phase 6 DoD scope |
| D3 | Multi-sample-rate FxChain test (prepare at 44100 then 48000) | SidechainCompressor prepare/reset/prepare tested in SC6; FxChain-level multi-SR is Phase 12 Hardening scope |

---

## 5. Audit & Compliance Readiness

**PD1, PD2:** After SR1 applied (separate instances), both tests produce defensible evidence. Two independent FxChain instances processed with different parameter values producing measurably different outputs is a clean, repeatable assertion with no cross-run contamination.

**PD3:** After M1 applied (contrast test with delay echo window), the test is falsifiable and distinguishes working reverb/delay from broken one. Delay echo window [2205, 4096] is physically motivated (50ms delay = 2205 samples at 44100Hz).

**PD4:** After SR2 applied, boundary test covers both NaN/Inf (via isfinite) and silent-output regression (via max amplitude check). 440Hz sine input for max-boundary run is below 0dBFS sidechain threshold; compression should not trigger, ensuring signal passes through.

**PD5 DoD marker:** References PD1-PD4 and SC4 by name. Provides traceability to Phase 6 DoD requirements ("P-lock automates any param per step; sidechain pump functional"). Adequate for audit purposes.

**Overall:** Test evidence is physically motivated, falsifiable, and covers the critical integration path (FxParams override → FxChain DSP output observable change). ROADMAP.md + STATE.md updates in T2 provide formal phase closure documentation.

---

## 6. Final Release Bar

**Must be true before shipping:**
- PD3 contrast test passes with energy_wet > energy_dry × 2.0f in delay echo window
- PD1 and PD2 use separate FxChain instances — no state contamination
- PD4 max-boundary run produces non-silent output (max > 0.001f)
- All 109 tests pass (104 baseline + 5 new PD1-PD5)
- clang-format clean on new test file

**Remaining risks if shipped without upgrades:**
- PD3 as originally written would pass on a broken FxChain, providing false confidence in reverb/delay integration
- PD2 with reused instance could fail intermittently depending on IIR envelope state at measurement start

**Signed off:** Conditionally acceptable → upgraded. Plan is executable after these three changes.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
