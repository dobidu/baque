# Enterprise Plan Audit Report

**Plan:** .paul/phases/10-ui-ux/10-01-PLAN.md
**Audited:** 2026-06-10
**Verdict:** conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable as written; acceptable after applied upgrades.** The plan correctly identifies the threading debt and chooses the established mechanism (SPSC AbstractFifo + per-field atomic snapshot). However, it shipped with one release-blocking blind spot: by transferring struct ownership to the audio thread, it converts the existing message-thread state save/load paths into a data race the plan never mentions. A preset saved during playback could capture torn or stale state — silent corruption, unreproducible in tests that don't interleave. With M1 applied, I would approve this for production.

## 2. What Is Solid

- **Queue design:** fixed-capacity POD ring over `juce::AbstractFifo` is the project's established lock-free pattern; zero-alloc drain is verifiable by inspection. Full-queue rejection (no overwrite, no block) is the correct backpressure for UI writes.
- **Drain placement:** top of processBlock, before transport read and `generate()` — commands take effect atomically with respect to the block that follows. Correct ordering guarantee, explicitly tested.
- **`apply_hardware_template` moved INTO the audio thread:** elegant resolution — the method mutates routing/cc/pattern, all audio-owned after this plan; executing it as a drained command dissolves the 09-04 "single-writer setup" caveat instead of patching it.
- **Clamp-at-dispatch posture:** mirrors 08-02 SR1; UI bugs cannot push out-of-range state into DSP.
- **Integration tests drive real processBlock:** the 08-02/09-04 vacuous-test lesson is institutionalized; UI1-UI7 assert observable audio/MIDI consequences, not struct fields alone.
- **Boundaries:** sample-buffer loads explicitly kept on the safe-load protocol; CcLearnMap handshake untouched. Both are the correct exclusions.

## 3. Enterprise Gaps Identified

1. **State save/load race (M1).** `getStateInformation`/`setStateInformation` run on the message thread, host-initiated, legally concurrent with processBlock. The plan moves write ownership of `lane_routing_`, `cc_out_`, `plock_pattern_`, the active pattern, and pad params to the audio thread — making every state read/write in those methods a data race (formal UB, practical preset corruption). Additionally, a command pushed but not yet drained at save time would be silently absent from the preset.
2. **Snapshot hook plumbed into Sequencer internals (SR1).** "Hook where NoteTracker is updated" couples UI state into the sequencer fire path — a maintenance hazard (every future fire-path change must consider the UI hook) and unnecessary: the gated fire is already externally observable as a note-on in `midi_buffer_seq_`.
3. **Producer contract unenforced (SR2).** SPSC is stated in a comment only. A future background-thread producer (file scanner, preset loader) compiles fine and corrupts the FIFO in release. Plans 10-02..10-07 add many call sites.
4. **`apply_template` id clamping is the wrong failure mode (SR2).** Clamping an invalid id selects a wrong-but-valid template — silently reprograms 16 lanes. Worse than ignoring.
5. (Minor, noted) Queue capacity 256 vs. drain cadence: at typical block rates (≥ 90 drains/s) a knob drag cannot realistically fill the queue; no action needed beyond the existing full-rejection.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | State save/load races with audio-thread command application; undrained commands lost from presets | AC (new AC-5), Task 2 action, Task 3 tests (UI8), verification | Both state methods bracket with suspendProcessing(true/false); while suspended, drain pending commands ON the message thread before state read/write (SPSC consumer-role transfer valid only inside the bracket). UI8 test: push commands → getState without processBlock → restore → state present |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Lane-pulse hook coupled to Sequencer internals | Task 3 action | Derive lane_last_velocity by scanning midi_buffer_seq_ note-ons (lane = note − k_base_note); auto-respects 08-04 gating; Sequencer stays UI-agnostic; EXT-only-lane no-pulse documented as v1 limitation |
| 2 | SPSC producer contract unenforced; invalid template id clamped to wrong template | Task 1 action, Task 2 action, verification | push() debug-jasserts message thread (with documented bracket exception); apply_template with id ∉ {0,1} jasserts AND ignores instead of clamping |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | UI-side policy when push() returns false (retry/coalesce/drop indicator) | No UI exists yet; policy belongs to the consuming control kit (10-02). Queue contract (false, no block) is sufficient foundation |
| 2 | EXT-only lane pulse in snapshot | Needs lane↔hardware-note reverse mapping; only relevant once PERFORM screen renders pads (10-03); documented limitation |
| 3 | Host-automatable mute/solo/routing (APVTS exposure) | Scope decision, not threading; ESCOPO does not require host automation of these in v1; revisit at 10-05 (MIX screen) |

## 5. Audit & Compliance Readiness

- **Evidence:** every AC maps to a named automated test (QC1-4, UI1-UI8) running in the 3-OS CI gate; state-race fix carries its own regression test (UI8).
- **Silent-failure prevention:** full-queue push returns false (caller-visible); invalid template rejected with debug assert; suppressed steps cannot false-pulse (derived from gated note-ons).
- **Post-incident reconstruction:** threading contracts live in the headers they govern (updated comment blocks), not in tribal memory; the suspend-bracket rule is documented at both call sites.
- **Ownership:** clear after this plan — message thread produces commands and owns persistence (under suspension), audio thread owns live structs. The previous "single-writer, UI later" ambiguity is eliminated rather than extended.

## 6. Final Release Bar

Before this plan ships: all 4+1 ACs green via processBlock-driven tests; pluginval strictness 5 unchanged; both state methods show the suspend bracket; no existing test regresses.

Remaining accepted risks: cross-field snapshot tearing (by design, visualization only); EXT-only lanes don't pulse; queue-full UX undefined until 10-02.

With M1/SR1/SR2 applied: I would sign my name to this threading layer.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
