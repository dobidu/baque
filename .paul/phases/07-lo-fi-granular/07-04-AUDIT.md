# Enterprise Plan Audit Report

**Plan:** .paul/phases/07-lo-fi-granular/07-04-PLAN.md
**Audited:** 2026-06-08
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

Conditionally acceptable. The integration architecture is correct: GranularProcessor slot in FxChain after LoFi, append-only PLockParam extension, float→bool freeze conversion documented. However, the plan contained a self-contradicting boundary that would have caused a guaranteed test regression: LC4 in `test_lo_fi_chain.cpp` asserts `k_plock_param_count == 8`, but the plan simultaneously (a) adds 3 PLockParam entries making count=11 and (b) declares test_lo_fi_chain.cpp "DO NOT CHANGE". This is a release-blocking defect — the build would succeed but `ctest` would report LC4 FAIL, a regression the plan explicitly claims to prevent. Applied M1 fix. Two SR findings applied for behavioral documentation. Upgraded to acceptable.

---

## 2. What Is Solid

- **PLockParam append-only**: granular_spray=8, granular_pitch_spread=9, granular_freeze=10 appended after existing values 0–7. Existing p-lock data serialized with indices 0–7 is not affected. Correct.
- **FxChain process() order**: LoFi → Granular → LP → Reverb → Delay → Sidechain. Granular placed before LP filter is correct — LP filters the granular output, not the other way. Sidechain last (ducks full wet mix including reverb/granular) is correct.
- **granular_freeze float→bool `>= 0.5f`**: p-lock events carry float values; the threshold conversion is explicit and auditable. Values in [0.0, 0.5) = unfreeze; values in [0.5, 1.0] = freeze. P-lock at 0.0 or 1.0 (the natural values) both work correctly.
- **granular_.prepare() in FxChain::prepare() / granular_.reset() in FxChain::reset()**: correct lifecycle. reset() calls GranularProcessor::reset() (not prepare()) — appropriate since GranularProcessor has a dedicated reset() that clears buffers without reallocating.
- **GC3 fill phase pre-applied**: plan correctly specifies explicit FILL PHASE with separate buffers before measurement, same pattern as GR3/GR4 (07-03 audit M1). Not re-learning the same lesson. Correct.
- **ScopedJuceInitialiser_GUI in GC1/GC2/GC3, absent from GC4**: correct. FxChain instantiation requires JUCE init (juce::dsp Reverb/DelayLine/StateVariableTPT). GC4 only checks enum values — no JUCE init needed.
- **apply_plock_batch() silent swallow prevention**: plan adds 3 cases + existing `default: break` with warning comment. With 11 PLockParam values (0–10) all covered by explicit cases, silent swallow is impossible for any valid PLockParam value.
- **No SmoothedValue for granular params**: consistent with lo-fi (07-02): freeze/spray/pitch_spread are creative switches, not rampable automation targets. Adding SmoothedValue would incorrectly morph between freeze states.

---

## 3. Enterprise Gaps Identified

### Gap 1 — LC4 regression: boundary contradicts PLockParam change (Release-Blocking)

The plan boundary section lists `tests/test_lo_fi_chain.cpp` under "DO NOT CHANGE". Simultaneously, the plan extends PLockParam from count=8 to count=11. LC4 (test_lo_fi_chain.cpp:113) contains:
```cpp
REQUIRE(k_plock_param_count == 8);
```
After the PLockParam change, `k_plock_param_count == 11`. LC4 will FAIL. The plan's T1 verify `ctest -R "(GC|PL)" --output-on-failure` does not include LC4 (it targets "GC" and "PL" patterns, not "LC"), so this regression would be missed during T1 verification and only caught by T2's full suite. More critically: the DO NOT CHANGE boundary would prevent a developer from knowing they must update this file. This is a plan self-contradiction — both the boundary restriction and the PLockParam extension cannot be simultaneously satisfied.

Fix: Remove test_lo_fi_chain.cpp from DO NOT CHANGE; add to files_modified; add LC4 update to T1 action (`k_plock_param_count == 8` → `== 11`); add `ctest -R "LC4"` to verification checklist.

### Gap 2 — Granular "always-on" behavior undocumented (Strongly Recommended)

GranularProcessor always replaces the buffer with grain output — even at "neutral" params (spray=0, pitch_spread=0, freeze=false). This is semantically different from LoFiProcessor at neutral (bit_depth=16 → quantization transparent; sr_factor=1.0 → ZOH passthrough). At neutral granular params, the output is Hann-windowed grain reconstruction: amplitude varies with envelope, can sum >1.0 with overlapping grains, introduces a ~50ms latency from grain length.

Risk: A future maintainer sees `granular_spray=0, pitch_spread=0` and concludes "this is bypass mode". They then attempt to "optimize" by adding a bypass branch, or add SmoothedValue from some "bypass" default, disrupting grain state. The plan does not document that neutral params ≠ bypass.

Fix: Add comment in fx_chain.cpp granular section documenting always-on behavior.

### Gap 3 — FxParams granular_freeze comment imprecise (Strongly Recommended)

