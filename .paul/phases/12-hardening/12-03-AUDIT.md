---
phase: 12-hardening
plan: 03
audit_date: 2026-06-19
auditor: senior-principal-engineer
plan_approved: true
findings_applied: [M1, SR1, SR2]
findings_deferred: [D1]
type: Audit
about: "BAQUE"
---

# Plan 12-03 Audit Report

**Verdict: APPROVED with must-have and strongly-recommended findings applied.**

---

## Audit Summary

| Section | Rating | Notes |
|---------|--------|-------|
| Scope clarity | Pass | Three tasks: P12D3 polyphony, P12D4 zero-alloc, DoD gate |
| Acceptance criteria | Pass | GWT scenarios measurable and honest |
| Task decomposition | Pass | Tasks are independent; Task 3 is verification-only |
| Boundary conditions | Pass | Explicit no-go list; ODR fallback documented |
| Verification steps | Conditional | SR2 fixed: finite check was vacuous at block 499 |
| Test coverage | Conditional | M1 fixed: free() missing → UB crash; SR1 fixed: P12D4 setup only active 1 real voice |

---

## Findings

### M1 — MUST HAVE (APPLIED): `free()` not overridden → bootstrap pointer UB/crash

**Severity:** Must-have — bootstrap allocator returns pointers into `s_bootstrap_buf` (BSS segment). Code that receives these pointers will eventually call `free()` on them. Without a `free()` override, the system allocator receives a pointer not allocated by `malloc` → undefined behavior → crash. This would cause the test binary to crash before any test runs.

**Root cause:** Plan comment said "free does NOT need an override (deallocation is not an RT allocation)" — this is correct for the RT-counting logic, but wrong for the bootstrap pointer lifecycle. The bootstrap allocator has no per-object free; system free cannot accept its pointers.

**Applied:** Added `free()` override to rt_alloc_counter.cpp. Override checks if the pointer is within `s_bootstrap_buf` range (simple bounds comparison); if so, returns immediately (no-op). Otherwise, calls `dlsym(RTLD_NEXT, "free")` via a cached static function pointer. Fixed plan comment to clarify why `free()` IS required.

---

### SR1 — STRONGLY RECOMMENDED (APPLIED): P12D4 arms only 1 real DSP voice

**Severity:** Strongly recommended — P12D4 setup fired note-ons for notes 36-43 (pads 0-7). Only pad 0 has a loaded sample in the default test fixture; trigger_at returns nullptr for pads 1-7. With only 1 voice actually mixing, the measured 100 processBlock calls barely exercise FxChain, scatter, tape_stop, gater — the most complex and allocation-prone paths. The test would pass even if those paths had RT allocations.

**Root cause:** Plan description said "arm 8 voices (notes 36-43 = pads 0-7)" without accounting for the test fixture's single loaded sample.

**Applied:** Changed P12D4 setup to fire 4 × note-on for note 36 at sample offsets 0-3. This creates 4 real voices of pad 0 (the only loaded sample), exercising the full FxChain/scatter/tape_stop/gater accumulation path across all 100 measured blocks. Added explanatory comment documenting why notes 37-43 would yield only 1 active voice.

---

### SR2 — STRONGLY RECOMMENDED (APPLIED): P12D3 finite check at block 499 is vacuous

**Severity:** Strongly recommended — the finite check was placed AFTER the 500-block stability loop, checking block 499's output. test_kick.wav at 44100 Hz / 512 samples/block decays to silence long before block 499 (~11.6 seconds of audio time). `std::isfinite(0.0f)` is trivially true — the check adds no diagnostic value for the 64-voice accumulation scenario it intends to verify.

**Root cause:** Standard stability loop pattern copied from P12D1 (which had the same structure). P12D1's finite check was also at the end — acceptable there since the focus was the stability loop itself. For P12D3, the key risk is 64-voice accumulation overflow (64× amplitude → NaN if FxChain clips to inf), which occurs in block 0, not block 499.

**Applied:** Moved finite check to immediately AFTER block 0 (before the stability loop). At block 0, all 64 voices are actively contributing — the check is non-vacuous. Added comment explaining the reasoning. The 500-block stability loop follows (crash detection only, no finite check needed at block 499).

---

### D1 — DEFERRED: realloc() bootstrap recursion risk

**Classification:** Can safely defer — `dlsym(RTLD_NEXT, "realloc")` is called lazily on first `realloc` invocation (C++ static local). If `realloc` is called during the dlsym bootstrap phase, `dlsym` might internally call `realloc`, causing recursion. However, glibc's `dlsym` uses `calloc` (not `realloc`) for its internal allocations. The scenario is unlikely. The bootstrap buffer handles `calloc` via our `calloc` override which delegates to our `malloc` (which uses the buffer). Acceptable risk for a test-only helper.

**Defer to:** If test crashes unexpectedly before any TEST_CASE runs, investigate realloc bootstrap as first suspect. Resolution: add a static realloc bootstrap path similar to malloc's.

---

## Impact on Test Count

| State | Count |
|-------|-------|
| After 12-02 APPLY | 254 |
| After 12-03 APPLY (P12D3 + P12D4) | **256** |

---

## Files Modified by Audit

| File | Change |
|------|--------|
| `.paul/phases/12-hardening/12-03-PLAN.md` | M1 (free() override); SR1 (4 × note 36 setup); SR2 (finite check after block 0) |

---

*Audit performed: 2026-06-19*
*Built with PAUL Framework v1.4*
