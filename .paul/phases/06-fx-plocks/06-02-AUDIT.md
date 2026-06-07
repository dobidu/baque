# Enterprise Plan Audit Report

**Plan:** .paul/phases/06-fx-plocks/06-02-PLAN.md
**Audited:** 2026-06-07
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable — upgraded after applying findings.**

The plan is architecturally sound: RT-safety is maintained (no allocation in process(), noexcept throughout), the three DSP stages are correctly layered (filter → reverb → delay), and the SmoothedValue block-rate pattern is appropriate for an audio plugin. However, one test correctness bug and two undocumented behavioral invariants required remediation before APPLY.

Would I approve this for production as-is (pre-audit)? No — the FC4 test bug means reverb dry-pass behavior is untested on ch=0 and the delay pop-before-push ordering is undocumented (future maintainer risk). After applying findings: yes, conditionally, with the three deferred items tracked.

---

## 2. What Is Solid

- **RT-safety contract clearly enforced**: `jassert(is_prepared_)` + early return guards against pre-prepare process() calls. `is_prepared_ = false` default prevents silent UB. Zero allocation in process() — all buffers pre-allocated in prepare(). This is the correct JUCE DSP pattern and matches the established project constraint (zero allocation on audio thread).

- **SmoothedValue block-rate pattern is correctly applied**: `setTargetValue()` each block followed by `skip(numSamples)` gives the end-of-block smoothed value, then coefficients are updated once. This is the standard pattern for block-rate parameter smoothing in audio plugins. The 20ms ramp is adequate for p-lock transitions.

- **Delay feedback clamped at k_delay_feedback=0.45**: Feedback < 0.5 prevents self-oscillation on any input amplitude. Correct defensive choice.

- **Mono/stereo handling is correct**: Filter uses separate L/R instances with `num_channels > 1` guards. Delay pushSample(1)/popSample(1) correctly gated. Reverb uses JUCE's runtime channel check in process(). No mono-host crash risk.

- **Wire-in location correct**: FxChain::process() placed after the voice gain loop — audio is fully rendered in buffer before FX are applied. P-lock overrides (apply_plock_batch) happen before fx_chain_.process() — correct temporal ordering.

- **FxParams::sidechain_threshold explicitly ignored**: Documented in boundaries and noted in AC-5. No silent swallow risk. 06-03 explicitly owns sidechain.

---

## 3. Enterprise Gaps Identified

### G1 (Must-Have): FC4 test — buf.clear() inside channel loop wipes ch=0's impulse
The impulse-setup pattern `for (int ch = 0; ch < 2; ++ch) { buf.clear(); buf.setSample(ch, 0, 1.0f); }` calls `buf.clear()` on every iteration, wiping channel 0's impulse before the loop reaches channel 1. Result: ch=0 contains only zeros, ch=1 contains the impulse. Both FC4 blocks (reverb_mix=0 and reverb_mix=0.5) have this bug.

**Consequences:**
- reverb_mix=0 dry-only check: ch=0 trivially passes (zero in, zero out). Only ch=1 is actually tested.
- reverb_mix=0.5 tail check: ch=0 contributes no energy. Test passes by accident (ch=1 tail is non-zero) but doesn't verify stereo behavior.

**Risk class:** Test passes despite verifying half of what it claims. A future regression where ch=0 has incorrect behavior would not be caught.

### G2 (Strongly Recommended): reverb_.setParameters() called every process() block — RT behavior undocumented
`juce::dsp::Reverb::setParameters()` recalculates internal Comb and AllPass filter coefficients via `updateDamping()` on every call. Called every block regardless of whether parameters changed. This is RT-safe (no allocation, noexcept), but the behavior is non-obvious. Future maintainers might incorrectly assume this is expensive and try to "optimize" by removing it (breaking smooth automation), or assume it IS the smoothing mechanism (creating confusion with SmoothedValue).

### G3 (Strongly Recommended): pop-before-push ordering in delay loop is silent invariant
The delay loop reads the delayed sample before writing the current sample. This is correct — "read before write" preserves the correct delay relationship. But the ordering is undocumented. Reversing push/pop would introduce a 1-sample feedback error that is subtle and would not cause an immediate test failure (delay_mix tests check for "differs from input," not exact timing). A future maintainer refactoring the delay loop could silently break this invariant.

### G4 (Can Safely Defer): Hardcoded numChannels=2 in prepare() ProcessSpec
The reverb and delay line are always prepared for stereo (numChannels=2). Mono hosts are supported via JUCE's runtime channel check in `dsp::Reverb::process()` (picks mono vs stereo path based on AudioBlock channel count) and the existing `num_channels > 1` guards in the delay loop. This is functionally correct but semantically misleading — the spec says 2 channels even when the plugin may run in mono.

**Why safe to defer:** JUCE 8.x runtime behavior is well-defined; prior phases have used the same pattern; no crash risk. Address by passing actual channel count to prepare() in Phase 12 when the full plugin architecture is hardened.

