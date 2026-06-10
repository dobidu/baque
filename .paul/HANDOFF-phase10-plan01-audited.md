# PAUL Handoff

**Date:** 2026-06-10 (session 25, updated at second pause)
**Status:** paused — 10-01 planned + audited, APPLY not started; PR #1 merge in flight (CI running)

---

## READ THIS FIRST

You have no prior context. This document tells you everything.

**Project:** BAQUE — hybrid groovebox audio plugin (VST3/AU/Standalone, JUCE 8 / C++17, GPLv3).
**Core value:** Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, controlled error as first-class features.

---

## Current State

**Version:** 0.0.0
**Phase:** 10 of 13 — UI/UX — Planning (Phase 9 complete, 209/209 tests)
**Plan:** 10-01 — created + enterprise-audited, awaiting APPLY

**Loop Position:**
```
PLAN ──▶ AUDIT ──▶ APPLY ──▶ UNIFY
  ✓        ✓        ○        ○
```

---

## What Was Done (this session)

- Resumed at clean Phase 9→10 boundary; consumed + archived HANDOFF-2026-06-10.md.
- **Phase 10 decomposition confirmed (complex track, 7 plans):**
  - 10-01: UI→engine command queue + atomicization + UiStateSnapshot ← current
  - 10-02: LookAndFeel + design system + header/transport + NAV shell
  - 10-03: PERFORM screen (pads, sequencer grid, sample editor)
  - 10-04: Feel Field radial visualizer (parallel candidate after 10-02)
  - 10-05: FX + MIX screens
  - 10-06: PERF FX + MIDI screens
  - 10-07: BROWSER + undo/redo + Phase 10 DoD (drag-to-beat)
- **10-01 PLAN written** (`.paul/phases/10-ui-ux/10-01-PLAN.md`), 3 tasks:
  1. `UiCommand` POD + `UiCommandQueue` (SPSC, juce::AbstractFifo, 256 slots, zero-alloc) + QC1-QC4 tests
  2. Drain at top of processBlock → routes to perf_state_/lane_routing_/clock_master_/cc_out_/pad params/live step+p-lock edits/apply_hardware_template (template now executes ON audio thread)
  3. `UiStateSnapshot` (per-field atomics: step, playing, peaks, lane pulse, bpm) + UI1-UI8 integration tests via real processBlock
- **Enterprise audit done** (`10-01-AUDIT.md`), verdict conditionally acceptable → upgraded:
  - M1 (release-blocking): getState/setState race once structs become audio-owned → both methods bracket `suspendProcessing` + message-thread pre-drain of queue; AC-5 + UI8 test added
  - SR1: lane pulse derived by scanning `midi_buffer_seq_` note-ons (lane = note − 36), NOT hooking Sequencer internals; auto-respects 08-04 gating; EXT-only lanes don't pulse (documented v1 limit)
  - SR2: `push()` debug-jassert message thread; `apply_template` invalid id jassert+IGNORE (never clamp — clamp = wrong hardware silently)
  - Deferred 3: queue-full UI policy (10-02), EXT-only pulse (10-03), APVTS mute/solo automation (10-05)
- STATE/ROADMAP/paul.toml/ledger synced; WIP commit `c55bcc1` pushed.
- **PR #1 unblocked (second half of session):**
  - Root cause found: **origin/main was 23 commits stale** (stuck at Phase-4 commit `10c39bb`, 2026-06-05) — Phases 4–9 never pushed, never CI-tested on macOS/Windows. **Pushed main** (`10c39bb..c55bcc1`).
  - Old PR branch carried stale Phase-9-wip code → both red checks were artifacts (clang-format violations at 09-02 snapshot; GR4 granular test from 182-test era). Verified lint clean locally on current code.
  - **Rebuilt PR branch:** deleted old `ci/node20-actions-v5`, recreated from current main + cherry-pick of the CI bump → commit `8603864`, force-pushed. PR diff now = `ci.yml` only (3 lines, checkout/cache v4→v5).

---

## What's In Progress

- **PR #1 CI running** (run 27253122002): lint PASS, ubuntu FAILED with runner-shutdown SIGTERM (exit 143 — infra flake, NOT code), macOS + Windows pending (~1h legs). Rerun of ubuntu blocked until run completes ("cannot be rerun" while jobs pending).
- Background watcher on `gh pr checks 1 --watch` active in the paused session; in a fresh session just check `gh pr checks 1`.
- Main CI also running on `c55bcc1` (first main run since Phase 4 — first macOS/Windows exposure for Phases 5–9 code).
- **Open risk:** GR4 (`test_granular.cpp:114`, pitch_spread contrast test) failed on macOS at the old snapshot. May be a REAL macOS-specific issue — current code's macOS leg decides. If it fails: platform bug, fix before merge; do NOT merge red.
- PAUL code work: nothing mid-flight; 10-01 ready for APPLY; src/ untouched.

---

## What's Next

**Immediate (CI, time-critical):**
1. `gh pr checks 1` — when run 27253122002 completes: ubuntu red from flake → `gh run rerun 27253122002 --failed`.
2. All green → `gh pr merge 1 --squash --delete-branch` (or rebase-merge; single commit either way). Deadline **June 16** (Node.js 20 deprecation).
3. If macOS GR4 fails on current code → real bug: read `tests/test_granular.cpp:114` (GR4), likely FP/platform-rand sensitivity; fix on main first, then merge PR.

**Then (PAUL loop):** `/paul:apply .paul/phases/10-ui-ux/10-01-PLAN.md` → `/paul:unify 10-01` → `/paul:plan` 10-02 (LookAndFeel + design system).

---

## Key Files

| File | Purpose |
|------|---------|
| `.paul/STATE.md` | Live project state |
| `.paul/ROADMAP.md` | Phase overview (Phase 10 = 7 plans) |
| `.paul/phases/10-ui-ux/10-01-PLAN.md` | Current plan (audited, ready for APPLY) |
| `.paul/phases/10-ui-ux/10-01-AUDIT.md` | Audit report (M1/SR1/SR2 rationale) |
| `src/plugin_processor.h` | Structs the queue will own; drain target |
| `src/audio/perf_state.h`, `lane_routing.h`, `midi_cc.h`, `pad_bank.h` | Single-writer contracts being retired |
| `BAQUE-UI-SPEC.md` / `BAQUE_MOCKUP.html` | Phase 10 visual source of truth (plans 10-02+) |

---

## Resume Instructions

1. Read `.paul/STATE.md` for latest position.
2. Loop at PLAN+AUDIT done; APPLY pending — no checkpoint mid-task.
3. Run `/paul:resume`, then `/paul:apply .paul/phases/10-ui-ux/10-01-PLAN.md`.

---

*Handoff created: 2026-06-10*
