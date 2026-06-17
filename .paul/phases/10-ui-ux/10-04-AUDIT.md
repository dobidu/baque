# Enterprise Plan Audit Report

**Plan:** .paul/phases/10-ui-ux/10-04-PLAN.md
**Audited:** 2026-06-16
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable — would not approve as-is; approved after 3 upgrades applied.**

The plan is structurally sound for a read-only visualizer. The data-access patterns follow established codebase precedent (`current_pattern()` advisory read contract). The timer-gated visibility pattern and explicit destructor are correctly specified. However: one Must-Have (missing standard library includes — Windows build failure on CI), one Strongly Recommended code smell (`is_playing_` dead member), and one Strongly Recommended test gap (no automated exercise of the node-drawing code path). All three applied.

## 2. What Is Solid

- **Timer-gated visibility pattern**: `visibilityChanged()` starts/stops, `~dtor() { stopTimer(); }` explicit. Consistent with 10-03 established pattern (PadGrid, Sequencer). Correct.
- **M1 single-read per paint**: All three advisory reads (`current_pattern()`, `current_feel_pattern()`, `ui_snapshot().bpm`) explicitly snapshotted once at the top of `paint()`. Correct.
- **Advisory read contract**: `current_feel_pattern()` returns `FeelPattern` by value with correct documentation matching `current_pattern()` precedent. Not atomic — correctly documented and consistent with codebase accepted risk.
- **Playhead stopped-state guard**: `if (current_step_ >= 0)` is the correct guard — `UiStateSnapshot::current_step` is documented as `-1 when stopped`. No separate `is_playing_` check needed.
- **nodePos() BPM formula**: `ms_per_step = 60000 / (bpm * 4)` is correct for 16th-note steps at given BPM. `jlimit(±k_deg_per_step)` clamps offset to ±22.5° (one full step width) — prevents nodes crossing adjacent step positions.
- **`feel.steps[step]` indexing**: `step` (0-15) indexes `FeelPattern::steps[16]` correctly — timing is per-column, not per-lane.

## 3. Enterprise Gaps Identified

### Gap 1 (Must-Have): Missing explicit `<cmath>` and `<algorithm>` includes
`feel_field_component.cpp` uses `std::cos`, `std::sin` (from `<cmath>`) and `std::min` (from `<algorithm>`). Neither is listed in the plan's include specification for the `.cpp`. BAQUE's established pattern (see `sequencer_component.cpp:4`) requires explicit standard library headers — not implicit JUCE transitive includes. MSVC resolves headers differently than GCC/Clang; this is a Windows CI build failure waiting to happen. 3-platform CI matrix means the failure surfaces on first Windows run.

### Gap 2 (Strongly Recommended): `is_playing_` is a dead member variable
The plan declares `bool is_playing_ = false;` in the header and reads it in `timerCallback()`, but it is never referenced in `paint()`. The `if (current_step_ >= 0)` guard already correctly handles the stopped state (per `UiStateSnapshot` comment: `current_step == -1 when stopped`). Dead member variables in timer-driven UI components mislead future maintainers into believing they need to stay in sync, may prompt incorrect "fix" attempts, and represent unfinished thinking. Should be removed entirely.

### Gap 3 (Strongly Recommended): No automated test exercises the node-drawing code path
FEEL1 only tests timer gating (construction + visibility). The most complex code in the component — `nodePos()` geometry (trigonometry, step offset, per-lane ring assignment) — has zero automated coverage. If the `nodePos()` formula is broken on refactor, or if `feel.steps[step]` indexing regresses, only human-verify catches it (too slow, too late). A paint smoke test with one active step covers the full node-drawing branch, including bounds checks.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | Missing `<cmath>` + `<algorithm>` explicit includes in feel_field_component.cpp | Task 2 `<action>`, Task 2 `<verify>`, `<verification>` checklist | Added explicit include block before Constructor section; added grep checks to verify section |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | `is_playing_` dead member | Task 2 `<action>` header snippet + timerCallback snippet | Removed `bool is_playing_ = false;` from header; removed `is_playing_` load from timerCallback; added explanatory comment; added `grep -c "is_playing_" h # must be 0` to verify |
| SR2 | No automated test for node-drawing path | Task 3 `<action>`, Task 3 `<done>`, `<verification>`, `<success_criteria>` | Added FEEL2 paint smoke test with active pattern + off-screen Graphics render; test count updated 230→231 |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | `current_feel_pattern()` advisory read lacks C++ atomic protection | Same contract as `current_pattern()` — established codebase accepted risk; documented in comment; defer to Phase 10-07 atomicization sweep |
| D2 | `std::atomic<float>` lock-free guarantee not verified per-platform | In practice JUCE targets (x86/ARM64) are hardware-atomic for 4-byte floats; formal verification deferred to platform validation |
| D3 | Zero-size component before `setSize()` not guarded | `max_r = 0` → all drawing is no-op; no crash risk; no guard needed |

## 5. Audit & Compliance Readiness

**Build determinism**: M1 fix ensures platform-consistent compilation across 3-platform CI matrix. Without explicit includes, Windows build failure is non-deterministic (depends on MSVC header pull-in version).

**Silent failure prevention**: SR1 removes a dead variable that looks like an unfinished state guard — a future maintainer might add `if (is_playing_)` checks that bypass the correct `current_step_ >= 0` logic, causing a latent UI bug.

**Regression coverage**: SR2 ensures the most complex code path (node geometry + step loop + velocity calculation) has automated smoke coverage. One missed include or bounds error in `nodePos()` will crash the test rather than silently show wrong visuals.

**Post-incident reconstruction**: Timer-gated visibility pattern + explicit destructor follows audited pattern from 10-03 (SUMMARY documented). No new patterns introduced.

**Ownership**: PerformScreen remains sole focus authority. FeelFieldComponent is read-only (no `push_ui_command`). No new mutation paths.

## 6. Final Release Bar

**Must be true before shipping:**
1. `feel_field_component.cpp` includes `<cmath>` and `<algorithm>` explicitly
2. `is_playing_` removed from header and timerCallback
3. FEEL1 + FEEL2 pass (231 total tests)
4. clang-format clean
5. Standalone builds on all 3 CI platforms

**Risks if shipped as-is (pre-upgrade):**
- Windows CI build fails on `std::cos`/`std::sin`/`std::min` symbol not found (M1)
- Dead `is_playing_` member creates maintenance hazard (SR1)
- nodePos() geometry regression invisible to automated testing (SR2)

**Sign-off**: I would sign this plan post-upgrade. The component is read-only, timer-gated, structurally follows audited 10-03 patterns. Risk profile after upgrades is low.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
