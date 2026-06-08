# Enterprise Plan Audit Report

**Plan:** .paul/phases/08-scatter-perf-fx/08-03-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The plan is well-structured (standalone-then-wire collapsed sensibly for
two small DSP modules, order invariant pinned, clamp + 4-count-assert handling carried from 08-02).
It was NOT release-ready because the tape-stop "halt" was specified as holding the last interpolated
sample — that produces a sustained DC offset on the full mix, a real defect (speaker thump, possible
pluginval DC failure), not a tape-stop. With halt→silence, per-sample rate smoothing, and a
mandatory processBlock wiring smoke, I would approve for APPLY.

## 2. What Is Solid

- **Order invariant.** voices → fx_chain → scatter → gater → tape_stop, with tape_stop explicitly
  last as the "master halt", is the correct TR-8 performance-FX topology and is pinned.
- **Bypass-exact at param≤eps + continuous capture** mirrors the scatter contract — predictable,
  click-free re-engage.
- **4-count-assert override flagged up front** (incl. SCH2 added in 08-02) — the LC4 lesson is fully
  internalized; no APPLY-time surprise.
- **Gater stereo-coherent shared phase + anti-click fade** correctly anticipates the image-tear and
  click hazards from prior audits (SR1 of 08-01, gate work).

## 3. Enterprise Gaps Identified

1. **(M1) Tape-stop halt = DC hold.** "rate→0 → segura último valor interpolado" sustains a DC offset
   on the master mix at tape_stop=1. DC on output is a defect: speaker thump, breaks downstream gain
   staging, and pluginval has DC-offset checks. A tape machine stopping goes to silence, not a held
   level. Must ramp output gain to 0 as rate→0.
2. **(SR1) Rate smoothing specified per-block.** "one-pole coef 0.999/bloco" is ambiguous and, if
   per-block, produces a stair-stepped rate (zipper/pitch-steps). Must be per-sample (SmoothedValue,
   ~30-50 ms) like the FxChain smoothers.
3. **(SR2) Wiring smoke hedged ("se prático").** Same gap class as 08-02 M1: if prepareToPlay forgets
   tape_stop_/gater_.prepare(), ring_size_=0 → silent no-op → the wiring is unverified but tests pass.
   The processBlock smoke must be mandatory, not optional.
4. **(deferred) Gater pattern editor / multiple gate rates** — v1 is fixed 1/16 + depth; editable
   patterns are UI/Phase-10 territory.
5. **(deferred) Tape-stop trigger mode / configurable ramp curve** — continuous param + one-pole is
   sufficient for v1.
6. **(deferred) vinyl-brake / riser / down-sampler-sweep variants** (ESCOPO §4.7) — not in the Phase 8
   DoD; post-v1.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Tape-stop halt holds DC instead of silencing | AC-4, Task 1 (tape_stop action), Task 3 TS2, INVARIANTS, verification | Halt now ramps output gain→0 (out *= rate-derived gain); AC-4 + TS2 assert |sample|<0.01 at full stop (silence), explicitly NOT DC-freeze. |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | Per-block rate smoothing → zipper | Task 1 (tape_stop action), INVARIANTS, verification | Specified juce::SmoothedValue per-sample (~30-50 ms), getNextValue() each sample; removed ambiguous "coef 0.999/bloco". |
| SR2 | Wiring smoke was optional ("se prático") | Task 3 (new GT6), INVARIANTS, verification | GT6 made mandatory: BaqueProcessor processBlock × 64 with tape_stop=1 + gate_depth=1 via APVTS, assert finite — proves prepare+call (08-02 M1 lesson). |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Gater editable patterns / multiple rates | v1 fixed 1/16 + depth; pattern editor is Phase 10 (UI). |
| D2 | Tape-stop trigger mode / ramp-curve config | Continuous param + one-pole smoothing covers the v1 use case. |
| D3 | vinyl-brake / riser / down-sampler-sweep | ESCOPO §4.7 extras, not in Phase 8 DoD; post-v1. |

## 5. Audit & Compliance Readiness

- **Defensible evidence:** TS1-3 + GT1-6 map to AC-4/AC-5/AC-6/AC-2; GT6 is the auditable wiring gate.
- **Silent-failure prevention:** DC-halt removed (M1) eliminates a downstream/hardware hazard and a
  likely pluginval DC failure; GT6 catches an unprepared processor; clamp [0,1] guards bad p-locks.
- **Reconstruction:** order, halt-to-silence, per-sample smoothing, and bypass-exact are written into
  INVARIANTS for future maintainers.
- **State-recall safety:** PLockParam 0-12 frozen; new entries appended at 13/14.

## 6. Final Release Bar

**Must be true before APPLY ships:** tape-stop halt → silence (no DC); rate smoothed per-sample;
gater beat-synced 1/16 with anti-click fade; tape_stop/gater after scatter (tape_stop last); 4 count
asserts → 15; GT6 wiring smoke green; full suite green.

**Remaining risk if shipped as-is:** low — gater pattern editing, tape-stop trigger curves, and
§4.7 brake/riser variants are deferred by design and out of the Phase 8 DoD.

**Sign-off:** With M1 + SR1 + SR2 applied, I would sign my name to this plan for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
