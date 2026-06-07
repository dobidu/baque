# Enterprise Plan Audit Report

**Plan:** `.paul/phases/06-fx-plocks/06-03-PLAN.md`
**Audited:** 2026-06-07T23:00:00Z
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable — approved after applying 1 must-have + 2 strongly-recommended upgrades.**

The DSP design is architecturally correct: pre-computed IIR coefficients, per-sample asymmetric
envelope follower, hard-knee gain computer, RT-safe (zero allocation in process()). The plan's
scope, boundaries, and chain ordering are well-reasoned. However, two tests had integrity problems
(SC3 threshold too loose, SC5 vacuous pass) and one silent-failure risk was unguarded (NaN
propagation from threshold_db). All three are fixed and the plan is now enterprise-ready.

---

## 2. What Is Solid (Do Not Change)

**IIR envelope follower design**: First-order lowpass is the correct mechanism for pump
compression. Per-sample IIR with asymmetric attack/release (5ms/200ms) produces the characteristic
slow swell. Using `SmoothedValue` (block-rate) would be wrong; the IIR approach is exactly right.
Pre-computing coefficients in `prepare()` — not per-block, not per-sample — is correct.

**Chain position (LAST in FxChain)**: Reverb tail and delay echoes are compressed together with
the dry signal. This creates the "ducking reverb" effect characteristic of UK Garage and deep house
pumping bass. The architectural reasoning is explicitly stated and correct.

**jassert + early return guard**: Consistent with all other BAQUE DSP classes. No UB on misuse.

**Scope limits**: No make-up gain, no lookahead, no soft knee, no cross-lane sidechain. Each
deferral is justified by phase scope. The "no juce::dsp::Compressor" note is important — the JUCE
compressor uses a different envelope model that would not produce the pump characteristic.

**SC1, SC2, SC4, SC6**: All mathematically correct with honest margins. SC4 especially strong —
the carry-over compression test directly verifies the pump mechanism that is the plan's stated goal.

**Boundaries section**: Correctly protects FxParams struct, APVTS IDs, and FxChain public
interface. No regression risk from 06-01 or 06-02 work.

---

## 3. Enterprise Gaps Identified

**G1 → M1: SC3 test threshold too loose (test correctness failure)**
- AC-3 claims ">6 dB gain reduction at 8:1 ratio" but threshold `< 0.5f` is the exact output for
  a 2:1 ratio compressor (gain = 0.5 at boundary). A 2:1 compressor would produce output ≈ 0.5,
  which barely fails `< 0.5f`. A broken compressor with inadvertent 2:1 behavior would be caught
  only at the tolerance boundary.
- Math: 8:1 at 12 dB above threshold → gain_db = 12 × (0.125 − 1) = −10.5 dB → gain ≈ 0.299.
  Threshold `< 0.4f` is comfortable for 4:1 (0.355) and 8:1 (0.299), fails for ≤2:1 (0.5 ≥ 0.4).

**G2 → SR1: SC5 vacuous pass (test cannot fail)**
- IIR envelope: `env_[n] = coeff × env_[n-1] + (1−coeff) × key[n]`. Both terms are non-negative
  and sum to a weighted average. For key[n] ≤ 1.0 (unit-amplitude sine) and env_[0] = 0:
  by induction env_[n] ≤ 1.0 for all n.
- `threshold_linear = decibelsToGain(0.0f) = 1.0`. Condition `env_ > 1.0` is always false.
- The compression branch is never entered. The test passes even if all threshold logic is deleted.
- Precedent: same class of vacuous-pass was flagged in 05-03 (FP2) and fixed then.

**G3 → SR2: NaN propagation from unguarded threshold_db**
- `process(buffer, float threshold_db)` receives raw float from `params.sidechain_threshold`.
- `juce::Decibels::decibelsToGain(NaN)` returns NaN. `left[i] *= NaN` silently corrupts entire
  audio buffer. No jassert, no clamp, no early return catches this.
- APVTS normally clamps parameter values, but defense-in-depth matters for RT code where silent
  corruption is harder to debug than an assertion.
- Fix is RT-safe: `jlimit(-80.0f, 0.0f, threshold_db)` before decibelsToGain. Zero cost in the
  hot path (single float clamp).

