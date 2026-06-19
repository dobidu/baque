# Enterprise Plan Audit Report

**Plan:** .paul/phases/11-presets-library/11-01-PLAN.md
**Audited:** 2026-06-18
**Verdict:** Conditionally Acceptable → Upgraded

---

## 1. Executive Verdict

**Conditionally acceptable, now upgraded.** Architecture is sound: suspendProcessing bracket preserved (10-01 M1 lesson), backward-compat path via invalid-ValueTree guard is correct, PresetManager abstraction is clean and decoupled. Three test gaps were release-blocking; two more findings were strongly recommended. All five applied. Plan is ready for APPLY.

Would I sign my name to this system after the upgrades? Yes, with the caveat that PLock round-trip is exercised only indirectly (no dedicated P11D_plock test) — accepted as a can-safely-defer given the overall AC-1 coverage.

## 2. What Is Solid

- **suspendProcessing bracket preserved throughout** — 10-01 M1 lesson applied correctly; both getStateInformation and setStateInformation bracket their state mutation. load_sample_from_file() is called inside the bracket on restore — message thread, audio suspended, safe buffer mutation.
- **getChildWithName() invalid-ValueTree guard for AC-4** — no explicit version check needed; missing subtrees return invalid ValueTree → `if (valid)` branches silently skip. Correct pattern.
- **juce::jlimit bounds on all scalar fields** — gain, pan, pitch_semitones, pitch_cents, ADSR are all clamped on restore. Defensive reads from untrusted ValueTree.
- **PresetManager wraps getStateInformation/setStateInformation** — no coupling to plugin internals beyond processor reference. Single responsibility.
- **Magic header format "BQP1"** — length-prefixed metadata before binary state is forward-compatible; unknown-magic fallback treats file as raw state (legacy/naked blobs still load).
- **juce::TemporaryFile for P11D4** — correct RAII; file exists for duration of test, cleaned up on destruction.

## 3. Enterprise Gaps Identified

### M1: P11D3 pollutes real user preset dir AND has vacuous CI path
`pm.save("test_groove", "custom")` always writes to `userApplicationDataDirectory/BAQUE/presets/` — never to `tmp_dir` (which is created but never used). The `preset_file` variable is assigned but never written; it is a ghost variable. Two consequences:
- In dev environment: permanently leaves `test_groove.bqpreset` in user's actual preset directory. `tmp_dir.deleteRecursively()` only deletes tmp_dir, not the preset.
- In CI: `userApplicationDataDirectory` is often unwritable → `list_user_presets()` returns empty → `found=false` → `WARN` path → test passes without exercising anything. Silent CI green on untested code.

### M2: P11D2 is vacuous
Saves default state (feel disabled, seed=1) → loads it back → asserts defaults are still defaults. Passes even if the `feel_v5` subtree serialization is completely absent from getStateInformation. No mutation, no proof of round-trip.

### M3: AC-4 backward-compat has no test
AC-4 is in acceptance criteria. Verification checklist says "Backward-compat: loading a v4 blob doesn't crash." No test constructs a v4-style state blob (without pattern_v5/feel_v5/plock_v5/pads_v5 subtrees) and calls setStateInformation. If any of the `getChildWithName` guards were wrong, or if a restore path threw on an invalid subtree, it would never be caught before shipping.

### SR1: PLock values not bounds-clamped
All other scalar fields restored from ValueTree have `juce::jlimit` guards. `plock_pattern_.steps[s].values[p]` is the only field restored with a raw `static_cast<float>` and no range guard. A corrupted preset with NaN or Inf would propagate directly into the FX pipeline (FxChain applies p-lock values to SmoothedValue targets → filter/reverb/delay parameters). Not caught in tests because P11D* tests use valid values.

