# Enterprise Plan Audit Report

**Plan:** .paul/phases/07-lo-fi-granular/07-03-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The granular DSP architecture is correct: power-of-2 capture buffer, pre-allocated grain pool, Hann window, linear interpolation, zero RT allocation in process(). However, GR3 and GR4 as written are structurally flawed — they will fail vacuously because the capture buffer fill phase is not specified in code. Without explicit fill code, grains read from zero-initialized capture memory and both instances output zeros → max_diff=0 → test passes accidentally or not at all depending on assertion direction. This is a release-blocking defect in the test specification. Applied M1 fix. Two SR findings applied for defensive invariants. Upgraded to acceptable.

---

## 2. What Is Solid

- **k_capture_size = 8192 power-of-2**: branchless wrap with `& (k_capture_size - 1)` in both write and lerp_buf. RT-safe, no branch misprediction on wrap boundary.
- **Grain[k_num_grains] fixed member array**: zero allocation in process(). Correct for audio thread.
- **grain_timer_ = 0 in reset()**: first grain spawns at first sample of first process() call. GR1 can detect non-zero output immediately.
- **spray=0 → scatter=0 math**: `scatter = (rng.nextFloat() - 0.5f) * 2.0f * spray_samples` where spray_samples=0 → scatter=0 regardless of RNG state. Deterministic baseline for comparison tests.
- **pitch_spread=0 → rate=1.0 math**: `rate = 1.0f + (rng.nextFloat() - 0.5f) * 0.0f = 1.0f`. Deterministic.
- **No juce::dsp dependency**: GranularProcessor uses only juce_audio_basics + cmath. No ScopedJuceInitialiser_GUI required in tests. Correct.
- **GR2 freeze test structure**: fills capture buffer with 4096 samples first, then activates freeze + silent input. Grains active at freeze time continue replaying from frozen capture. Valid test of freeze semantics.
- **Single-correction wrap in spawn_grain**: max scatter = ±(8192 * 0.25) = ±2048. Min possible read_pos before correction = 0 - 2048 = -2048. One `+= k_capture_size` → 6144. Valid. Single correction is sufficient.

---

## 3. Enterprise Gaps Identified

### Gap 1 — GR3/GR4 fill phase missing from code spec (Release-Blocking)
GR3 and GR4 say "fill both with identical 4096-sample 440Hz input" but the task action only defines 512-sample measurement buffers with no prior fill code. Execution path: grain_timer_=0, grain spawns at sample 0, base_pos = (0 - grain_len_ + k_capture_size) & mask = 5987. capture_l_[5987] = 0 (never written). lerp_buf returns 0. Both instances A and B output zeros. max_diff = 0. Test fails to distinguish spray=0 from spray=0.5. This is a vacuous pass or explicit failure depending on assertion direction. The fix requires explicit `AudioBuffer fill_a; fill_sine; gp_a.process(fill_a, 0, 0, false)` before the measurement phase.

### Gap 2 — hann() divides by `length` without guard
`0.5f * (1.0f - cos(2π*pos/length))` — if length=0, undefined behavior. Practically unreachable at valid sample rates (grain_len_ = min(4096, int(sr*0.05)) ≥ 1 for sr ≥ 20Hz). However, project pattern established in Phase 6-03 audit requires jassert for any function that divides by a variable that could theoretically be zero. Missing.

### Gap 3 — "all grains busy → skip spawn" undocumented
spawn_grain() silently drops spawns when all 16 slots are active. No comment explains this is intentional. Risk: future maintainer reads "silently returns" as a bug and adds retry logic (RT allocation), dynamic growth (heap), or panics. None of these are acceptable on the audio thread. Must document the intentional contract.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | GR3/GR4 fill phase unspecified — capture buffer stays zero → test vacuously fails | T1 task action: GR3 and GR4 test code spec | Added explicit FILL PHASE for both tests: `juce::AudioBuffer fill_a(2,4096); fill_sine(fill_a,...); gp_a.process(fill_a,0,0,false)` before measurement. Separate buffers for each instance (process() modifies in place). <!-- audit-added: M1 --> |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | hann() divides by `length` with no guard | T1 task action: hann() implementation spec | Added `jassert(length > 0)` at top of hann(). Matches Phase 6-03 jlimit/jassert pattern. <!-- audit-added: SR1 --> |
| 2 | spawn_grain() silent skip undocumented — risk of future RT-alloc "fix" | T1 task action: spawn_grain() implementation spec | Replaced terse comment with explicit: "All grain slots busy — drop this spawn. Not a bug: pool is pre-allocated; adding retry or dynamic allocation violates zero-alloc audio-thread contract." <!-- audit-added: SR2 --> |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|------------------------|
| 1 | Rate clamp for pitch_spread > 1.0 (negative rate → reverse playback unintended) | UI in Phase 10 will enforce pitch_spread ∈ [0, 1]. No user-facing control exists yet. rate ∈ [0.5, 1.5] for valid inputs. |
| 2 | juce::Random deterministic seeding for test reproducibility | spray=0/pitch=0 paths are fully deterministic (RNG output zeroed by multiply). spray=0.5/pitch=0.5 paths always differ sufficiently (~1024-sample scatter). Practical risk: zero. |
| 3 | Output gain normalization (N overlapping grains with Hann windows can sum >1.0) | Not a correctness bug — no clipping (float). Loudness change is acceptable for v1. FxChain integration (07-04) can add wet/dry mix to control level. |

---

## 5. Audit & Compliance Readiness

**Evidence produced:** 5 GR tests with [granular] tag. GR1 (non-silent), GR2 (freeze), GR3 (spray contrast), GR4 (pitch spread contrast), GR5 (DoD marker). After M1 fix, GR3/GR4 provide genuine contrast evidence, not vacuous pass.

**Silent failure prevention:** grain_timer_=0 guarantees grain activity from first sample (GR1 can't vacuously pass with zero output). GR2 freeze test requires peak>0.01f after silence input. jassert(length>0) in hann() catches invalid state in debug builds.

**Post-incident reconstruction:** spawn_grain() "all busy → skip" is now documented. lerp_buf wrap is branchless and documented. process() ordering (capture → spawn → accumulate → replace) is explicit in pseudocode.

**Ownership:** GranularProcessor is self-contained. No shared state with FxChain (07-04 integration is separate plan). Each test uses a fresh instance (no cross-test contamination).

---

## 6. Final Release Bar

Before this plan ships:
- GR1–GR5 all pass with 124/124 total suite green
- M1 fix verified: GR3 shows max_diff > 0.001f (spray=0 vs spray=0.5 measurably differ)
- M1 fix verified: GR4 shows max_diff > 0.001f (pitch_spread=0 vs pitch_spread=0.5 differ)
- clang-format clean on all 3 new files
- No allocation in process() (fixed grain pool only)

Remaining risks if shipped as-is (before M1 fix — now applied):
- GR3/GR4 would pass vacuously or fail, providing no coverage of spray/pitch_spread behavior
- Future refactors could silently break spray wiring with no regression detection

After M1 + SR1 + SR2: Plan is acceptable for APPLY. Tests are structurally sound.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