**G4 → defer: Mono channel path untested**
- When `num_channels == 1`, `right = nullptr`, key = |left[i]| only, only `left[i]` modified.
- Code is correct by inspection. Primary BAQUE use case is stereo. Mono test deferred.

**G5 → defer: `gainToDecibels` floor behavior undocumented**
- `gainToDecibels(near-zero)` returns −100 dB floor by default. Irrelevant here since the
  compression branch only executes when `env_ > threshold_linear > 0`, so env_ is always > 0
  when gainToDecibels is called. No practical risk.

**G6 → defer: No exact ratio verification**
- Tests verify "compression IS happening" and "strong compression" (after M1 fix) but not "ratio
  is exactly 8.0f." Computing expected gain from formula and comparing to actual is over-testing
  for Phase 6 scope; ratio verification belongs in a dedicated DSP unit test suite (Phase 12).

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | SC3 threshold `< 0.5f` passes 2:1 ratio compression — test verifies "some compression" not "strong compression" | AC-3 + Task 3 SC3 test | AC-3: "50% → 40%" with math rationale. SC3: `< 0.5f` → `< 0.4f` with audit comment |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | SC5 vacuous pass — `threshold_linear=1.0` never exceeded by IIR envelope tracking unit-amplitude signal; test proves nothing | AC-5 + Task 3 SC5 test | AC-5 rewritten as contrast criterion (amplitude=0.5 is above -12dBFS threshold, below 0dBFS). SC5 replaced with contrast lambda test. Includes math comment proving threshold values were chosen correctly |
| SR2 | NaN/extreme `threshold_db` silently corrupts audio buffer via `decibelsToGain(NaN)` | Task 1 process() code | Added `jlimit(-80.0f, 0.0f, threshold_db)` before `decibelsToGain()` call. PT comment documents audit rationale |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|------------------------|
| D1 | Mono channel path (num_channels==1) untested | Primary BAQUE use case is stereo; mono path is correct by inspection; Phase 12 can add |
| D2 | `gainToDecibels` floor at near-zero env_ | Only called when `env_ > threshold_linear > 0`; floor never reached in practice |
| D3 | Exact ratio (8.0:1) verification test | Over-testing for Phase 6; belongs in Phase 12 DSP regression harness |
| D4 | Sample rate change without re-prepare (stale coefficients) | JUCE/host contract: `releaseResources → prepareToPlay` before sample rate changes; jassert on is_prepared_ covers misuse |

---

## 5. Audit & Compliance Readiness

**Determinism:** `env_` is the sole state. Fresh `prepare()` + same input → same output. Tests
initialize fresh compressor → deterministic results. Post-incident reconstruction possible by
replaying audio with known env_ initial state.

**Silent failure prevention:** SR2 (jlimit guard) prevents NaN buffer corruption, which is
the most dangerous silent failure in RT audio code (produces audible distortion with no error log).

**RT safety verification:** By inspection — no allocation in `process()` (only scalar arithmetic on
stack). No locks. No IO. No system calls. `jlimit` and `decibelsToGain` are stack-only. Consistent
with PROJECT.md hard constraint "zero allocation / locks / IO on audio thread."

**Audit evidence:** 6 tests named SC1-SC6 map directly to 6 acceptance criteria. The
`[sidechain]` tag enables targeted CI: `baque_tests "[sidechain]"`. After SR1 fix, all tests are
honest (no vacuous passes).

---

## 6. Final Release Bar

**What must be true before this plan ships:**
- M1 applied: SC3 uses `< 0.4f`
- SR1 applied: SC5 is a contrast test, not a vacuous pass
- SR2 applied: jlimit guard before decibelsToGain
- 104/104 tests pass; FC1-FC6 regression check clean; clang-format clean

**Remaining risks after upgrades:**
- No exact ratio=8.0 test (acceptable for Phase 6; flagged in D3)
- No mono channel test (acceptable; code correct by inspection)
- No make-up gain (by design — scope limit; compression-only for Phase 6)

**Sign-off:** After applying the 3 upgrades above, I would approve this plan for production.
The DSP model is correct, the test suite is honest, the RT-safety constraints are respected.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 4 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
