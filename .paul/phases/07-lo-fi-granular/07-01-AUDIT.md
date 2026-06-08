# Enterprise Plan Audit Report

**Plan:** .paul/phases/07-lo-fi-granular/07-01-PLAN.md
**Audited:** 2026-06-07
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The plan is well-scoped for a standalone DSP class with clean RT-safety intent. One performance gap (transcendental function in the sample loop hot path) and one undefined-behavior risk (channel count not capped) are release-blocking. Both fixed. One test semantic risk (float `==` without explanation) is strongly recommended to document. After applied fixes, this plan is approved for APPLY.

---

## 2. What Is Solid

- **RT-safe guarantee explicitly stated** — no allocation in process(), all POD state. Correct for audio thread.
- **phase_ initialized to 1.0f** — first sample always advances hold immediately (not stale 0.0f). Mathematically traced and correct.
- **ZOH before bitcrush** — matches hardware ADC order (SP-1200: sample at lower rate, then quantize). Correctly specified.
- **Preset constants are mathematically documented** — SP-1200 = 26.04kHz → factor 44100/26040 ≈ 1.694x. Traceable to hardware specs.
- **LF2 exact value derivation** — 0.3f at 8-bit → round(0.3 × 128) / 128 = 38/128 = 0.296875 exactly. Mathematically verified; 0.296875 is exactly representable in IEEE754.
- **LF3 traces correctly** — phase_=1.0f, sr_factor=2.0: outputs [~0.1, ~0.1, ~0.3, ~0.3] verified by manual trace.
- **Boundaries clearly scope FxChain/vinyl/granular out** — no scope creep risk.
- **ASCII-only test names** — Phase 1 constraint remembered.

---

## 3. Enterprise Gaps Identified

### Gap 1 (M1): `std::pow` in per-sample hot path
The plan spec places `float levels = std::pow(2.0f, bit_depth - 1.0f)` inside the "Per-sample" description. For a 512-sample block: 512+ transcendental evaluations per process() call. `std::pow` is not O(1); latency is implementation-dependent and unpredictable. This violates RT-safe predictability requirements for the audio thread. `bit_depth` is constant within a process() call — `levels` must be hoisted before the sample loop.

### Gap 2 (SR1): Channel count not capped — UB on mono/multi-channel buffers
The plan says "loop over actual channels" but names `held_l_` and `held_r_`, implying exactly 2. If a 1-channel (mono) buffer is passed, the loop would attempt to write to channel index 1 which doesn't exist in the JUCE buffer — `buffer.getSample(1, i)` on a 1-channel AudioBuffer is undefined behavior (JUCE uses `jassert(channelNumber < getNumChannels())`). Must explicitly cap: `const int num_ch = std::min(buffer.getNumChannels(), 2)`.

### Gap 3 (SR2): LF3 exact `==` is correct but undocumented — maintenance hazard
`REQUIRE(buf.getSample(0, 0) == buf.getSample(0, 1))` uses exact float equality. This is IEEE754 correct (both samples execute `round(held_l_ * levels) / levels` with identical inputs → identical bit result). However, without documentation, a future maintainer will "correct" this to `Approx()` on the assumption that float `==` is always wrong. The `Approx` version would then allow held_ state to drift between consecutive held samples, silently invalidating the test's semantic.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | `std::pow` in per-sample loop — transcendental, not O(1), violates RT predictability | T1 action — process() spec | Hoisted `const float levels = std::pow(2.0f, bit_depth - 1.0f)` BEFORE sample loop. Per-sample BitCrush step now uses precomputed `levels`. |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Channel count ambiguous — no cap at 2, UB on mono | T1 action — process() spec | Added `const int num_ch = std::min(buffer.getNumChannels(), 2)`. Mono: only held_l_ active. Header comment required. |
| 2 | LF3 exact `==` non-obvious — maintenance hazard | T1 action — LF3 test spec | Added explicit comment block explaining why `==` (not `Approx`) is correct: same held_ + same levels → IEEE754 identical. |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | k_sp303 has no dedicated test | Behavior covered: LF2 tests 12-bit-equivalent quantization (AC-2), SR1 covers no-reduction path. k_sp303={12,1.0} is a subset of tested behaviors. |
| 2 | k_8bit has no dedicated test | Covered by LF2 (8-bit quantization) + LF3 (sr_factor=2.0 ZOH). Combined behavior is tested. Explicit k_8bit preset test deferred to 07-02 FxChain integration. |
| 3 | Boundary values for bit_depth (0, <0, >32) and sr_factor (<1.0) untested | Presets enforce sensible values. Edge cases are caller responsibility. Deferred to 07-02 where FxParams will enforce valid range. |

---

## 5. Audit & Compliance Readiness

- **Silent failure prevention**: LF3 `==` fix (SR2) prevents tests that pass vacuously after a semantic regression. LF2 uses Approx correctly (exact value but tolerance allows float rounding path to match).
- **Post-incident reconstruction**: `levels` hoisted (M1) makes process() deterministic and measurable. RT profiling will show predictable per-block cost.
- **Ownership**: Plan boundaries clearly exclude FxChain changes — scope is unambiguous.
- **Channel UB**: SR1 fix prevents silent corruption or assertion failure in debug mode when mono audio passes through.

---

## 6. Final Release Bar

**Must be true before shipping:**
- `levels` computed once per process() call, not per sample
- Channel index never exceeds `min(num_channels, 2) - 1`
- LF1–LF5 all pass, 114/114 total suite passes
- clang-format clean on all 3 new files

**Remaining risk after fixes:**
- sr_factor < 1.0 behavior is undefined (phase accumulates, `if` never triggers after first sample). Acceptable since no preset uses sr_factor < 1.0 and caller contract prevents it.

**Sign-off:** I would approve this plan for APPLY with the three fixes applied above.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