The plan specifies `float granular_freeze = 0.0f; // 0.0 = false, 1.0 = true`. The comment implies only 0.0 and 1.0 are meaningful values. But the actual fx_chain.cpp conversion uses `>= 0.5f`. A p-lock event could set granular_freeze to 0.3f (expecting no freeze) or 0.7f (expecting freeze). The comment "0.0 = false, 1.0 = true" fails to document the actual threshold — a maintainer reading only FxParams would not know that 0.7f activates freeze.

Fix: Update FxParams comment to `>= 0.5f = freeze on (float boolean convention)` to match the implementation.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | LC4 regression: `test_lo_fi_chain.cpp` boundary contradicts PLockParam count increase 8→11 — LC4 FAIL is guaranteed without update | `files_modified` frontmatter; `<files>` in T1 and T2; T1 `<action>`; `<boundaries>`; `<verification>` | Added `tests/test_lo_fi_chain.cpp` to files_modified + T1 files + T2 files; added LC4 update instruction to T1 action; removed test_lo_fi_chain.cpp from DO NOT CHANGE with audit comment; added `ctest -R "LC4"` to verification checklist. <!-- audit-added: M1 --> |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Granular always-on behavior undocumented — neutral params ≠ bypass; future maintainer risk of incorrect "bypass" optimization | T1 `<action>`: fx_chain.cpp granular block spec | Added 4-line comment to granular block in fx_chain.cpp spec: "granular sempre ativo mesmo com params neutros — output é reconstrução Hann-windowed, não passthrough. wet/dry deferred to Phase 10." <!-- audit-added: SR1 --> |
| 2 | FxParams granular_freeze comment imprecise: "0.0 = false, 1.0 = true" misses ≥0.5f threshold — 0.3f vs 0.7f behavior undocumented | T1 `<action>`: fx_params.h spec | Updated comment to `>= 0.5f = freeze on (float boolean convention); 0.0 = off`. Threshold now matches fx_chain.cpp conversion. <!-- audit-added: SR2 --> |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|------------------------|
| 1 | Granular wet/dry mix (no bypass path at spray=0/pitch=0/freeze=false) | SCOPE LIMITS explicitly defers to Phase 10 UI. Granular output at neutral params is perceptually close to input (grain reads from captured input, Hann envelope ≈ unity at overlap center). Audible but acceptable for v1 before UI. |
| 2 | apply_plock_batch() full dispatch coverage (cannot unit-test BaqueProcessor) | Established pattern from Phase 6-01 (LC4/PL6 same limitation). Enum plumbing verified by GC4; dispatch verified by smoke test crash behavior. GC4 + PL6 are the accepted evidence pattern for this project. |
| 3 | PLockBatch k_max=32 vs 11-param p-lock density (16 steps × 11 params = 176 possible events, overflow drops silently) | k_max=32 was designed for "some events per step" not "all params all steps". In practice, producers p-lock 1–3 params per step, not all 11. Hard cap upgrade deferred to Phase 12 (RT-safety hardening audit). |

---

## 5. Audit & Compliance Readiness

**Evidence produced:** 5 GC tests with [granular] tag + [dod] on GC5. GC1 (non-silent), GC2 (freeze via FxChain), GC3 (spray contrast via full chain), GC4 (PLockParam indices), GC5 (Phase 7 DoD marker). After M1 fix, LC4 regression is prevented; all 129 tests are expected green.

**Silent failure prevention:** `ctest -R "(GC|PL)"` now supplemented by explicit `ctest -R "LC4"` in verification checklist (audit M1). PLockParam switch cases now cover all 11 values — no silent swallow possible. granular_freeze threshold >= 0.5f documented in FxParams (audit SR2) and in fx_chain.cpp comment.

**Post-incident reconstruction:** FxChain process() order is documented in objective Output section and in code comments. Granular always-on is documented (SR1) to prevent future "why does granular affect signal at neutral params?" confusion. granular_.reset() in FxChain::reset() is distinct from lo_fi_ (which uses prepare()) — explained by GranularProcessor having a dedicated reset() method.

**Ownership:** GranularProcessor (07-03) is owned standalone; FxChain owns lifecycle (prepare/process/reset). PLockParam is append-only — no existing entries touched.

---

## 6. Final Release Bar

Before this plan ships:
- LC4 updated to k_plock_param_count == 11; passes in full suite
- GC1-GC5 pass: 129/129 total
- ctest -R "dod" green (Phase 7 DoD confirmed)
- clang-format clean on all 8 modified files
- fx_chain.cpp granular block has always-on comment (SR1)
- FxParams granular_freeze comment uses >= 0.5f threshold (SR2)

Remaining risks if shipped without M1 fix:
- LC4 FAIL = reported regression; CI would block merge; developer confused why test_lo_fi_chain.cpp broke
- Boundary instruction "DO NOT CHANGE test_lo_fi_chain.cpp" would actively guide developer AWAY from the fix

After M1 + SR1 + SR2: Plan is acceptable for APPLY.

---

**Summary:** Applied 1 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY.

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
