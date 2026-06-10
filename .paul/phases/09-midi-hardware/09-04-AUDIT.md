# Enterprise Plan Audit Report

**Plan:** .paul/phases/09-midi-hardware/09-04-PLAN.md
**Audited:** 2026-06-09
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The plan is well-scoped, honest about what it ships vs. defers (note
map populated, CC numbers left for hardware confirmation — correct call), and reuses the proven
09-01/02/03 mechanism rather than re-implementing it. It would NOT have been production-defensible
as written for two reasons: (1) the Phase-9 DoD INT-lane assertion was vacuous — it would pass on
a lane that never fired, certifying "internal lanes don't leak MIDI" without proving any internal
lane did anything; (2) the template-apply path risked silently wiping the user's programmed beat.
Both are fixed below. With the must-have applied, I would sign off on this as the Phase 9 closer.

## 2. What Is Solid (Do Not Change)

- **CC-number honesty.** Shipping the documented Roland GM note map populated while leaving
  firmware-specific CC numbers OFF (confirmed at the hardware checkpoint) is exactly right —
  fabricating CC numbers would silently drive the wrong knob. Do not change.
- **Mechanism reuse.** Templates only WRITE lane_routing_/cc_out_/pattern; they do not touch the
  EXT-buffer ordering, clock, or stop-flush. Correctly layered on top of 09-01/02/03.
- **Checkpoint with a defer path.** The real-TR-8 sign-off as checkpoint:human-verify with an
  explicit "defer hardware" branch is the honest way to handle a DoD that software cannot
  self-certify. Keep it.
- **Single-writer discipline** carried forward from lane_routing_/cc_out_ — consistent with the
  whole project's threading contract.

## 3. Enterprise Gaps Identified

- **Vacuous INT-lane DoD check.** P9D1 asserted only "no MIDI note-on for the INT lane," and even
  warned AGAINST checking audio. That assertion is trivially true for an idle lane → the DoD would
  certify hybrid INT/EXT coexistence without ever proving an internal lane fired. Recurring failure
  mode in this project (05-03 FP2, 06-03 SC5, 06-04 PD3).
- **Integration-vs-unit ambiguity.** The plan said "real BaqueProcessor, processBlock" but did not
  forbid testing via Sequencer::generate. Testing the unit would not prove lane_routing_ + cc_out_
  + clock_master_ are wired together (the exact 08-02 M1 lesson).
- **Destructive template apply.** Task 1 offered "apply to a default StepPattern and set it" as a
  fallback. Applying a hardware template would then erase every active step / trig / p-lock the user
  programmed — silent data loss. Also the processor had no way to READ the current pattern, so the
  destructive path was the only specified one.
- **Note-off / clock-exactness blind spot.** P9D1/P9D2 checked note-ON and clock > 0 only. A
  regression that drops EXT note-offs (stuck hardware notes) or doubles/drops a clock at a block
  boundary (09-02's own M1 risk) would pass the DoD unnoticed.
- **cc_out_ wholesale reset** on every template apply was unspecified as intentional vs. accidental.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Vacuous INT-lane check + unit-not-integration risk | AC-4b (new), Task 2 (P9D1b), boundaries, verification | DoD INT check now requires NON-ZERO audio (lane provably fired) before asserting no-INT-MIDI; mandated processBlock path, forbade Sequencer::generate; grep + checklist guards |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | Template apply must not wipe step programming; needs current-pattern read | AC-2b (new), Task 1, frontmatter (sequencer.h), boundaries, verification | apply_template = set_note only (never set_active/set_trig/clear); added Sequencer::current_pattern() accessor; processor apply copies current pattern (no fresh default); routing+cc reset documented as intentional |
| SR2 | DoD missed EXT note-off + exact clock count | AC-4, Task 2 (P9D2), verification | Assert matching EXT note-OFF on gate close (no stuck note) + EXACTLY the 24ppqn count for elapsed ppq (not >0) |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | TR-8S advanced/assignable per-instrument routing + TR-8S-only CC banks | v1 template = shared GM default; per-instrument assignment is UI-coupled → Phase 10/11 |
| D2 | Template import/export file format (custom mapping save/load) | Preset/library concern (ESCOPO §10) → Phase 11 |
| D3 | Automated clock-jitter measurement vs real hardware | No TR-8 in CI; observational at the manual checkpoint only |

## 5. Audit & Compliance Readiness

With M1 applied, the DoD now produces defensible evidence: the hybrid test proves an internal lane
fired (audio) AND emitted no MIDI, on the real processBlock path, with exact clock and note-off
accounting. Silent failures are prevented — a dropped note-off, a doubled clock, a wiped pattern, or
an idle "passing" INT lane each now fail a specific assertion. The hardware sign-off is explicitly
tracked (approved with CC numbers, or deferred-and-recorded), so post-incident reconstruction of
"did we actually verify against a TR-8?" is unambiguous. Ownership: single-writer contract documented
for Phase 10 UI handoff.

## 6. Final Release Bar

Must be true before this ships:
- M1 applied (done): non-vacuous INT check + processBlock-only DoD.
- SR1 + SR2 applied (done): non-destructive apply + note-off/exact-clock coverage.
- ctest green incl. test_hardware_templates + test_phase9_dod; no regression in MIDI routing/clock/cc.
- AC-6 resolved: real TR-8 plays in sync (CC numbers baked in) OR hardware sign-off deferred and
  recorded in STATE.md.

Remaining risk if shipped as-is: hardware CC numbers are unconfirmed until a TR-8 is on the bench
(by design — slots ship OFF). Real-hardware jitter is observed, not gated. Both acceptable and
explicitly deferred. With the above applied, I would sign my name to this as the Phase 9 closer.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
