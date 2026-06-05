# Enterprise Plan Audit Report

**Plan:** .paul/phases/04-sample-engine/04-04-PLAN.md
**Audited:** 2026-06-05
**Verdict:** conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. Core algorithm design (input snapshot before setSize, pool.reset_all() before mutation, setTempo not setRate, do/while drain loop) is correct and production-safe. Two compile-breaking defects found in test code that would prevent the build from succeeding. Both applied. Plan ships after fixes.

---

## 2. What Is Solid

- **Input snapshot pattern**: explicit documentation and AVOID note against reading `pad.data()` after `setSize`. Correct and prevents UAF.
- **pool.reset_all() placement**: called before buffer mutation, after guards, matching single-writer contract from 04-01. Guards (empty pad, ratio 1.0/invalid) correctly skip reset — no-op calls don't kill voices.
- **setTempo vs setRate**: AVOID section explicitly calls out this distinction. WSOLA-only path preserved.
- **Drain loop**: `do { } while (nReceived > 0)` is the correct SoundTouch drain pattern. Will not terminate early.
- **Output size estimate headroom**: `num_input / tempo_ratio * 1.15 + 512` — appropriate WSOLA headroom.
- **Stack-allocated SoundTouch instance**: fresh `soundtouch::SoundTouch st` per call — no state leak between invocations.
- **GIT_SHALLOW TRUE**: correct for FetchContent; avoids fetching full history.
- **Offline-only constraint**: documented in header comment and boundaries — clear RT safety separation.
- **Test buffer size guidance**: AVOID("< 4096 samples") explicitly noted — WSOLA constraint surfaced.

---

## 3. Enterprise Gaps Identified

### G1 — Cross-file fixture reference (compile error)
`test_time_stretch.cpp` uses `SlicerJuceFixture` which is defined in `test_slicer.cpp`, a separate translation unit. This is not a header — the struct is not reachable from `test_time_stretch.cpp`. Build will fail with "undeclared identifier: SlicerJuceFixture". Plan inconsistently notes "or define own JUCE fixture" in T1 but drops the caveat in T2–T5.

### G2 — [[nodiscard]] return ignored in T3 (compiler warning)
`pool.trigger_at()` is declared `[[nodiscard]]`. T3 calls it without capturing the return. Clang and GCC will emit `-Wnodiscard` warning. Plan success_criteria requires "no new compiler warnings" — this violates it. Will block clean CI.

### G3 — Silent failure on empty SoundTouch output
`if (output.empty()) return;` silently exits after `pool.reset_all()` already killed active voices. Result: voices silenced for a pad that still has valid unmodified data. User action (stretch) appears to succeed but playback is dead until re-trigger. No diagnostic, no jassert — invisible in debug builds.

### G4 — No minimum input guard before SoundTouch processing
The plan notes "< 4096 samples" is "very short" for WSOLA (in AVOID section), but `apply()` has no guard. A pad with e.g. 64 samples will run through the full SoundTouch pipeline and likely produce empty output, hitting G3 silently. The guard must be in production code, not just test guidance.

### G5 — T3 buffer at exact AVOID boundary (4096 samples)
T3 loads `4096` samples — the same threshold flagged as "very short" in the AVOID section. At ratio 0.5, WSOLA will struggle to produce output; test may flake on slower CI or different SoundTouch build configs. The AVOID guidance recommends "8192 which is safe" — T1/T2 follow this, T3 doesn't.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | G1: SlicerJuceFixture cross-file compile error | Task 3 action — test file definition | Added `struct TimeStretchJuceFixture { juce::ScopedJuceInitialiser_GUI init; };` before helper; replaced all 5 `SlicerJuceFixture f;` instances with `TimeStretchJuceFixture f;` |
| 2 | G2: [[nodiscard]] trigger_at return ignored in T3 | Task 3 action — T3 test body | Changed `pool.trigger_at(...)` to `(void)pool.trigger_at(...)` with audit comment |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 3 | G3: Silent empty-output failure — voices killed, no diagnostic | Task 2 action — apply() drain section | Added `jassert(!output.empty());` before `if (output.empty()) return;` with explanatory comment |
| 4 | G4: No minimum input guard before SoundTouch processing | Task 2 action — apply() body; AC-4 | Added `if (num_input < 256) return;` after snapshot; added AC-4 case (d) |
| 5 | G5: T3 buffer at WSOLA boundary (4096 → 8192) | Task 3 action — T3 test body | Changed `load_sine_pad(bank.pad(0), 4096)` to `load_sine_pad(bank.pad(0), 8192)` |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | Checkpoint Task 1 CMakeLists template omits `cpu_detect_x86.cpp` — diverges from actual fork | Fork already created and verified (includes cpu_detect_x86.cpp); template is historical documentation only; actual FetchContent will pull correct fork |
| 2 | No upper bound on input sample count — large samples allocate proportionally | Expected behavior for a sample engine; not in audio thread; document in header if needed for Phase 10 |

---

## 5. Audit & Compliance Readiness

**Compile failures caught**: G1 (undefined struct) and G2 (nodiscard warning → CI fails) would have caused build failure on first APPLY attempt. Caught and fixed.

**Silent failure prevention**: G3/G4 add a jassert and early return. Post-incident reconstruction: jassert fires in debug build, identifies the code path. Empty-output case is now surfaced.

**Single-writer contract**: pool.reset_all() placement documented and correct. No race window between snapshot and buffer mutation.

**Test coverage after fixes**: 5 tests cover AC-1/2/3/4. AC-4(d) added for the new guard.

**No RT violations**: apply() never called on audio thread — enforced by scope (offline bake-in) and documented in header comment.

---

## 6. Final Release Bar

Before this plan ships:
- [ ] G1 and G2 fixes applied to test file during APPLY
- [ ] G3/G4 guards in time_stretch.cpp
- [ ] All 5 tests pass, no new warnings

Remaining risks if shipped as-is (pre-audit): Build failure on first attempt (G1, G2). Silent voice kill on stretch failure (G3).

Sign-off: Conditional. Applied fixes make this production-safe for Phase 4 bake-in use case.

---

**Summary:** Applied 2 must-have + 3 strongly-recommended upgrades. Deferred 2 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
