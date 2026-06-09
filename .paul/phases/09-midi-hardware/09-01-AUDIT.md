# Enterprise Plan Audit Report

**Plan:** .paul/phases/09-midi-hardware/09-01-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The routing design is correct (per-lane INT/EXT/BOTH + channel, optional
backward-compatible generate() params, reuse of the 08-04 trig/mute/solo gate, correct MIDI-in-before-
MIDI-out ordering, producesMidi + CMake flag). It was NOT release-ready because of a MIDI-to-hardware
correctness hazard: stopping the transport strands ringing external notes (generate() early-returns
when stopped, so the note-off never fires → the TR-8/synth holds the note as a drone). With an
All-Notes-Off stop-flush and the EXT note-off note-source pinned, I would approve for APPLY.

## 2. What Is Solid

- **Optional params / backward compat.** routing+midi_ext default to nullptr → legacy behavior;
  existing generate() callers compile unchanged. MR1/MR6 guard it.
- **Gate reuse.** EXT note-on inside the existing trig/mute/solo gate (08-04), note-off unconditional
  — consistent with the internal stuck-note policy.
- **Ordering.** Consuming host MIDI-in (scheduler) before clearing + writing EXT out is the correct
  sequence for a MIDI-generating instrument.
- **producesMidi + NEEDS_MIDI_OUTPUT.** Correctly recognizes the plugin-capability flag is required
  or the host never exposes a MIDI out (a common omission).
- **Resolves a long-standing deferred item** (per-lane channel filter, open since Phase 3).

## 3. Enterprise Gaps Identified

1. **(M1) Transport-stop strands hardware notes.** generate() returns early when !is_playing, so an
   EXT note that was on at stop time never receives its note-off → stuck note / drone on external
   hardware. Internal voices fade naturally; hardware does not. Needs All-Notes-Off (CC123) on the
   play→stop edge, per EXT channel.
2. **(SR1) EXT note-off note source ambiguous.** The plan reuses the internal note-off path, which
   keys on the INT NoteTracker. The tracker is only populated on the internal emit (note_triggered),
   so for an external-only lane it's empty (0) → EXT note-off would send the wrong note. Must use
   pattern.get_note(lane, prev_step) directly (deterministic, correct).
3. **(SR2) Stop-flush + note-off-source untested.** Both are correctness-critical and need explicit
   regression tests (MR8 stop-flush, MR9 note-off source).
4. **(deferred) Live mode-change mid-play** orphaning a held EXT note — no live routing changes in v1
   (no UI); Phase 10 concern.
5. **(deferred) EXT feel humanize / BOTH-lane INT-EXT timing divergence** — EXT uses swung clamped_pos
   without gaussian humanize; documented, v1-acceptable.
6. **(deferred) MIDI-thru passthrough** dropped by midi_messages.clear() — BAQUE generates its own
   out, not a thru device.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Transport stop strands ringing EXT notes on hardware | New AC-7, Task 2 (stop-flush impl + was_playing_ member), INVARIANTS, verification | On play→stop edge, emit All Notes Off (CC123) per unique EXT/BOTH channel into the out buffer, once at the edge (generate never emits the note-off when stopped). |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | EXT note-off note source (INT tracker empty for EXT-only lanes) | Task 1 (note-off spec), INVARIANTS, verification | EXT note-off note = pattern.get_note(lane, prev_step) directly, not note_tracker_.get_active_note (INT-only). |
| SR2 | Stop-flush + note-off source untested | Task 3 (MR8 stop-flush, MR9 note-off source) | MR8: play→stop emits CC123 on EXT channel once; MR9: external-only note-off uses pattern note, not 0/tracker. |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Live mode-change mid-play orphaning held EXT note | No live routing change in v1 (no UI); Phase 10. The stop-flush covers the dominant stuck-note case. |
| D2 | EXT humanize / BOTH INT-EXT timing divergence | EXT uses swung clamped_pos; gaussian humanize for EXT is a later refinement; humanize off by default. |
| D3 | MIDI-thru passthrough | BAQUE generates its own MIDI out; not a thru device. |

## 5. Audit & Compliance Readiness

- **Defensible evidence:** MR1-9 map to AC-1..AC-7; ctest [midi_routing] is the auditable gate.
  Real-TR-8 sync is explicitly a manual hardware verify (09-04), not falsely automated.
- **Silent-failure prevention:** M1 stop-flush removes the stuck-note-on-hardware path (a real,
  audible, support-generating defect); SR1 prevents a wrong-note note-off; MR8/MR9 are permanent guards.
- **Reconstruction:** routing semantics, note-off source, stop-flush, and ordering are in INVARIANTS.
- **Ownership:** LaneRouting single-writer documented (Phase 10 UI must atomicize).

## 6. Final Release Bar

**Must be true before APPLY ships:** per-lane INT/EXT/BOTH + channel routing; EXT note-on gated,
note-off unconditional using pattern note; stop-flush All-Notes-Off per EXT channel on the stop edge;
producesMidi + NEEDS_MIDI_OUTPUT; MIDI-in consumed before EXT written; full suite green + pluginval.

**Remaining risk if shipped as-is:** low — live routing changes, EXT humanize, and MIDI-thru are
deferred by design; real-hardware sync validation is the manual DoD step in 09-04.

**Sign-off:** With M1 + SR1 + SR2 applied, I would sign my name to this plan for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
