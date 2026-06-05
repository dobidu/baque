---
phase: 04-sample-engine
plan: 02
type: Audit
about: "BAQUE"
---

# Enterprise Plan Audit Report

**Plan:** `.paul/phases/04-sample-engine/04-02-PLAN.md`
**Audited:** 2026-06-05
**Verdict:** Conditionally acceptable → upgraded

---

## 1. Executive Verdict

**Conditionally acceptable — upgraded after applying 7 findings.**

The plan is architecturally sound and shows strong RT-safety discipline carried forward from prior phases. The three-task decomposition is clean and sequencing is correct. However, several non-obvious correctness risks would have shipped silently: a voice-leak in a specific ADSR parameter combination, wrong-voice note_off due to voice stealing, and stale scheduler state after plugin restart. All have been remediated in the upgraded plan. I would sign off on execution against the upgraded plan.

---

## 2. What Is Solid

- **RT-safety throughout**: fixed-size arrays for layers (k_max_layers=8), no heap allocation in hot path, two-pass bounded iteration for layer selection (max 8×2). Correct.
- **Backward compatibility design**: `num_layers_=0` falls back to `buffer_` unchanged; all new params to `trigger()`, `trigger_at()` are defaulted; existing call sites compile without modification. Well-designed.
- **Choke-before-trigger ordering**: choke group dispatch fires before new voice is allocated — correct sequencing that prevents the new voice from being immediately choked.
- **Velocity gain formula preserved**: layer selection changes the buffer, not the gain formula. The 04-01 pinned convention is respected.
- **`pad_bank.h` boundary protection**: `buffer_`, `data()`, `num_samples()`, `sample_buffer()` explicitly protected for 04-03. Good forward dependency tracking.
- **`uint8_t rr_index` overflow safety**: natural overflow with `% match_count` is mathematically correct and documented. Acceptable.
- **ADSR ms→frames computed at trigger time**: single multiply per trigger, not per frame. Correct RT design.
- **End-of-sample fade-out preserved**: the existing `remaining_frames < k_fade_frames` safety net is kept alongside ADSR and handles click prevention independent of envelope state. Correct layering.

---

## 3. Enterprise Gaps Identified

### Gap 1 (Must-Have): Stale `runtime_` after `prepareToPlay` / plugin restart
`Scheduler::prepare()` stores `sample_rate_` but the plan did not specify clearing `runtime_`. After `pool_.reset_all()` (which reinitialises voice objects), `runtime_[i].last_voice` still holds a pointer into the now-reset pool — the address is valid (std::array, no realloc) but logically stale. More critically, `rr_index` retains its value across stop/restart: a pad that fired 5 times in one session starts the next session at rr_index=5, not 0. For a user who restarts the plugin mid-session, round-robin starts mid-cycle silently.

### Gap 2 (Must-Have): `dec_frames_` private field not declared in `.h` plan section
The plan action correctly identifies that `dec_frames_` must be a private field (required for attack→decay transition inside `process()`), but the `.h` declaration section only listed `release_ms_` and `sample_rate_`. This would produce a compile error (`use of undeclared identifier`). Implementation-blocking.

### Gap 3 (Must-Have): ADSR voice leak when `sustain=0` + `note_off` at envelope floor
When `play_mode=gate`, `sustain=0`, and decay has completed: `env_level_=0.0`. In `note_off()`, the release compute path calculates `env_rate_ = -env_level_ / rel_f = 0.0f / 32 = 0.0`. The voice enters `EnvState::release` but `env_level_` never changes — `active_` is never set to false. Voice leaks: occupies a pool slot indefinitely, invisible to the steal heuristic (frames_rendered keeps growing). With 64 voices and sustained notes at sustain=0, pool exhaustion is reachable in a live performance.

### Gap 4 (Strongly Recommended): Wrong-voice `note_off` after voice stealing
`runtime_[pad_index].last_voice` stores the last allocated voice. If the pool fills up and `allocate()` steals that voice for a different pad, the pointer now points to a voice playing different content with a different `pad_index_`. When note-off arrives for the original pad, the check `v->is_active()` passes (voice is active) but fires note_off on the wrong voice — silencing the newly stolen voice mid-playback. The fix is a one-field comparison: `v->pad_index() == pad_index`.

### Gap 5 (Strongly Recommended): `goto done` — enterprise code smell + structural risk
The plan used `goto` to break out of the per-frame loop from inside a switch-case nested inside the loop body. While technically safe here (no RAII objects in scope), `goto` in C++ is: (a) flagged by clang-tidy (`cppcoreguidelines-avoid-goto`), (b) a CI-fail risk if clang-tidy is added in hardening phase, (c) structurally fragile — any future developer inserting a RAII object or a second loop level breaks the invariant silently. The restructured approach (set `active_=false` in switch, mix outside, `if (!active_) break`) is zero-overhead and eliminates the risk.

### Gap 6 (Strongly Recommended): `frames_rendered_ = 0` on loop wrap corrupts steal metric
`get_position()` returns `frames_rendered_` as the voice age for steal decisions. Resetting it to 0 on each loop wrap makes a looping voice appear as young as a freshly triggered voice. In a full pool, `allocate()` preferentially steals the highest `frames_rendered_` — resetting on loop means one-shots accumulate age and get stolen over looping voices, which is inverted priority (a held hi-hat drone should be stolen before a kick that fired 2ms ago). Secondary bug: the existing `frames_rendered_ < k_fade_frames` fade-in check re-triggers on every loop iteration, producing an audible amplitude dip at each loop point.

