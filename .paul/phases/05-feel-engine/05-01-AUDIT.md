# Enterprise Plan Audit Report

**Plan:** `.paul/phases/05-feel-engine/05-01-PLAN.md`
**Audited:** 2026-06-05
**Verdict:** conditionally acceptable → upgraded

---

## 1. Executive Verdict

Plan is **conditionally acceptable**. Architecture is sound — FeelEngine's pre-allocated deferred queue is the correct RT-safe pattern, and the Sequencer integration approach (nullable params + backward compat) is clean. However, four defects would cause test failures or correctness bugs in production: (a) all Sequencer-based tests would produce no notes due to pattern switch mechanics, (b) the flush_deferred loop leaks stale notes forever causing queue overflow, (c) no ppq regression detection causes wrong notes after DAW loop restart, (d) no guard prevents RT allocation from large MIDI messages in defer(). After applying findings below, plan is approved for APPLY.

---

## 2. What Is Solid

- **Pre-allocated fixed array** (`DeferredNote deferred_[128]`) — correct RT-safe deferred queue; no allocation in steady state
- **Nullable feel params with default null** — perfect backward compat; existing 59 tests unaffected on generate() API change
- **`timing_ms_to_samples()` as static pure function** — correct design; no state, easily testable, no coupling
- **`flush_deferred` before note generation** — correct ordering; deferred notes from previous block flushed before new ones added
- **`vel_scale` clamped [1, 127]** — silence-prevention at lower bound is correct; note-on velocity=0 is spec-compliant "note off" in MIDI, so clamping to 1 is right
- **ppq regression note**: Plan's comment section correctly identifies the transport-restart risk; audit translated this to mandatory code

---

## 3. Enterprise Gaps Identified

**G1 — Test infrastructure broken (Sequencer pattern mechanics)**
`set_next_pattern()` stores to `next_pattern_` and arms `pattern_pending_`. The switch to `pattern_` only fires when `step == 0 && last_step_fired_ == 15`. At startup `last_step_fired_ = -1`. Calling `set_next_pattern()` then `generate(ppq=0)` immediately: step 0 fires, condition `0 == 15` is false, `pattern_` stays default (all inactive). No notes emitted. All 5 Sequencer-based tests (FE1, FE2, FE5, FE6, FE7) assert `REQUIRE(note_on_pos >= 0)` or equivalent — all fail. This is a silent infrastructure failure, not a logic error in FeelEngine itself.

**G2 — Queue leak in flush_deferred**
The `else` branch in `flush_deferred` re-queues ALL non-matching notes with comment "belongs to a future block". This is wrong: `d.target_sample < block_start` (past note) also hits this branch and is re-queued indefinitely. On DAW loop (without G3 fix), these accumulate. Queue fills to `k_max_deferred = 128`. Subsequent `defer()` calls drop new notes silently. Musical notes lost.

**G3 — No ppq regression detection**
`block_start_sample_` is a monotonically increasing counter in PluginProcessor. On DAW loop restart (ppq resets to loop start), `block_start_sample_` keeps incrementing. Deferred notes from the previous loop iteration have `target_sample` values still in the future (relative to `block_start_sample_`). They fire in the new iteration at wrong musical positions. This is a correctness bug: "Dilla Drunk" feel would fire extra ghost notes after every loop boundary.

**G4 — RT allocation risk in defer()**
`DeferredNote` contains `juce::MidiMessage`. `juce::MidiMessage` stores messages ≤7 bytes inline (on 64-bit). Note-on and note-off are 3 bytes — safe. But `defer()` accepts any `juce::MidiMessage`. If a MIDI CC, program change, or (in Phase 9) sysex message is passed, the copy in `deferred_[deferred_count_++] = {msg, ...}` calls `juce::MidiMessage`'s copy constructor, which heap-allocates for data >7 bytes. No guard exists. Phase 9 MIDI expansion is the obvious risk vector.

**G5 — FE7 test verifies count, not correctness**
`REQUIRE(positions.size() >= 16)` passes if all 16 notes fire at position 0. This would also pass if the feel offsets were silently ignored. The test does not verify the core claim of AC-7: "each note samplePosition matches expected offset ± 2 samples".

