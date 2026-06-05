# Enterprise Plan Audit Report

**Plan:** `.paul/phases/04-sample-engine/04-03-PLAN.md`
**Audited:** 2026-06-05
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable — two must-have defects applied before APPLY is safe.**

The plan is structurally well-formed: correct scope, RT-safety boundary is clear, offline-only
classification is explicit, and the single-writer UAF contract is carried forward correctly from
04-01. However, two silent-failure paths make the plan incorrect as written:

1. `chop_to_pads(nullptr, n, data, 2048, {0})` triggers UB (no null guard on `data`)
2. `chop_to_pads` called twice leaves stale pad data silently — AVOID annotation in original plan
   actively prevented the fix

Both issues were auto-applied. Plan is now acceptable for APPLY.

---

## 2. What Is Solid

- **`pool.reset_all()` first in `chop_to_pads`:** Correctly enforces single-writer contract at the
  API boundary. Any future caller in Phase 10 UI that forgets will still be protected.
- **`detect_onsets` null/zero guard:** Correctly prevents dereference of null data in the detection
  path. Position 0 always returned — at least one slice invariant holds regardless of input.
- **HWR delta algorithm:** Half-wave rectification (only positive energy increases) is the correct
  onset model. Negative deltas (energy decay) don't fire spurious detections.
- **Always-prepend position 0:** Ensures first slice always starts at sample 0. Avoids the class
  of bug where all detections are missed and nothing loads.
- **Max 16 cap hardcoded to `PadBank::k_num_pads`:** Correct reference to the canonical constant.
  No magic numbers.
- **`if (length <= 0) continue` guard:** Prevents zero/negative-length `setSize` from reaching JUCE.
- **clang-format in verification checklist + ASCII-only test name constraint documented.**

---

## 3. Enterprise Gaps Identified

### Gap A — `chop_to_pads` crashes on null `data` (MUST-HAVE)

`detect_onsets` has `if (data == nullptr || num_samples <= 0) return {0}` — correct.
`chop_to_pads` had no equivalent guard. With `data=nullptr`, `juce::FloatVectorOperations::copy`
receives a null source pointer. The `if (length <= 0) continue` guard only fires for empty slices,
not for null data with valid onset positions. This is UB in production and a crash under sanitizers.

### Gap B — Stale pad data silently persists after second chop (MUST-HAVE)

The original plan's AVOID section explicitly stated:
> "Resetting pads beyond `n` (untouched pads preserve their previous content)"

This is incorrect. On second call with fewer onsets, old slices persist in higher pads. The user
expects a fresh chop — "chop this new sample" should not leave samples from the previous chop
playing on pads 3–15. This is a silent correctness bug: no crash, wrong audio output, no error.

### Gap C — Unsorted onset contract not asserted (STRONGLY RECOMMENDED)

`chop_to_pads` accepts any `std::vector<int>` without validating sort order. If a future UI caller
constructs manual chop points in wrong order, `length = end - start` goes negative, the guard skips
it, and pad content is lost silently. `jassert` in debug mode surfaces this immediately vs. shipping
with silent data loss that's hard to reproduce.

### Gap D — No test for `min_slice_ms` enforcement (STRONGLY RECOMMENDED)

The `frame_start - last_onset_sample >= min_spacing` branch is the sole quality gate preventing
over-fragmentation. No test verifies it fires correctly. The branch could be removed, inverted, or
have an off-by-one without any test catching it. This is a non-trivial algorithm guard and deserves
direct test coverage.

### Gap E — No test for stale pad clearing after second chop (STRONGLY RECOMMENDED)

Gap B's fix (clearing pads beyond n_slices) needs a regression test. Without it, any future
refactor that re-introduces the bug passes all tests. The fix is one-line but the regression path
is silent — only a specific "second chop with fewer pads" scenario reveals it.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | `chop_to_pads` crashes on null `data` | Task 1 `chop_to_pads` action | Added null guard at entry: `if (data == nullptr \|\| num_samples <= 0) { pool.reset_all(); return; }` |
| 2 | Stale pad data after second chop | Task 1 `chop_to_pads` action, AVOID section | Removed incorrect AVOID directive; added clear loop `for (int i = n; i < k_num_pads; ++i) bank.pad(i).sample_buffer().setSize(0, 0);` |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 3 | No `jassert` for sorted onset contract | Task 1 `chop_to_pads` action | Added `jassert(std::is_sorted(onsets.begin(), onsets.end()))` |
| 4 | No test for `min_slice_ms` enforcement | Task 2 tests + AC | Added AC-5 (min_slice_ms suppresses close onsets) + Test S9 |
| 5 | No test for stale pad clearing | Task 2 tests + AC | Added AC-4 (stale pads cleared on re-chop) + Test S10 |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 6 | Test S2 tolerance window (768..1280) could be tighter | Not incorrect — 1024 is inside range; algorithm correctness verified by exact signal construction |
| 7 | Adaptive threshold (spectral flux, perceptual weighting) | Energy HWR delta sufficient for v1 chop use case; advanced detection is Phase 7+ |
| 8 | Multi-channel / stereo slicing | SamplePad is mono by convention (04-01 decision); stereo path deferred to Phase 7 |

---

## 5. Audit & Compliance Readiness

- **Silent failures:** Two silent failure modes (null crash, stale pads) now guarded by must-have fixes.
- **Contract enforcement:** `jassert` for sorted onsets provides debug-mode signal on contract violation;
  release builds still tolerate invalid input without crash (length guard catches it).
- **Regression coverage:** S10 ensures stale-pad clearing regresses are caught by CI.
- **Algorithm coverage:** S9 ensures min_slice_ms branch is exercised independently of the integration tests.
- **Audit trail:** `pool.reset_all()` call is self-documenting and traceable. No side effects beyond
  in-process buffer writes. Offline-only path: no external APIs, no I/O, no allocations on audio thread.

---

## 6. Final Release Bar

**Before shipping:**
- Must-have fixes 1 and 2 applied (done)
- All 10 tests pass including S9 + S10 (new)
- Second-chop stale-pad scenario manually exercised

**Remaining risks if shipped with these fixes:**
- `jassert` fires in debug but is silent in release with unsorted onsets — acceptable given offline
  path and single-caller scenario for v1
- Onset detection quality is energy-only; complex signals (pitched bass, cymbals) may under-detect.
  Acceptable — this is Phase 4 first-pass, advanced detection deferred.

**Would I sign off on this plan with the applied fixes?** Yes.

---

**Summary:** Applied 2 must-have + 3 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.
Test count updated: 8 → 10. AC count: 3 → 5.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