### Gap 7 (Strongly Recommended): Missing self-retrigger choke test
The plan tests choke between two *different* pads but omits the primary hi-hat usage: pad A retriggered while its own previous voice is still active. The code path is identical (choke_group() iterates all voices, including those with `pad_index_` equal to the triggering pad), but this scenario has different timing characteristics and is the most common real-world test case. Absence of a self-retrigger test leaves the primary hi-hat use case untested.

---

## 4. Upgrades Applied to Plan

### Must-Have (Release-Blocking)

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 1 | Stale `runtime_` after prepareToPlay | T1 action (note), T3 scheduler prepare() | Added `runtime_.fill({})` to `prepare()` body; added note to T1 flagging the dependency |
| 2 | `dec_frames_` missing from `.h` private fields | T3 `sample_voice.h` additions | Added `int dec_frames_ = 0;` to the private fields list with explanation comment |
| 3 | Voice leak: `sustain=0` + `note_off` → zero `env_rate_` | T3 `note_off()` code | Added immediate `active_ = false; return;` guard when `env_level_ <= 0.0f` before computing `rel_f` |

### Strongly Recommended

| # | Finding | Plan Section Modified | Change Applied |
|---|---------|----------------------|----------------|
| 4 | Wrong-voice `note_off` after steal | T1 note-off path | Added `v->pad_index() == pad_index` guard to note-off condition with comment explaining steal scenario |
| 5 | `goto done` pattern | T3 ADSR `process()` RELEASE case | Removed inline output writes + `goto`; set `active_=false` in switch; added structured `if (!active_) break;` after mix block |
| 6 | `frames_rendered_ = 0` on loop wrap | T3 loop behaviour code | Removed reset line; added documentation comment explaining steal metric invariant and why fade-out is independent |
| 7 | Missing self-retrigger choke test | T1 test list | Added test 4: pad A (choke_group=1) triggers twice — first voice enters fade-out, new voice at full amplitude |

### Deferred (Can Safely Defer)

| # | Finding | Rationale for Deferral |
|---|---------|------------------------|
| D1 | `uint8_t rr_index` overflow behaviour not explicitly tested | Mathematical correctness is clear (`n % m` with natural uint8_t overflow); add a comment in code, not a separate test |
| D2 | `play_mode=loop + reverse=true` interaction not tested | Loop wraps to `num_samples-1` for reverse, which is the correct initial position. Logic is trivially correct from the existing reverse condition; explicit test deferred to Phase 6 when loop points become configurable |
| D3 | `AdsrParams` field names in ms vs. JUCE's `juce::ADSR::Parameters` (seconds) | No integration with JUCE ADSR class planned; if integrated in Phase 6 FX a conversion layer handles it; naming is clear for the domain |

---

## 5. Audit & Compliance Readiness

**Evidence produced by this plan:**
- Catch2 tests cover: choke dispatch (amplitude-based), layer selection, round-robin cycling, ADSR ramp shape, play mode note-off behaviour, gate release timing, loop wrap, self-retrigger choke. Tests are deterministic (fixed sample data, fixed block sizes). Post-incident reconstruction of "why did the hi-hat not choke" is answerable by test trace.

**Silent failure prevention:**
- Gap 3 (voice leak) would have been silent in all tests: no test used `sustain=0` with gate+note_off. Now guarded and tested.
- Gap 4 (wrong-voice note_off) would have been silent in unit tests (pool rarely full) but reproducible in live performance (16-pad full-velocity patterns). Now guarded.

**Potential failure modes not covered by tests (deferred to Phase 12 hardening):**
- MIDI note-on flood without note-off (all 64 voices active, steal loop under contention) — performance regression, not correctness issue.
- Host sends note-off without prior note-on — `runtime_[pad_index].last_voice` is nullptr; existing `v != nullptr` guard handles this.

**Ownership:** Audio engine is single-author (BAQUE project); RT-safety invariants documented in `pad_bank.h` header contract. No multi-owner coordination gaps for this phase.

---

## 6. Final Release Bar

**Must be true before execution:**
1. `Scheduler::prepare()` calls `runtime_.fill({})` — verified via test (rr_index starts at 0 after prepare)
2. `dec_frames_` declared in `sample_voice.h` — verified at compile time
3. `note_off()` guard for `env_level_ <= 0.0f` — verified by test 6 (gate + sustain=0 immediate deactivation)
4. `pad_index()` check in note-off routing — verified by pool-full steal scenario test
5. No `goto` in production code — clang-tidy clean at Phase 12 hardening
6. All 31 existing tests pass + 6+ new tests

**Risks remaining if shipped after upgrade:**
- Minimal. All identified correctness risks are addressed. Remaining deferreds are documentation/test-coverage items, not correctness gaps.

**Sign-off:** I would approve execution of the upgraded plan.

---

## Summary

Applied **3 must-have** + **4 strongly-recommended** upgrades. Deferred **3** items.
Plan status: **Updated and ready for APPLY.**

---
*Audit performed by PAUL Enterprise Audit Workflow*
*Audit template version: 1.0*
*Phase: 04-sample-engine, Plan: 02*
*Audited: 2026-06-05*
