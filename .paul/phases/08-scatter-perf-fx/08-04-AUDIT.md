# Enterprise Plan Audit Report

**Plan:** .paul/phases/08-scatter-perf-fx/08-04-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The plan correctly closes the Phase 8 DoD (fills via trig conditions +
mute/solo + live-scatter-no-dropout), defers scene morph with a logged rationale, and keeps the
generate() signature backward-compatible (optional `perf` param). It was NOT release-ready because
of a subtle but real MIDI-correctness gap: `note_triggered()` lives inside the `is_active` block, so
gating only the note-on *emit* (not the tracker update) would let a suppressed fill/muted step poison
the NoteTracker and mis-target the next note-off. With the fire gated as a unit and a regression test,
plus the PerfState single-writer contract documented, I would approve for APPLY.

## 2. What Is Solid

- **Note-off left unconditional.** Correct given the one-step note-duration model — prevents stuck
  notes when conditions change, and (with M1 applied) the tracker stays consistent.
- **Backward-compat optional param.** `perf=nullptr` → legacy behavior; existing test_sequencer/
  test_plock callers compile unchanged. TF4/MS4 explicitly guard this.
- **Scene-morph deferral.** Correctly recognized as a UI/scene-coupled performance gesture, out of the
  Phase 8 DoD, and logged to deferred issues rather than half-built.
- **No PLockParam change.** Trig/mute/solo are sequencer-domain, not FxParams — avoids touching the
  4 count asserts. Clean separation.
- **DoD split.** D1 (audio FX RT-safety) + D2 (fills correctness) cover both halves of the DoD line.

## 3. Enterprise Gaps Identified

1. **(M1) NoteTracker poisoning under suppression.** `note_triggered(lane,note)` is inside the
   `if (is_active)` block (sequencer.cpp:121). Gating only the emit but still calling note_triggered
   for a suppressed step records a note that never sounded → the subsequent note-off keys off a wrong
   tracked note. The whole fire (tracker update + emit) must be gated as one unit.
2. **(SR1) PerfState concurrency contract unstated.** Read on the audio thread in generate(); no
   writer in v1, but Phase 10 UI will write fill/mute/solo from the message thread → data race unless
   upgraded to atomics/command queue. Same class as the Phase-4 "pad params single-writer" decision;
   must be documented so it isn't a silent race later.
3. **(SR2) Suppression interaction untested.** The note-off/tracker behavior under a suppressed step
   is correctness-critical and non-obvious; needs an explicit regression test (not just the happy-path
   fill counts).
4. **(deferred) Ratio trig conditions** (1:2, 2:4, A/B, prev, rnd%) — enum is extensible; DoD only
   needs always/fill/not_fill. Post-v1.
5. **(deferred) APVTS/UI for fill/mute/solo** — Phase 10.
6. **(deferred) Scene morph** — already deferred + logged (Phase 10/11).

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | note_triggered must be gated with the emit; suppressed step must not touch NoteTracker | Task 1 step 3, new AC-7, INVARIANTS, verification | Gate the entire fire as a unit (`if (is_active && trig_ok && audible)` wrapping note_triggered + emit); note-off stays unconditional; AC-7 + TF5 verify tracker integrity. |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | PerfState concurrency contract unstated | Task 1 step 2 (perf_state.h spec), INVARIANTS, verification | Documented single-writer contract: no concurrent writer in v1; Phase 10 UI must migrate to atomics/command queue (mirrors Phase-4 pad-params decision). |
| SR2 | Suppression interaction untested | Task 1 tests (new TF5), verification | TF5: step0 always (A) + step1 fill (B), fill_active=false → B count 0, no note-off references B, A has on+off. Regression guard for tracker under suppression. |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Ratio trig conditions (1:2/A/B/prev/rnd%) | Enum extensible; DoD needs only always/fill/not_fill. Post-v1. |
| D2 | APVTS/UI for fill/mute/solo | Phase 10 (UI/UX) owns control surface + automation. |
| D3 | Scene morph | Already deferred + logged (Phase 10/11); UI/scene-coupled. |

## 5. Audit & Compliance Readiness

- **Defensible evidence:** TF1-5, MS1-4, D1-3 map to AC-1..AC-7; ctest tags [trig]/[mute_solo]/[dod]
  are the auditable gates.
- **Silent-failure prevention:** M1 removes the tracker-poison path (mis-targeted note-off); TF5 is a
  permanent regression guard; SR1 documents the race so Phase 10 doesn't introduce one unknowingly.
- **Reconstruction:** the fire-gating-as-unit, note-off-unconditional, and single-writer rules are
  written into INVARIANTS.
- **Ownership:** sequencer-domain state (PerfState) separate from FxParams; clear boundary.

## 6. Final Release Bar

**Must be true before APPLY ships:** fire gated as a unit (suppressed step does not touch NoteTracker);
note-off unconditional; trig always/fill/not_fill correct; mute/solo audible rule correct; PerfState
single-writer documented; D1 (200-block live FX finite) + D2 (fills add hits) green; full suite green.
This is the last plan in Phase 8 — its UNIFY triggers transition.

**Remaining risk if shipped as-is:** low — ratio trigs, fill/mute/solo UI, and scene morph are
deferred by design and out of the Phase 8 DoD.

**Sign-off:** With M1 + SR1 + SR2 applied, I would sign my name to this plan for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
