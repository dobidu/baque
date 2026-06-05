# Enterprise Plan Audit Report

**Plan:** .paul/phases/02-core-audio/02-01-PLAN.md
**Audited:** 2026-06-04
**Verdict:** Conditionally acceptable → upgraded → ready for APPLY

---

## 1. Executive Verdict

Conditionally acceptable. The architecture is sound (pre-allocated fixed array, SmoothedValue gain, separation of prepareToPlay vs processBlock concerns), but three gaps were production-blocking: raw byte casting of WAV BinaryData (guaranteed garbage/crash), missing `start_offset_` field in SampleVoice (creates a forced patching conflict in 02-02), and the allocate() `nullptr` return path (silently drops notes with no feedback). All three applied. Approved post-upgrades.

## 2. What Is Solid

- Pre-allocated `std::array<SampleVoice, 64>` — correct pattern; array lives in processor, no audio-thread allocation.
- `SmoothedValue<float>` for gain — standard JUCE pattern, eliminates zipper noise correctly.
- BinaryData decode in `prepareToPlay` not `processBlock` — correct separation.
- 32-sample fade-in/out on SampleVoice — necessary to prevent clicks; specified correctly.
- APVTS wiring with parameter layout — correct JUCE idiom for automatable parameters.
- Test allocation guard using thread_local flag + file-scope operator new — correct approach; properly scoped to processBlock only.

## 3. Enterprise Gaps Identified

1. **WAV header parsing (must-have):** `BinaryData::test_kick_wav` is a RIFF/WAV container, not raw PCM. Casting it to `int16_t*` reads the "RIFF" magic bytes as audio samples — first samples are garbage, then reads past the array if size/sizeof(float) is used on byte count. This is a silent data bug and potential out-of-bounds read.

2. **start_offset_ missing from SampleVoice (must-have):** 02-02 adds `start_offset_` to SampleVoice in its Task 1, but 02-02's boundary explicitly says "DO NOT CHANGE voice pool base interface." This contradiction means 02-02 would violate its own boundary the moment it touches the struct. Fix: add the field in 02-01 (where the struct is defined), with trigger() defaulting it to 0.

3. **allocate() may return nullptr (must-have):** Plan says "returns nullptr if pool full" and separately "voice stealing: steal if full." These are contradictory. If stealing is always attempted (as required for a beat machine that must not silently drop notes), nullptr is never a valid return from allocate(). A silent drop means the user's beat has missing hits with no error.

4. **prepareToPlay called multiple times (must-have):** `sample_data_` (vector) reallocates on second call to prepareToPlay (e.g., host changes buffer size or sample rate). Voice pointers into sample_data_.data() become dangling. The decode-once guard with `sample_loaded_` flag prevents this. Also needed: reset all voices inactive on prepareToPlay to prevent use-after-move if the buffer DOES get reallocated.

5. **Allocation test Catch2 interaction (strongly recommended):** Using `g_count_allocs = true` before processBlock and `false` after is correct, but Catch2's `REQUIRE` macro may allocate (for error message formatting). The guard must wrap ONLY the processBlock call, not the assertions. The plan text now specifies this correctly.

6. **Steal test expected nullptr (strongly recommended):** Test case 3 said "returns nullptr or valid stolen voice" — too permissive. A beat machine that sometimes drops notes silently has a correctness bug. Test must assert non-null return.

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | WAV header parsing — raw cast is wrong | Task 2 action | Use JUCE WavAudioFormat decoder; store decoded samples in juce::AudioBuffer<float> |
| 2 | start_offset_ field missing from SampleVoice | Task 1 action | Added start_offset_ field + trigger() sets it to 0; process_all checks it (02-02 compat) |
| 3 | allocate() returns nullptr — silent note drop | Task 1 action | steal is MANDATORY; allocate() NEVER returns nullptr |
| 4 | prepareToPlay reallocation hazard | Task 2 action | sample_loaded_ guard + reset all voices inactive in prepareToPlay |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 5 | Allocation test scoping | Task 3 action | Exact mechanism specified: thread_local flag + scoped guard wrapping processBlock only |
| 6 | Test 3 accepts nullptr | Task 3 action | Steal test asserts non-null return (mandatory steal) |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| 7 | Velocity curve mapping (linear vs exponential) | Phase 4 (Sample Engine) handles per-pad velocity layers; linear is correct for Phase 2 |

## 5. Audit & Compliance Readiness

- Build evidence: cmake + ctest + pluginval create reconstructable proof.
- Silent failure prevention: allocate() never silently drops; allocation test confirms RT-safety.
- Post-incident: `sample_loaded_` guard provides deterministic state; voice pool state is fully inspectable.

## 6. Final Release Bar

Must be true: WAV decoded via JUCE (not raw cast), allocation test passes, steal always returns voice, no -Wall -Wextra warnings. Remaining risk after upgrades: none blocking. Signed off.

---

**Summary:** Applied 4 must-have + 2 strongly-recommended upgrades. Deferred 1 item.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
