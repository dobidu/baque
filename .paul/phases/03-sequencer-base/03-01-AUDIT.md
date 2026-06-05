# Enterprise Plan Audit Report

**Plan:** .paul/phases/03-sequencer-base/03-01-PLAN.md
**Audited:** 2026-06-05
**Verdict:** Conditionally acceptable → upgraded → ready for APPLY

---

## 1. Executive Verdict

Conditionally acceptable. Architecture is correct (stateless StepClock, Sequencer owns state, pattern pre-populated for immediate audio), but three gaps were production-blocking: the MidiBuffer merge strategy was unresolved (with a misleading claim that addEvents is "alloc-free if pre-allocated" which is conditional, not guaranteed), the Sequencer::generate() algorithm was underspecified enough that an implementer would guess incorrectly on wrap-around, and the double-trigger problem at step boundaries was identified but not solved. All applied. Approved post-upgrades.

## 2. What Is Solid

- StepClock stateless design — correct for RT. All inputs passed explicitly; no hidden state to protect across threads.
- `std::array<std::array<Step, 16>, 16>` for StepPattern — fixed size, stack/inline allocation, no heap.
- Default pattern pre-populated (steps 0,4,8,12) — produces audible output on first run; critical for rapid validation.
- Sequencer feeds existing Scheduler rather than duplicating dispatch — correct layering; Phase 2 investment reused.
- MidiBuffer pre-allocated in prepareToPlay not processBlock — correct.
- `last_step_fired_` pattern for double-trigger guard — correct approach once specified.

## 3. Enterprise Gaps Identified

1. **MidiBuffer merge approach unresolved (must-have):** Plan offered two options ("call scheduler twice OR merge") and claimed addEvents() is "RT-safe (no alloc if pre-allocated)." JUCE's MidiBuffer uses internal MemoryBlock that CAN reallocate on addEvents() if capacity is exceeded. The plan didn't specify ensureSize() or which approach to use — leaving this to APPLY-time guessing.

2. **Sequencer::generate() algorithm underspecified (must-have):** "Determine which step boundaries fall within this block using clock_" has no algorithm. Step wrap-around (15→0 across a block), how to walk forward from current step, and how to compute sample offsets were all missing. Incorrect implementations here cause timing drift or missed steps.

3. **Double-trigger guard unimplemented (must-have):** Plan mentioned the epsilon guard on StepClock but StepClock is stateless — it can't track what was already fired. The Sequencer is the correct owner of `last_step_fired_` to prevent re-firing the same step when two consecutive blocks both see the same step as "current."

4. **Test 6 ppq value imprecise (strongly recommended):** "ppq_position just before step 1 boundary" — imprecise. Fixed to 0.249 with computed expected sample offset ~88.

5. **Test 2 assertions imprecise (strongly recommended):** "sample_offset < block_size" was not enough — added exact expected range with math derivation.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | MidiBuffer approach unresolved | Task 2 action | Resolved: two scheduler_.process() calls (avoids merge), ensureSize(512) in prepareToPlay, midi_buffer_seq_.clear() at top of processBlock |
| 2 | generate() algorithm underspecified | Task 2 action | Full algorithm added: step_at_block_start → ppq_at_block_end → walk step boundaries → handle wrap-around → compute sample offset formula |
| 3 | Double-trigger without stateful tracking | Task 2 action | last_step_fired_ member added to Sequencer; skip if current_step == last_step_fired_ |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 4 | Test 6 imprecise ppq value | Task 3 action | Pinned to ppq=0.249; expected sample offset ≈88 with derivation |
| 5 | Test 2 imprecise assertions | Task 3 action | Case A + Case B with exact assertions and math |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 6 | ppq_at_block_end integer vs float accumulation for long runs | Relevant for 24h+ sessions; can use cumulative integer sample counter in Phase 5 Feel Engine |
| 7 | Step resolution configurability (1/16 hardcoded) | Phase 5 adds per-lane resolution; 1/16 fixed is correct for Phase 3 |

## 5. Audit & Compliance Readiness

- Algorithm now fully specified — reconstructable from plan alone without guessing.
- Double-trigger guard prevents stuck-on and phantom notes detectable in testing.
- Two-call approach avoids RT-unsafe merge allocation.

## 6. Final Release Bar

Build + ctest 17/17 + no double-trigger in 60s continuous run. Signed off post-upgrades.

---

**Summary:** Applied 3 must-have + 2 strongly-recommended upgrades. Deferred 2 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
