# Enterprise Plan Audit Report

**Plan:** .paul/phases/09-midi-hardware/09-02-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. Clock math (24 ppqn, tick positions), transport-edge logic (start/continue/
stop), and off=silent are correct, and the wiring into the existing EXT buffer is clean. It was NOT
release-ready because the plan recomputed clock ticks purely from ppq each block and explicitly
hedged "loop/restart could re-emit; acceptable v1." For a MIDI clock driving external hardware, a
duplicated or dropped 0xF8 is an audible tempo glitch on the TR-8 — directly failing the DoD "drives
real TR-8 in sync." With a monotonic absolute-tick guard, I would approve for APPLY.

## 2. What Is Solid

- **Clock as a pure, testable class** (mirrors StepClock) — deterministic, unit-testable without a host.
- **off=silent + clock in the EXT buffer** — clean reuse of the 09-01 merge path; clock and EXT notes
  coexist correctly.
- **start vs continue by ppq** (Start at ppq~0, Continue at ppq>0) — correct MMC semantics.
- **was_playing_ gated by (is_playing && master)** — avoids a spurious Start when master is toggled
  on mid-play (it sends Continue instead, correct).

## 3. Enterprise Gaps Identified

1. **(M1) Non-monotonic tick generation.** Recomputing ticks from ppq each block via ceil + strict
   `< ppq_end` is float-fragile at block boundaries (a tick at exactly ppq=N/24 can double-emit or
   drop by sub-ULP rounding), and on a DAW loop/ppq-regression the same ticks re-emit. Either makes
   the external clock jitter — the TR-8 momentarily speeds up / drops a step. The plan's own
   "acceptable v1" note understates a DoD-failing defect.
2. **(SR1) Boundary monotonicity untested.** The dominant failure (double/drop at a block boundary)
   needs an explicit two-contiguous-block test, plus a ppq-regression test.
3. **(SR2) start/stop sub-block position.** Emitting transport bytes at offset 0 regardless of where
   in the block the edge occurred is a minor timing inaccuracy; document it (clocks themselves are
   sample-accurate).
4. **(deferred) Slave/PLL** — scoped out (needs jitter filter).
5. **(deferred) SPP (Song Position Pointer)** — scoped out; start/continue/stop sufficient for v1.
6. **(deferred) Real-hardware jitter measurement** — manual verify in 09-04.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Non-monotonic clock ticks (boundary double/drop + loop re-emit) | New AC-6, Task 1 (last_tick_ absolute index + ppq-regression resync), INVARIANTS, verification | Added int64_t last_tick_; emit only ticks n > last_tick_; resync on ppq regression (no backfill). Guarantees no dup/drop at block boundaries — stable clock to hardware. |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | Boundary monotonicity untested | Task 1 (MC8 two-block-sum-24, MC9 regression), verification | MC8: contiguous ppq 0→0.5 + 0.5→1.0 sums to exactly 24, no dup at boundary. MC9: ppq regression resyncs without re-emitting history. |
| SR2 | Transport-byte sub-block position | SCOPE LIMITS, INVARIANTS | Documented start/stop/continue emitted at offset 0 in v1 (timeInSamples refinement later); clocks remain sample-accurate. |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Slave / follow-external-clock (PLL) | Needs jitter filter; out of master-out scope; later/post-v1. |
| D2 | Song Position Pointer | start/continue/stop cover transport for v1; SPP if a host/device needs it. |
| D3 | Real-TR-8 jitter measurement | Manual hardware verify in 09-04 (phase DoD). |

## 5. Audit & Compliance Readiness

- **Defensible evidence:** MC1-9 map to AC-1..AC-6; ctest [midi_clock] is the auditable gate. Real
  jitter explicitly a manual hardware step (09-04), not falsely automated.
- **Silent-failure prevention:** M1 removes the clock-glitch path (a defect that only shows on real
  hardware sync — exactly the kind of thing unit tests must guard, hence MC8/MC9); off=silent prevents
  clock pollution.
- **Reconstruction:** monotonic-tick + regression-resync + offset-0 transport are in INVARIANTS.
- **Ownership:** clock_master_ single-writer (public, like lane_routing_); UI phase atomicizes.

## 6. Final Release Bar

**Must be true before APPLY ships:** 24-ppqn clock; monotonic ticks via last_tick_ (no dup/drop at
boundaries; regression resync); start/continue/stop on edges; off=silent; clock merged to MIDI out;
MC1-9 green + full suite + pluginval.

**Remaining risk if shipped as-is:** low — slave/SPP/sub-block-transport-position deferred by design;
real-hardware sync is the manual DoD step in 09-04.

**Sign-off:** With M1 + SR1 + SR2 applied, I would sign my name to this plan for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
