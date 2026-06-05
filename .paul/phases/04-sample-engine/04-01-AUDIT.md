# Enterprise Plan Audit Report

**Plan:** .paul/phases/04-sample-engine/04-01-PLAN.md
**Audited:** 2026-06-05
**Verdict:** Conditionally acceptable → upgraded (must-haves applied)

---

## 1. Executive Verdict

**Conditionally acceptable as written; acceptable after applied upgrades.**

The plan's architecture is sound — pad bank model, note-arithmetic routing, in-house varispeed, preserved Phase 2/3 contracts. But as written it contained two release-blocking latent defects: an unspecified sample-load protocol that permits use-after-free on the audio thread (host crash, the worst failure class for a plugin), and underspecified end-of-sample behavior under rate >1 with linear interpolation (out-of-bounds read). Neither would surface in happy-path tests; both would surface in production hosts. With the applied upgrades, I would approve this plan for execution.

## 2. What Is Solid

- **Note→pad arithmetic lookup** (`note − 36` into array) — O(1), allocation-free, cache-friendly, consistent with the StepPattern convention locked in Phase 3. Correctly rejects out-of-range and empty pads.
- **Varispeed in-house, fork isolated to 04-04** — correctly reads ESCOPO §4.11's three-path split; realtime varispeed is a rate change, not a stretch. Unblocks 75% of Phase 4 without the fork dependency.
- **Phase 3 files in DO NOT CHANGE boundaries** — sequencer/step_clock/note_tracker are audited and stable; protecting them prevents regression of two prior enterprise audits.
- **APVTS layout frozen** — plugin identity and state compatibility locked since Phase 1; the plan explicitly keeps pad params out of APVTS for now. Correct.
- **Hoisted per-frame math** (rate/pan computed at trigger) and explicit rejection of std::map for routing — RT-discipline carried forward.
- **Legacy compatibility** — pad 0 inherits the Phase 2 WAV load path; existing behavior and tests remain meaningful.

## 3. Enterprise Gaps Identified

1. **Use-after-free on sample load (critical).** "Load-before-play contract documented" is not a control — it is a hope. `SampleVoice::data_` is a raw pointer into a pad's `AudioBuffer`; `setSize`/reload reallocates; an active voice then reads freed memory on the audio thread. Crash inside a DAW. No specified invalidation protocol, no test.
2. **Out-of-bounds interpolation read (critical).** Linear interp reads `floor(pos)+1`. At rate 4.0 (+24 st) position can pass `num_samples − 1` mid-block. "Clamp last frame" was mentioned without a termination condition or test. Silent OOB read in release builds.
3. **`start_offset_` semantics ambiguous under reverse.** Block-relative dispatch delay vs sample-data position — a reverse implementation could plausibly misread it. Phase 2's sample-accuracy DoD silently breaks if misinterpreted.
4. **Velocity curve unpinned.** "Velocity-scaled gain" without a defined mapping → nondeterministic tests, divergent implementations across call sites, untraceable level changes in later phases.
5. **Pad param concurrency contract implicit.** Plain floats read by audio thread are safe only under a single-writer, no-live-edit discipline. Undocumented, this invariant will be violated by the first UI phase that adds a pan knob.
6. **Frontmatter `files_modified` incomplete** — Task 2 modifies `tests/test_voice.cpp`, absent from the list. Breaks conflict detection for parallel plans.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Use-after-free on pad sample load | AC (new AC-5), Task 3 action/verify/done, verification | Safe-load protocol: `pool.reset_all()` (or suspendProcessing bracket) BEFORE buffer mutation; single-writer contract; dedicated use-after-free test |
| 2 | OOB interpolation read at rate >1 | AC-2, Task 2 action/verify, verification | Explicit termination: forward deactivates at `pos ≥ num_samples − 1`, reverse at `pos ≤ 0`; jassert bounds invariant; rate 4.0 short-sample test |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | `start_offset_` semantics under reverse | Task 2 action/verify | Pinned as block-relative dispatch delay (unchanged from Phase 2); reverse+offset silence test added |
| 2 | Velocity curve unpinned | Task 3 action, verification | Pinned linear `velocity/127 × pad.gain`; v127/v64 unit tests; single change point documented |
| 3 | Pad param concurrency contract implicit | Task 1 action | Single-writer contract documented in pad_bank.h as the invariant future UI/automation phases must upgrade |
| 4 | `files_modified` missing test file | Frontmatter | Added `tests/test_voice.cpp` |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | Equal-power center pan = −3 dB vs legacy mono-to-both | Behavior change documented in plan with test adjustment; perceived-level policy belongs to Phase 9/10 mixer work |
| 2 | Lock-free sample hot-swap while playing | Safe-load protocol (must-have #1) removes the crash; live hot-swap is Phase 12 hardening scope, already in plan boundaries |
| 3 | Interpolation quality beyond linear (cubic/sinc) | Explicit R&D-TS track concern (ESCOPO §4.11); linear is the documented Phase 4 baseline |

## 5. Audit & Compliance Readiness

- **Evidence:** Catch2 tests per AC (routing, rate math, bounds, reverse, offset, velocity, safe-load) + pluginval strictness 5 + RT-allocation test = reproducible, CI-archived evidence. Defensible.
- **Silent failures:** The two must-haves were exactly silent-failure shapes (UAF, OOB read in release builds). Post-upgrade, both have failing-test coverage and a debug assert. Unmapped notes skip silently by design — acceptable and specified in AC-1.
- **Reconstruction:** Plan → AUDIT → SUMMARY chain plus pinned conventions (note−36 mapping, velocity curve, offset semantics) recorded in STATE.md decisions supports post-incident reconstruction.
- **Ownership:** Single-writer contract now written where the next maintainer will read it (pad_bank.h), not just in planning docs.

## 6. Final Release Bar

Before this plan ships:
1. Safe-load test proves no voice reads a mutated buffer (AC-5) — non-negotiable.
2. Rate 4.0 bounds test green with jassert active (AC-2 extension).
3. All 21 existing tests pass; the only sanctioned behavior delta is the documented equal-power center level in adjusted tests.
4. pluginval strictness 5 green.

Remaining risks if shipped post-upgrade: pad params still rely on a documented (not enforced) single-writer discipline — acceptable until UI phase, flagged for upgrade there; center-pan level change may surprise A/B against Phase 2 audio — documented.

**Sign-off:** With the applied upgrades executed and verified, I would sign my name to this plan.

---

**Summary:** Applied 2 must-have + 4 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
