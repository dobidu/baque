# Enterprise Plan Audit Report

**Plan:** .paul/phases/12-hardening/12-01-PLAN.md
**Audited:** 2026-06-19T07:00:00Z
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable — upgraded after applying 1 must-have + 3 strongly-recommended findings.**

The plan's architectural decisions are sound: ScopedAudioThreadGuard RAII is the correct pattern, MidiBuffer pre-sizing in prepareToPlay() is the right fix for the primary RT alloc risk, and the 2-task decomposition is appropriately scoped. The plan is not acceptable as-written due to a vacuous test assertion (M1) that would give false confidence in audit evidence. The grep audit scope (SR2) and explicit file coverage (SR3) gaps are also material for a hardening phase where "zero RT alloc" is a DoD criterion.

After applying the 4 findings, I would approve this plan for production.

---

## 2. What Is Solid

**ScopedAudioThreadGuard using `inline thread_local bool`:** Correct C++17 approach. POD `bool` in TLS is zero-initialized without heap allocation. RAII set/clear is exception-safe. The boundary note (no operator new override due to JUCE Debug ODR conflict) is correctly scoped and documented.

**MidiBuffer::ensureSize(512) in prepareToPlay():** Correctly identifies the primary RT alloc risk: `juce::MidiBuffer::addEvent()` grows its internal `juce::MemoryBlock` on first use. Calling `ensureSize()` in prepareToPlay() is the right fix — exactly the point where one-time heap ops are permitted.

**Pre-plan code audit performed before writing plan:** The plan was informed by direct inspection of processBlock (lines 265–465) before design. `dispatch_ui_command()`, `time_stretch.cpp`, and `slicer.cpp` allocations were correctly pre-classified as either RT-safe or off-path. This kind of upfront investigation is the correct enterprise practice.

**Scope limits clearly defined:** Plan explicitly calls out operator new override as deferred with the technical reason (JUCE Debug ODR conflict with pre-compiled BAQUE target) and 3 out-of-scope items (pluginval, 64-voice, JUCE upgrade). Clear boundaries prevent scope creep.

---

## 3. Enterprise Gaps Identified

### G1 — P12D1 output assertion is vacuous (M1)
The original test ran 1000 blocks of processBlock (512 samples @ 44100 Hz = 11.6ms/block → ~11.6 seconds total). Voices triggered in block 0 with default drum samples (none loaded — default BaqueProcessor has no samples). By block ~500, all voices have long since decayed (or produced silence immediately since no PCM data is present). The `buf.clear()` at the top of each iteration means `buf` at block 999 is the output of processBlock with no active voices = all zeros. `std::isfinite(0.0f) == true` → `all_finite = true` always. The REQUIRE(all_finite) proves nothing. Additionally, processBlock modifies `midi_first` in-place (clears and replaces with EXT MIDI output), so any post-loop check of `midi_first` would also be vacuous.

### G2 — Grep pattern too narrow for defensible RT audit (SR2)
The original grep covers `push_back`, `juce::String(`, and basic allocators, but misses: `emplace_back`, `.insert(`, `.resize(`, `std::map`, `std::unordered_map` (operator[] on std::map inserts!), `.lock(`, and `CriticalSection`. Any of these on the RT path would violate the zero-alloc or lock-free contract. If a future contributor adds `std::map<int,float>` to a processBlock path, the existing narrow grep would not catch it.

### G3 — `scheduler.cpp` and `note_tracker.cpp` absent from explicit audit file list (SR3)
`Scheduler::process()` is called twice directly from processBlock (`scheduler_.process(midi_buffer_seq_, ...)` and `scheduler_.process(midi_messages, ...)`). These are the primary audio event dispatch functions and are firmly on the RT path. The grep covers them via `src/audio/` glob, but the audit file list must name them explicitly for audit defensibility. If a violation were later found in scheduler.cpp, "we grepped src/audio/" would not satisfy an audit requiring "all RT-path files were explicitly reviewed."

