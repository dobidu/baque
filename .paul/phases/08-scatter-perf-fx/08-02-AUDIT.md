# Enterprise Plan Audit Report

**Plan:** .paul/phases/08-scatter-perf-fx/08-02-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The wiring design is correct (post-FxChain placement, APVTS+p-lock dual
control matching the Phase-6 param contract, explicit LC4 count-assert handling). It was NOT
release-ready because the plan's own test section hedged: "if BaqueProcessor not instantiable,
degrade to checking the IDs exist." That hedge would let 08-02 ship with the integration
**unverified** — the entire point of this plan is that scatter is prepared and called in the real
audio path after FxChain. `test_transport.cpp:136` already constructs `BaqueProcessor` and calls
`processBlock`, so the hedge is false and the real test is feasible. With the test upgraded and the
p-lock clamp added, I would approve for APPLY.

## 2. What Is Solid

- **Post-FxChain placement.** Scatter on the full wet mix (incl. reverb/delay tails) is the correct
  TR-8 performance-FX semantics and is pinned as an invariant.
- **APVTS + p-lock dual control.** Correctly follows the filter_cutoff pattern (host default + per-step
  override) rather than the lo_fi/granular p-lock-only pattern — appropriate because scatter is
  performance-automatable.
- **LC4 handling.** Plan explicitly flags the 3 `k_plock_param_count == 11 → 13` asserts as an allowed
  boundary override up front, carrying the 07-04 lesson forward instead of hitting it in APPLY.
- **Enum index stability.** "Do not reorder 0-10 (breaks state recall)" is the right constraint for a
  plugin that persists PLockParam-indexed state in DAW sessions.

## 3. Enterprise Gaps Identified

1. **(M1) Integration unverified — the plan hedged the only test that proves the wiring.** "Degrade to
   ID check" would pass even if `scatter_.prepare()` is forgotten (ring_size_=0 → silent no-op) or if
   scatter is wired before FxChain. test_transport proves the processor is testable, so the wiring
   must be positively verified: a deterministic post-chain ordering test + a real processBlock smoke.
2. **(SR1) p-lock can push scatter_type out of [0,10].** APVTS clamps to its range, but a p-lock value
   is an arbitrary float. `roundToInt(15.0)` → 15 → trips 08-01's `jassert(type in range)` in debug.
   Must clamp with `jlimit(0, k_num_types, …)` before dispatch.
3. **(SR2) Precedence undocumented + count-assert completeness.** The APVTS-then-plock override order
   matters (p-lock wins) and should be stated like the other params. And a grep confirms exactly 3
   `== 11` asserts exist — a verify-grep guards against a missed one (LC4 completeness).
4. **(deferred) Type change mid-roll.** Changing scatter_type while staying active doesn't re-latch
   (08-01 latches once per roll). Documented limitation; acceptable for v1.
5. **(deferred) Host ppq → slice grid alignment.** Uses transport_.bpm, not bar-phase alignment.
   Already scoped out; bpm-sync is sufficient for v1.
6. **(deferred) No UI/editor for the new params.** Belongs to Phase 10.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Integration unverified (test hedge would pass on broken wiring) | AC-4 (rewritten as ordering), new AC-4b, Task 3 (SCH4 ordering + SCH5 processBlock smoke), verification | Removed "degrade to ID check" hedge (test_transport proves instantiability). SCH4: FxChain→Scatter diverges from FxChain-only on a known buffer (non-vacuous, no voice-playback dependency). SCH5: real BaqueProcessor processBlock × 64 blocks asserts finite output (prepare+call proven). |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | p-lock can push scatter_type out of [0,10] → debug jassert in 08-01 | Task 2 step 5, SCH3, verification | `juce::jlimit(0, ScatterEngine::k_num_types, roundToInt(scatter_type))` before scatter_.process; SCH3 adds clamp case (15→10). |
| SR2 | Precedence + count-assert completeness | Task 2 step 3, Task 3 verify, verification | Documented p-lock-overrides-APVTS-snapshot precedence inline; added `grep -rn "== 11" tests/ → zero hits` to verify (guards a missed count assert). |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Type change mid-roll doesn't re-latch | Documented 08-01 behavior; musically valid (roll holds its slice); re-latch-on-type-change is a future refinement. |
| D2 | Host ppq bar-phase alignment | Explicitly scoped out; bpm-derived slice length is sufficient for v1 perf FX. |
| D3 | UI/editor for scatter params | Phase 10 (UI/UX) owns all editor binding. |

## 5. Audit & Compliance Readiness

- **Defensible evidence:** SCH1-5 map to AC-1..AC-5 + AC-4b; SCH4 proves post-chain semantics
  deterministically, SCH5 proves the real call site. ctest -R "scatter_chain" is the auditable gate.
- **Silent-failure prevention:** the jlimit clamp removes a debug-only crash path from bad p-locks;
  the count-assert grep prevents a stale `==11` from silently masking the enum growth; SCH5's
  finite-check catches an unprepared ring.
- **Reconstruction:** post-chain ordering + precedence + clamp are written into boundaries/invariants.
- **State-recall safety:** enum-order-frozen constraint protects saved DAW sessions.

## 6. Final Release Bar

**Must be true before APPLY ships:** scatter_.prepare in prepareToPlay; scatter_.process after
fx_chain_.process with jlimit-clamped type; 3 count asserts → 13; SCH4 (ordering) and SCH5
(processBlock smoke) present and non-vacuous; full suite green.

**Remaining risk if shipped as-is:** low — type-change-mid-roll and ppq-alignment are deferred by
design and documented; pluginval in CI exercises the processor instantiation path additionally.

**Sign-off:** With M1 + SR1 + SR2 applied, I would sign my name to this plan for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