### G5 (Can Safely Defer): Block-rate SmoothedValue creates per-block coefficient step changes in filter
`skip(numSamples)` returns the end-of-block value; filter coefficients are set once for the entire block. Within a block (e.g., 512 samples at 44100 Hz = ~11.6ms), the coefficient is constant — a step change occurs at the block boundary. For the 20ms ramp at 512-sample blocks, the cutoff frequency changes in 2-3 steps. Audible as a mild staircase artifact on rapid p-lock transitions.

**Why safe to defer:** Per-sample coefficient updates would require calling `setCutoffFrequency()`/`setResonance()` inside the sample loop (expensive at 44.1kHz rate). Block-rate is the standard for most plugin DSP. Address in Phase 12 if measurements confirm audible artifacts.

### G6 (Can Safely Defer): `juce::roundToInt()` on delay_time converts to integer samples — linear interpolation unused
`delay_line_` uses `DelayLineInterpolationTypes::Linear`, but the plan's delay_samples calculation rounds to integer: `juce::roundToInt(dly_t * sample_rate_)`. This quantizes delay to integer samples, making the linear interpolation redundant. At delay times ≥44ms (the minimum at 44100Hz with 0.001s minimum delay_time clamped to 44 samples), integer quantization is inaudible. Fractional delay becomes relevant for pitch/chorus effects.

**Why safe to defer:** Current use case is tempo-synced delay (0.25s default), where integer-sample precision is audible. Not a correctness issue. If delay_time is ever used for Haas-effect or chorus, enable fractional by removing the roundToInt.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | FC4 impulse setup: buf.clear() inside loop wipes ch=0 data | Task 3, FC4 test body (both blocks) | Moved `buf.clear()` outside the loop; inner loop now only sets each channel's impulse. Both FC4 sub-blocks fixed. |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | reverb_.setParameters() every-block behavior undocumented | Task 1, process() → Reverb section | Added comment block: "KNOWN PATTERN (audit SR1): setParameters() called every block even when params unchanged. RT-safe: no allocation, noexcept. Dirty-flag optimization deferred to Phase 12." |
| SR2 | pop-before-push ordering invariant silent | Task 1, process() → Delay section | Added comment: "INVARIANT (audit SR2): pop BEFORE push — read delayed sample first, then write current sample. Reversing order introduces 1-sample feedback error." |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Hardcoded numChannels=2 in prepare() | JUCE runtime checks channel count in process(); existing mono guards correct; Phase 12 hardening pass |
| D2 | Block-rate SmoothedValue → per-block step change in filter coefficients | Standard plugin pattern; per-sample update expensive; address if measurements confirm artifacts (Phase 12) |
| D3 | roundToInt() makes linear interpolation redundant | Integer-sample precision adequate for tempo-synced delay; fractional needed only for pitch/chorus effects (future scope) |

---

## 5. Audit & Compliance Readiness

**Test coverage:** 6 targeted tests (FC1-FC6) covering zero-passthrough, frequency attenuation, reverb wet/dry, delay behavior, and re-initialization safety. After M1 fix, FC4 correctly tests both stereo channels for reverb dry-only and wet behavior. Coverage is adequate for the scope of this plan.

**Silent failure prevention:** `jassert(is_prepared_)` + early return prevents silent UB if process() is called before prepare(). `juce::jlimit()` clamps all smoothed values before use — no out-of-range coefficient risk. `k_delay_feedback = 0.45f` prevents self-oscillation unconditionally.

**Post-incident reconstruction:** The SmoothedValue block-rate pattern means parameter changes are observable at ≥1 block resolution. The pop-before-push invariant comment (SR2) documents the temporal relationship for future debuggers. The "KNOWN PATTERN" comment (SR1) explains why setParameters() is called every block.

**Ownership:** Boundaries section explicitly names what is NOT in scope (sidechain → 06-03; EQ/overdrive; filter mode switching; per-lane chain). Clear scope prevents accidental expansion.

**Audit trail gap (noted, not blocking):** The FC4 bug would not have been caught by running the tests (tests pass with the bug). The audit caught it through static code analysis. Future test templates should use `buf.clear()` once before channel iteration loops.

---

## 6. Final Release Bar

**Must be true before shipping:**
- M1 fix applied (done in this audit): FC4 test correctly tests stereo impulse response for both reverb_mix=0 and reverb_mix=0.5
- SR1 and SR2 comments applied (done in this audit): behavioral invariants documented
- 98/98 tests pass (build verification in T3)
- clang-format clean on all 5 new/modified files

**Remaining risks after applying findings:**
- Block-rate filter coefficient updates (D2): audible as mild staircase on very fast p-lock sweeps. Accepted for Phase 6; watch in user testing.
- Hardcoded stereo prepare spec (D1): mono host behavior correct but semantically misleading. Low risk for v1 targets (VST3 on modern DAWs defaults to stereo).

**Sign-off statement:** With M1+SR1+SR2 applied, I would approve this plan for APPLY. The architecture is RT-safe, the test coverage is honest, and the deferred items are tracked with clear rationale.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