**G6 — FE3 doesn't verify deferred content**
FE3 checks `deferred_count() == 1` but doesn't verify the deferred entry is a note-on for note 36. If only the note-off was deferred (e.g., prev_step logic fires note-off into deferred), the test passes but the deferred queue contains wrong data.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | All Sequencer-based tests broken — `set_next_pattern` not immediate | Task 2.1 (sequencer.h), Task 3.4 (all test cases) | Added `Sequencer::set_pattern(const StepPattern&)` for immediate set; changed all 7 `set_next_pattern` calls in tests to `set_pattern`; added `set_pattern()` impl in Task 2.2 |
| M2 | `flush_deferred` queue leak — past notes re-queued forever | Task 1.3 (feel_engine.cpp) | Split `else` into two branches: `target >= block_end` → keep (future); `target < block_start` → jassertfalse + discard (stale) |
| M3 | No ppq regression detection — stale deferred notes after loop | Task 2.1 (sequencer.h private), Task 2.2 (sequencer.cpp) | Added `double last_ppq_ = -1.0` to Sequencer; added ppq regression check at top of generate() — calls `feel_engine->prepare()` on backward jump |
| M4 | `defer()` no RT-allocation guard for large MIDI messages | Task 1.3 (feel_engine.cpp) | Added `jassert(msg.getRawDataSize() <= 3)` at top of `defer()` — fails fast in debug if oversized message passed |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | FE7 test verifies count only, not positions | Task 3.4 (FE7 test body) | Replaced `REQUIRE(positions.size() >= 16)` with `REQUIRE(size == 16)` + position verification loop: computes `expected = round(i * step_samples) + timing_ms_to_samples(...)` per step and checks `actual ≈ expected ± 3 samples` |
| SR2 | FE3 doesn't verify deferred content (note number) | Task 3.4 (FE3 test body) | Added second phase: calls `engine.flush_deferred(next_buf, 4096, 8192)` to simulate the next block and verifies note-on #36 appears — confirms correct message was deferred |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | `add_with_feel` lambda redefined in step loop | Pure style; compilers inline lambdas; zero runtime impact; refactor is pure cleanup with no correctness value |
| D2 | Base velocity 100 hardcoded — no TODO for p-locks integration | Phase 6 is the right place; Phase 5 has no per-step velocity in StepPattern; adding a comment would be noise at this stage |
| D3 | `FeelPattern` thread safety not atomic | Correctly scoped to Phase 10 (UI); Phase 5 has no UI; changing now would over-engineer before the UI design is known |

---

## 5. Audit & Compliance Readiness

**Evidence quality:** Tests FE1–FE8 (after SR1/SR2 fixes) produce falsifiable assertions. FE7 now verifies position accuracy, not just presence. Deterministic tests with known BPM/SR → computed expected values; reproducible across platforms.

**Silent failures:** M2 (queue leak) was a silent correctness failure — wrong notes fired, queue eventually overflowed, notes dropped. Now replaced with explicit jassertfalse + discard. M4 (RT alloc) now fails loudly in debug if violated.

**Post-incident reconstruction:** `jassertfalse` in flush_deferred (stale note) and in defer() (oversized message) provide debug-build diagnostic hooks. FeelEngine state (`deferred_count()`) is inspectable.

**Ownership:** FeelEngine is owned by PluginProcessor exclusively. Single-writer contract established. No shared ownership, no mutex needed.

---

## 6. Final Release Bar

**Must be true before plan ships:**
- `Sequencer::set_pattern()` implemented and tested
- `flush_deferred` discards past notes with jassertfalse (not re-queues)
- ppq regression detection clears deferred queue on backward jump
- `defer()` guards against oversized messages
- FE7 verifies positions, not just count
- All 67 tests pass; clang-format clean

**Remaining risks if shipped as-is (pre-audit):**
- FE1/FE2/FE5/FE6/FE7 all fail → 0% test coverage on the core path
- Queue leak on DAW loop → ghost notes + note loss in production
- No ppq regression → wrong notes after every loop in every DAW

**Sign-off:** After all 4 must-have + 2 strongly-recommended upgrades applied, plan is approved for APPLY.

---

**Summary:** Applied 4 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
