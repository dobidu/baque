# Enterprise Plan Audit Report

**Plan:** .paul/phases/08-scatter-perf-fx/08-01-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The plan is well-architected: standalone-first (mirrors proven 07-01/07-03
convention), anti-vacuous test discipline carried forward from the 07-03 GR3/GR4 lesson, clear
boundaries isolating wiring/p-lock/APVTS to 08-02, and explicit RT-safety framing. It was NOT
release-ready as written because of one central DSP-correctness gap: the ring buffer's read/write
head relationship was unspecified. Left to interpretation, repeat/reverse would have undefined
latency semantics and a real self-capture/feedback hazard (read region overlapping the live write
region). With the read-anchor/freeze invariant pinned, I would approve for APPLY.

## 2. What Is Solid

- **Standalone-first slicing.** Proving the DSP in isolation before wiring (08-02) is the correct
  keystone decomposition and matches the 07-01/07-03 pattern that has held for two phases.
- **Anti-vacuous tests.** SC2 explicit fill-phase-before-assert (07-03 GR3/GR4 lesson), SC4/SC6
  contrast checks. This is the right reflex and should not be diluted.
- **Boundaries.** Hard fences on fx_chain/fx_params/plock_pattern/plugin_processor + "129 tests
  must not regress" correctly protect prior work.
- **slice_length_samples() as a static helper.** Makes AC-8 deterministically testable without
  reaching into private state. Good testability seam.

## 3. Enterprise Gaps Identified

1. **(M1) Ring read/write head relationship undefined — correctness + feedback risk.** The plan said
   primitives "read from ring" and write_pos_ "advances per sample" but never defined the read anchor
   relative to write_pos_, nor that slice phase persists across process() blocks (a 6000-sample slice
   spans many 512-sample blocks). Without a frozen read anchor trailing write_pos_ by slice_len, the
   looped/reversed region can overlap the concurrently-written live region → self-capture/feedback,
   and the slice phase resets per block → wrong glitch. This is the load-bearing invariant of the
   whole engine.
2. **(SR1) Stereo coherence unspecified.** Ring is per-channel, but if gate mask / read offset /
   decimate stride are derived per channel independently, L and R glitch out of phase → stereo image
   tears. (Same class of concern flagged for granular stereo coherence.)
3. **(SR2) reset() has no acceptance coverage.** reset() was declared but no AC verified it clears the
   ring + heads. On transport stop/restart a stale frozen slice could replay → audible stale glitch.
4. **(deferred) SC7 alloc-freeness "by inspection + jassert"** — soft, but consistent with accepted
   prior-phase practice.
5. **(deferred) NaN/Inf input sanitization** — output-finite check only; input is the internal mix.
6. **(deferred) Denormal guard inside ScatterEngine** — no IIR/feedback path here; plugin-level
   ScopedNoDenormals covers the integration path (08-02).

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Ring read-anchor/freeze invariant + slice-phase persistence undefined (feedback/UAF + latency ambiguity) | AC-2, AC-6, Task 2 action, boundaries INVARIANTS, verification, SC6 | Pinned: slice_phase = sample_counter_ % slice_len persists across blocks; read_anchor_ frozen per slice = (write_pos_ - slice_len) wrapped, trails write by slice_len; jassert(slice_len < ring_size_) → no overlap/feedback; circular wrap mandatory. Tightened AC-6/SC6 to exact dry at depth=0 (math guarantees it). |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | Stereo coherence unspecified — per-channel offsets would tear image | Task 2 action, boundaries INVARIANTS, verification, new test SC10 | Slice phase / read_anchor_ / gate mask / decimate stride SHARED across L+R, only sample data differs. SC10 asserts gated positions coincide across channels. |
| SR2 | reset() correctness uncovered — stale slice replay on transport stop/restart | New AC-9, boundaries INVARIANTS, new test SC9 | AC-9: reset() zeros ring + write_pos_ + read_anchor_ + sample_counter_; SC9 captures A, resets, feeds B, asserts no residue of A. |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Hard runtime alloc-detection in SC7 | Inspection + jassert + pre-allocated ring is the accepted standard across phases 4–7; no regression in rigor. |
| D2 | NaN/Inf input sanitization | Input is the internal stereo mix (already bounded); SC7 output-finite check is sufficient guard for this standalone scope. |
| D3 | Denormal guard inside ScatterEngine | No IIR/recursive/decay path in scatter (frozen-slice playback only); plugin-level ScopedNoDenormals covers the integrated path in 08-02. |

## 5. Audit & Compliance Readiness

- **Defensible evidence:** SC1..SC10 map 1:1 to AC-1..AC-9 + SR1; ctest -R "[scatter]" is the auditable
  gate. slice_length_samples() gives a pure, inspectable beat-sync proof (SC8).
- **Silent-failure prevention:** type out of [0,10] → bypass + jassert; jassert(slice_len < ring_size_)
  catches the feedback-overlap precondition at debug time; anti-vacuous SC2/SC4/SC6 prevent
  green-but-meaningless passes.
- **Post-incident reconstruction:** the four ring invariants are now written into boundaries, so a
  future maintainer changing ring sizing or slice math has the contract in front of them.
- **Ownership:** standalone class, single-writer, no shared mutable state across threads in this scope.

## 6. Final Release Bar

**Must be true before APPLY ships:** read-anchor/freeze invariant implemented exactly as specified
(frozen per slice, trails write by slice_len, slice_len < ring_size_, circular wrap); slice phase
persists across blocks; reset() zeros all state; stereo phase shared. All covered by SC1..SC10.

**Remaining risk if shipped as-is:** low for this standalone scope — alloc-freeness rests on
inspection+jassert (accepted practice), and host-ppq alignment is explicitly deferred to 08-02, so
musical-grid drift is out of scope here by design.

**Sign-off:** With M1 + SR1 + SR2 applied, I would sign my name to this plan for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
