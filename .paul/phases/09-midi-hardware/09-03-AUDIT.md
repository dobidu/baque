# Enterprise Plan Audit Report

**Plan:** .paul/phases/09-midi-hardware/09-03-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded (now ready for APPLY)

---

## 1. Executive Verdict

Conditionally acceptable. The plan is well-scoped and correctly extends the 09-01/09-02
MIDI machinery, but it contained one release-blocking threading defect and one correctness
trap that would not surface until a real UI or a learn→CC round-trip exists. With the three
findings applied, I would approve it for APPLY. I would NOT have signed off on the original:
the MIDI-learn data structure mutated on the audio thread with a plain (non-atomic) arm field,
and it gets serialized into saved state — both hard to fix after the schema ships.

## 2. What Is Solid

- **EXT-buffer ordering respected.** CC-out is correctly inserted after `apply_plock_batch`
  and before the `midi_clock_.process` / clear+addEvents invariant (09-02). The plan does not
  touch the frozen order.
- **Precedence chain explicit.** APVTS < inbound CC < per-step p-lock, with inbound parse placed
  before `apply_plock_batch`. Read-only iteration before `scheduler_.process` consumes notes is
  the right sequencing.
- **PLockParam count=15 treated as frozen** — CC maps are keyed by it, never extend it. Matches
  the standing Phase 6–8 boundary.
- **Backward-compatible persistence intent** (pre-v3 state → empty map, no throw) is correct.

## 3. Enterprise Gaps Identified

1. **Audio-thread write race in MIDI learn (latent).** Capture appends/rebinds `cc_learn_` on the
   audio thread while `arm_learn()` writes from the message thread. This is NOT the single-writer
   pattern used in 09-01/09-02 (there: message writes, audio only reads). A plain `learn_arm`
   field is a torn-read / lost-update. The structure is also serialized → the contract is locked
   by schema v3 and cannot be fixed cheaply later.
2. **Range duplication / norm-denorm drift.** T1 normalize and T2 denormalize each restated the
   per-param ranges. Any future edit to one side silently corrupts learn→CC-out round-trips.
3. **Arbitrary "restrict inbound CC to 10 fields" limit.** Inconsistent (scatter/tape/gate ARE
   snapshot fields) and a silent capability gap — all 15 PLockParams have FxParams fields.
4. **Capture could latch a note as a learn target** if the loop did not gate on `isController()`.
5. **CC-out additivity unverified** — nothing asserted that emitting CC leaves note/clock events
   intact (count + position).
6. **Corrupt-state hardening underspecified** — only target was guarded; cc/channel/count were not.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | MIDI-learn audio-thread write race | T3 action, `<boundaries>`, success_criteria, verification | `learn_arm` → `std::atomic<int>` with release/acquire arm-disarm handshake; bindings documented audio-thread-owned during capture; Phase 10 UI must snapshot (mirrors pad-params/PerfState); CcLearnMap non-copyable consequence spelled out (clear()/set_binding(), mutate-in-place, no struct assignment); DO-NOT-CHANGE boundary added |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | Range duplication + arbitrary 10-field restriction | T1 + T2 actions, SCOPE LIMITS, verification | Single `k_param_range[15]` table as source of truth, read by both `plock_param_norm` and `plock_param_denorm`; inbound coverage expanded to all 15 PLockParams; switch default jasserts (no silent no-op); grep check added |
| SR2 | Capture/additivity test gaps | New AC-6, T1+T3 verify, AC-5 | AC-6: armed learn binds only controllers (note-on must not bind) + CC-out is purely additive (note/clock count+position unchanged); AC-5 extended with corrupt target/channel/cc drop-or-clamp; per-field bounds-guard on state load (count [0,k_max], cc [0,127], channel [1,16], target [0,15)) |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Sub-block sample-accurate CC timing (PLockEvent sample offset) | Block-granular param automation is standard; PLockEvent carries no offset. Already scoped out; revisit if hardware testing (09-04) shows audible timing error |
| D2 | Log/perceptual filter_cutoff CC law | Linear is functional for v1; perceptual mapping is polish, not correctness |
| D3 | NRPN / 14-bit CC + CC smoothing/ramp | 7-bit CC covers TR-8/TR-8S targets; smoothing is a Phase-10/hardening concern |

## 5. Audit & Compliance Readiness

- **Defensible evidence:** AC-1..AC-6 each map to a named test in test_midi_cc.cpp; grep checks
  pin the structural invariants (single range table, atomic arm, emit ordering).
- **Silent-failure prevention:** switch default jasserts on unmapped target; state load clamps
  rather than producing OOB enum/channel; no silent no-op targets.
- **Post-incident reconstruction:** the threading contract and non-copyable consequence are
  documented in the header, so a future maintainer cannot accidentally reintroduce the race.
- **Ownership:** single-writer / atomic-arm contracts name Phase 10 as the owner of the UI
  escalation — consistent with pad-params and PerfState.

Remaining audit risk: MIDI-learn is exercised only sequentially in tests (no true concurrent
audio+message thread test). The atomic contract is correct by construction; full concurrency
proof is a Phase 12 hardening item, not a 09-03 blocker.

## 6. Final Release Bar

Before this plan ships (APPLY): `learn_arm` is atomic; one range table feeds both directions;
all 15 params mappable; AC-1..AC-6 green; state schema v3 round-trips and clamps hostile input;
EXT-buffer order + PLockParam count untouched. With those true, I would sign my name to it.
Residual risk if shipped as-is: block-granular CC timing and linear cutoff law — both documented,
both acceptable for v1, both flagged for the 09-04 hardware verify.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
