# Enterprise Plan Audit Report

**Plan:** .paul/phases/03-sequencer-base/03-02-PLAN.md
**Audited:** 2026-06-05
**Verdict:** Conditionally acceptable → upgraded → ready for APPLY

---

## 1. Executive Verdict

Conditionally acceptable. Two data races would crash or corrupt audio in production: `next_pattern_` cross-thread write without synchronization (512-byte struct write from message thread while audio thread may copy it), and `bool pattern_pending_` non-atomic read/write. Also: memory_order for `std::atomic<float> swing_amount_` was unspecified, leaving an ambiguity that could generate subtle inter-thread visibility bugs. All applied. Approved post-upgrades.

## 2. What Is Solid

- NoteTracker design — `lane → last_triggered_note` is exactly right for Phase 3 (one note per lane). Simple, correct, no allocation.
- Swing formula `(swing - 0.5) * 2 * step_duration` — mathematically correct MPC-style even-step delay.
- Pattern swap deferral to bar boundary — the right UX; mid-pattern switches are jarring.
- Boundaries explicitly exclude song mode, polymeter — correctly scope-controlled.

## 3. Enterprise Gaps Identified

1. **Data race on next_pattern_ (must-have):** `set_next_pattern()` on message thread writes a 512-byte `StepPattern` while `generate()` on audio thread may concurrently copy it (when pattern_pending_ appears true). C++ memory model: concurrent non-atomic writes to a struct from one thread + reads from another = undefined behavior. No lock, no atomic wrapper, no fence specified.

2. **Non-atomic pattern_pending_ (must-have):** `bool pattern_pending_` read on audio thread + written on message thread = data race on a non-atomic type. UB under C++ standard; torn reads on some architectures.

3. **memory_order for swing_amount_ unspecified (must-have):** `std::atomic<float>` was correctly chosen but the plan didn't specify which memory_order. Without guidance, implementers default to seq_cst (expensive, unnecessary fence on audio thread) or relax incorrectly.

4. **Pattern switch condition ambiguous (must-have):** Two conditions described: "step == 0 AND pattern_pending_" AND "step index transitions from 15 → 0" AND "ppq mod 4.0 < epsilon." Three different algorithms produce different behavior. Raw "step == 0" can fire on the FIRST block if ppq starts near 0 without a prior step 15.

5. **NoteTracker misleading title (strongly recommended):** Objective said "per-note active voice map (note number → voice)" — this is not what the implementation builds. Mismatch between spec title and implementation would confuse future maintainers adding per-voice tracking.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Data race on next_pattern_ | Task 2 action | Write next_pattern_ first, THEN store pattern_pending_ with memory_order_release |
| 2 | Non-atomic pattern_pending_ | Task 2 action | Changed to std::atomic<bool> pattern_pending_{false} |
| 3 | memory_order unspecified for swing | Task 1 action | relaxed store on message thread, relaxed load on audio thread (correct: single value, no ordering dependency) |
| 4 | Pattern switch condition ambiguous | Task 2 action | Use step 15→0 transition via last_step_fired_ from 03-01 (not ppq epsilon, not raw step==0); acquire load pairs with release store |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 5 | NoteTracker misleading title | Task 1 action | Clarified: "lane → last_triggered_note" not "note → voice" |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 6 | Swing range > 75% protection | UI constrains it; clamp in set_swing() already specified |
| 7 | First-block pattern switch edge case (ppq=0.0 at startup) | Benign: no step 15 fired yet → last_step_fired_==-1 → pattern swap won't trigger on first block; correct |

## 5. Audit & Compliance Readiness

- Cross-thread data races eliminated: pattern swap uses acquire/release, swing uses relaxed atomic.
- Pattern switch determinism: step 15→0 transition is unambiguous, not subject to floating-point epsilon.

## 6. Final Release Bar

No data races (verified by ThreadSanitizer in Phase 12 hardening); pattern switch + swing audible by ear in Standalone. Signed off post-upgrades.

---

**Summary:** Applied 4 must-have + 1 strongly-recommended upgrades. Deferred 2 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