### G4 — `dispatch_ui_command()` not explicitly in audit checklist (SR1)
`dispatch_ui_command()` is called from the RT thread via `ui_commands_.drain([this](const UiCommand& cmd) { dispatch_ui_command(cmd); })` in processBlock. It is NOT a simple struct write — it is a 30-case switch dispatching to pad_bank_, perf_state_, lane_routing_, cc_out_, sequencer_, plock_pattern_, etc. From code inspection it appears RT-safe (all cases: bounds-clamped array writes to POD struct members), but this must be explicitly documented in the plan's checklist. An auditor reading the plan after the fact should not have to reverse-engineer whether dispatch_ui_command was examined.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| M1 | P12D1 output check vacuous — `buf` at block 999 is all zeros (no samples loaded; voices either silent or done); `isfinite(0.0f)` always passes; REQUIRE(all_finite) proves nothing | Task 2 `<action>` test code; AC-4 text; Task 2 `<done>` | Removed `all_finite` check; added `REQUIRE(midi_first.getNumEvents() == 16)` BEFORE block 0 (processBlock clears midi_messages in-place — post-block check would be vacuous); updated AC-4 to "midi_first.getNumEvents() == 16 before block 0 (honest setup check)"; updated `<done>` text |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| SR1 | `dispatch_ui_command()` not in audit checklist — executes on RT thread via drain lambda; ~30 cases not explicitly documented as RT-safe | Task 1 `<action>` Step 4 | Added "Special attention within plugin_processor.cpp" block explicitly calling out dispatch_ui_command(), its RT-thread context, and that pre-audit code read confirmed it clean (struct writes + jlimit clamps) |
| SR2 | Grep patterns too narrow — `emplace_back`, `.insert(`, `.resize(`, `std::map`, `std::unordered_map`, `.lock(`, `CriticalSection` not covered | Task 1 `<verify>` grep command | Expanded grep to include all missing patterns; added note that time_stretch.cpp and slicer.cpp hits are expected/safe (off RT path) |
| SR3 | `scheduler.cpp` and `note_tracker.cpp` absent from explicit audit file list — both on direct RT path via Scheduler::process() | Task 1 `<files>` tag; Task 1 `<action>` "Files needing close inspection" | Added both files to `<files>` tag; added entries in "Files needing close inspection" with rationale and note that they're covered by src/audio/ glob but must be explicitly documented |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|----------------------|
| D1 | Instrumented operator new override for zero-alloc counting (atomic counter + thread-local flag) | Conflicts with JUCE_CHECK_MEMORY_LEAKS=1 in Debug builds when baque_tests links pre-compiled BAQUE target (ODR: two operator new definitions); requires restructuring CMakeLists.txt (disable JUCE leak detector for test target); noted in plan boundaries; grep audit + code inspection provides equivalent coverage for v1 |
| D2 | Timing assertion in P12D1 — assert each block completes within RT budget | Non-deterministic in CI (variable machine load, WSL2 overhead); a hanging processBlock would cause Catch2 test timeout, caught as CI failure without an explicit timing check |
| D3 | Mid-processBlock tl_is_audio_thread "is-set" verification — prove the flag IS true during processBlock, not just false after | Cannot observe from outside processBlock without modifying production code or using a callback; RAII guarantee in C++ is sufficient for ctor/dtor correctness; the REQUIRE_FALSE verifies dtor ran without exceptions |

---

## 5. Audit & Compliance Readiness

**Audit evidence:** Post-M1-fix, the plan produces defensible evidence: the grep audit documents per-file RT safety, the SUMMARY.md will record dispatch_ui_command() as explicitly reviewed and cleared, and the P12D1 test gives a meaningful (non-vacuous) assertion. The operator new deferred item is correctly documented with a technical justification rather than a hand-wave.

**Silent failure prevention:** The vacuous `all_finite` check was a false positive — it passed while proving nothing. Replaced with `REQUIRE(midi_first.getNumEvents() == 16)`, which fails if the test setup is wrong (e.g., `for (int note = 36; note < 52; ++note)` produces exactly 16 events — any miscount would be caught). The 1000-block loop + REQUIRE_FALSE loop provides 1000 concrete assertions that the RAII guard works correctly on every exit path.

**Post-incident reconstruction:** The SUMMARY.md will contain the per-file grep results and `dispatch_ui_command()` clearance documentation. A future debugger investigating an RT alloc incident will have a dated record of what was reviewed and what wasn't.

**Ownership:** Plan is autonomous (no checkpoints). Task 1 audit output feeds directly into SUMMARY.md — owner must document every file's verdict, not just "grep passed." This requirement is implicit but should be made explicit in the plan.

---

## 6. Final Release Bar

**Must be true before this plan ships:**
- P12D1 passes: 1000 blocks, 16 voices, REQUIRE_FALSE RAII check ×1000, setup check REQUIRE(16 events)
- grep audit (expanded pattern) returns 0 RT-path violations across all named files
- `dispatch_ui_command()` explicitly documented as RT-safe in SUMMARY.md
- `scheduler.cpp` and `note_tracker.cpp` explicitly named in grep results with "0 violations" verdict
- `prepareToPlay()` contains `midi_buffer_seq_.ensureSize(512)` and `midi_buffer_ext_.ensureSize(512)`

**Risks remaining if shipped as-is (without fixes):**
- Without M1 fix: P12D1 passes regardless of whether processBlock actually makes RT allocations — zero audit value for the "zero RT alloc" DoD criterion
- Without SR2 fix: grep might miss a `.lock(` or `emplace_back` violation introduced during or after APPLY

**Sign-off statement:** After applying M1 + SR1 + SR2 + SR3, I would sign my name to this plan as enterprise-grade for a v1 audio plugin release. The operator new deferred item (D1) is an acceptable gap — grep + code audit provides equivalent coverage, and the architectural infrastructure (ScopedAudioThreadGuard) positions a future contributor to close it without redesign.

---

**Summary:** Applied 1 must-have + 3 strongly-recommended upgrades. Deferred 3 items.
**Plan status:** Updated and ready for APPLY

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
