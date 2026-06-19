---
phase: 12-hardening
plan: 02
audit_date: 2026-06-19
auditor: senior-principal-engineer
plan_approved: true
findings_applied: [M1, SR1, SR2]
findings_deferred: [D1, D2]
type: Audit
about: "BAQUE"
---

# Plan 12-02 Audit Report

**Verdict: APPROVED with must-have and strongly-recommended findings applied.**

Plan modified in place before APPLY. All findings below were auto-applied.

---

## Audit Summary

| Section | Rating | Notes |
|---------|--------|-------|
| Scope clarity | Pass | Single responsibility: pluginval strictness 10 + APVTS unit test |
| Acceptance criteria | Pass | 3 clear GWT scenarios with measurable outcomes |
| Task decomposition | Pass | 2 tasks, clean separation |
| Boundary conditions | Pass | Explicit no-go list; pluginval version pinned |
| Verification steps | Conditional | SR2 fixed: exit code capture was broken |
| Test coverage | Conditional | M1 fixed: P12D2 missing DSP boundary smoke |

---

## Findings

### M1 — MUST HAVE (APPLIED): P12D2 missing DSP boundary smoke

**Severity:** Must-have — pluginval strictness 10 explicitly runs `processBlock` with all params at normalized 0.0 and 1.0 (automation test). P12D2 as written only tests parameter metadata (names, ranges, defaults). A DSP failure at extremes (NaN/Inf at `filter_cutoff=20Hz`, `scatter_type=10`, `gate_depth=1.0`, `delay_time=0.001s`) would not surface until the pluginval binary run in Task 2 — harder to debug, slower iteration.

**Root cause:** Plan conflated "parameter contract" (metadata) with "DSP stability at automation boundaries" (behavioral). These are two distinct invariants.

**Applied:** Added P12D2b TEST_CASE to Task 1 Step 3. P12D2b sets all params to normalized 1.0, runs 50 blocks, asserts finite output; then resets to 0.0 and repeats. Test count updated 253 → 254 throughout plan, verify, and success_criteria.

**Rationale for split (P12D2 + P12D2b vs single test):** Keeping them separate preserves failure locality — a metadata failure names the bad param (via `INFO` macro), while a DSP failure locates to the boundary condition. Merging would produce a wider, harder-to-diagnose test.

---

### SR1 — STRONGLY RECOMMENDED (APPLIED): `isSynth()` not asserted

**Severity:** Strongly recommended — BAQUE is a MIDI-triggered drum machine and must present as an instrument (synth), not an audio effect. pluginval uses `isSynth()` to select its test suite. Effect-mode testing includes "output = f(input)" assertions that BAQUE cannot satisfy (it ignores input audio and generates sound from MIDI). If `PLUGIN_IS_SYNTH` is missing or set incorrectly in CMakeLists, `isSynth()` returns false and pluginval runs the wrong suite.

**Applied:** Added `REQUIRE(proc.isSynth());` to P12D2 with comment `// Drum machine must be a synth, not an effect; pluginval picks test suite based on this`. This documents the invariant and catches misconfiguration at unit-test time rather than during the pluginval binary run.

---

### SR2 — STRONGLY RECOMMENDED (APPLIED): Task 2 verify step exit code capture broken

**Severity:** Strongly recommended — the original verify block had two bugs:

1. `echo "Exit code: $?"` appeared AFTER `grep -c ... || echo "0 failures"`, capturing grep's exit code (not pluginval's). `$?` is consumed immediately; stale after any intermediate command.
2. The pluginval run in Step 1 used `2>&1 | tee ... | tail -20`, which routes output through a pipe. `$?` after a pipeline captures the last command's exit code (`tail`, always 0), masking pluginval failures entirely.
3. `grep -c "FAILED\|ERROR"` returns exit code 1 when no matches (zero failures = success). This triggers the `||` branch and prints `"0 failures"` — visually looks like a fallback/error path when it's actually the passing case.

**Applied:**
- Step 1 run command changed from `| tee ... | tail -20` to `> /tmp/pluginval-10/run.log 2>&1 ; echo "pluginval exit: $?" ; tail -20 /tmp/pluginval-10/run.log`. `$?` now captures pluginval directly (no pipe).
- Verify block changed from `grep -c "FAILED\|ERROR" ... || echo "0 failures"` to `grep -E "(PASSED|FAILED|Tests completed|Validation)" /tmp/pluginval-10/run.log | tail -10`. This shows the actual test result lines without misleading exit-code logic.

---

### D1 — DEFERRED: `isBusesLayoutSupported()` explicit mono rejection

**Classification:** Can safely defer — JUCE default `isBusesLayoutSupported()` accepts any layout without restriction. BAQUE should accept only stereo output (mono input + stereo output). pluginval strictness 10 tests bus layout combinations but does not fail on missing mono rejection for synth-type plugins (synths generate output from MIDI, not from audio input). Risk: a DAW using BAQUE in a mono chain gets stereo output silently summed to mono — not a crash, just a routing surprise.

**Defer to:** Phase 13 release prep, where bus layout is audited against target DAWs.

---

### D2 — DEFERRED: Automation smoothing artifact validation

**Classification:** Can safely defer — validating that no zipper noise occurs during parameter value transitions during `processBlock` is beyond unit test scope. The FX chain uses `SmoothedValue` (added in earlier phases). pluginval strictness 10 does not detect zipper noise; this requires manual A/B listening or a dedicated audio quality test rig. Not a correctness issue for shipping v1.

**Defer to:** Manual QA, Phase 13.

---

## Impact on Test Count

| State | Count |
|-------|-------|
| Pre-audit (252 existing + P12D1) | 252 |
| After 12-01 APPLY | 252 |
| After 12-02 APPLY (P12D2 + P12D2b) | **254** |

---

## Files Modified by Audit

| File | Change |
|------|--------|
| `.paul/phases/12-hardening/12-02-PLAN.md` | Applied M1 (P12D2b), SR1 (isSynth assert), SR2 (exit code fix); updated all 253 → 254 references |

---

*Audit performed: 2026-06-19*
*Built with PAUL Framework v1.4*