### SR2: P11D3 `presets[0]` positional assumption (resolved by M1 fix)
`pm2.load(presets[0].file)` assumes test_groove sorts first alphabetically. Any pre-existing preset in user_preset_dir (e.g., "auto_save.bqpreset") breaks this. Resolved by the M1 fix (save_to_file to TemporaryFile, load by path directly).

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | P11D3 pollutes user preset dir + vacuous CI skip | Task 3 — P11D3 test body | Added `save_to_file(name, category, dest)` to PresetManager header/impl; refactored `save()` to delegate to it. P11D3 redesigned to use `juce::TemporaryFile` via `save_to_file()` — no user-dir contact, no graceful-skip path. |
| 2 | P11D2 vacuous — default-in/default-out | Task 3 — P11D2 test body | Added `set_feel_pattern(const FeelPattern&)` to BaqueProcessor header/impl (analogous to `set_pattern()` from 05-01). P11D2 redesigned: sets enabled=true, steps[0].timing_ms=25.5f, steps[3].vel_scale=0.7f, seed=42; round-trips; asserts all non-default values restored. |
| 3 | AC-4 backward-compat untested | Task 3 — new P11D5 added | Added P11D5: gets v5 blob, parses ValueTree via `readFromData`, strips 4 v5 subtrees, re-serializes, calls setStateInformation on fresh processor, asserts REQUIRE_NOTHROW + feel.enabled==false + source_file()==File{}. Test count 248 → 249. |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | PLock values not bounds-clamped on restore | Task 1 — setStateInformation plock_v5 block | Added `juce::jlimit(-1.0e6f, 1.0e6f, ...)` wrapper on plock values restore. Wide range preserves all legitimate physical values (filter cutoff, reverb amount, etc.) while blocking NaN/Inf from corrupted presets. |
| 2 | P11D3 `presets[0]` positional assumption | Task 3 — P11D3 | Resolved as a side-effect of M1 fix: P11D3 no longer uses `list_user_presets()` at all; loads by direct path from `juce::TemporaryFile`. |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 1 | PLock round-trip test (no dedicated P11D_plock) | PLock paths covered indirectly: AC-1 includes PLock in Given; setStateInformation PLock restore path is exercised by P11D5 (no crash). Dedicated PLock value-assertion test deferred to Phase 11-02 or Hardening. |
| 2 | Async preset I/O for large files | Preset files are <50KB ValueTree binary. Synchronous I/O on message thread is acceptable for v1. Async path deferred to Phase 12 (Hardening) if profiling shows latency. |
| 3 | Metadata XML parsing in list_user_presets() | Filename = preset name in v1. Full category/tag parsing deferred to Phase 11-02 per plan scope (browser UI needs DB/library). |

## 5. Audit & Compliance Readiness

**Evidence produced:** Round-trip tests (P11D1–P11D5) with specific non-default values provide defensible CI evidence that serialization is faithful. No vacuous-pass risks remain.

**Silent failure prevention:** P11D2 now fails if feel_v5 subtree is missing from getStateInformation. P11D3 now fails if save_to_file produces an unreadable file. P11D5 fails if setStateInformation throws on a v4 blob.

**Post-incident reconstruction:** Binary ValueTree format is deterministic. The "BQP1" magic header with length-prefixed XML metadata provides enough context to identify preset format version and origin without opening a hex editor.

**Ownership:** PresetManager is a distinct class with clear responsibility. save/load are message-thread-only (documented). No audio-thread callers.

**Weak area:** The `set_feel_pattern()` method bypasses UiCommandQueue — it's a direct write to `feel_pattern_` from message thread. This is safe in tests (no audio thread running) and in setStateInformation (audio suspended). But if called outside those contexts, it's a data race. The comment in the plan marks this as "not RT-safe for live edits." Phase 11-02 UiCommand for feel_pattern_ should eventually replace this — acceptable for Phase 11-01 scope.

## 6. Final Release Bar

**What must be true before this plan ships:**
- 249/249 tests pass including all 5 P11D tests
- P11D2 asserts non-default feel values (enabled=true, timing_ms=25.5f) are restored
- P11D3 uses TemporaryFile via save_to_file — no user dir involvement, deterministic pass/fail
- P11D5 verifies v4 blob loads without throw and defaults are correct

**What risks remain if shipped after upgrades:**
- PLock round-trip not explicitly value-asserted (P11D5 only checks no-crash + feel defaults). If plock_v5 restore had a bug writing wrong param indices, tests would still pass. Acceptable risk at Phase 11-01 — PLock serialization code is straightforward and directly derives from PLockPattern layout.
- `set_feel_pattern()` is a raw message-thread write — safe for current callers (tests + state load), risky if misused later. Phase 11-02 should add the UiCommand path.

**Sign-off:** Yes, with these upgrades applied, I would approve this plan for APPLY execution.

---

**Summary:** Applied 3 must-have + 2 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
