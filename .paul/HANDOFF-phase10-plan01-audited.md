# PAUL Handoff

**Date:** 2026-06-10 (session 26)
**Status:** paused — 10-01 planned + audited, APPLY not started; PR #1 CI still running (macOS pending)

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

## What Was Done (sessions 25–26)

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
- **PR #1 unblocked:**
  - Root cause: origin/main was 23 commits stale (stuck at Phase-4 commit `10c39bb`). Pushed main (`10c39bb..c55bcc1`).
  - Rebuilt PR branch: deleted old `ci/node20-actions-v5`, recreated from current main + cherry-pick of CI bump → commit `8603864`, force-pushed. PR diff = `ci.yml` only (3 lines, checkout/cache v4→v5).
- Session 26: CI monitoring only — no src/ changes.

---

## What's In Progress

- **PR #1 CI running** (run 27253122002):
  - Lint: PASS ✓
  - Windows: PASS ✓ (7m2s — Phases 5–9 work on Windows)
  - Ubuntu: FAIL — exit code 143 (SIGTERM infra flake, confirmed not code)
  - macOS: **PENDING** (GR4 open risk — see below)
- Main branch CI (run 27253315540) also in progress (first macOS/Windows run since Phase 4).
- PAUL code work: nothing mid-flight. 10-01 ready for APPLY; src/ untouched.

---

## Open Risk

**GR4** (`tests/test_granular.cpp:114`, pitch_spread contrast test) failed on macOS at stale snapshot. Current code's macOS leg decides if it's a real platform bug. If macOS is red:
- Read `tests/test_granular.cpp:114` (GR4 test)
- Likely FP or platform-rand sensitivity
- Fix on main first, then merge PR

Do NOT merge PR #1 if macOS is red.

---

## What's Next

**Immediate (CI, time-critical, deadline June 16):**
1. `gh pr checks 1` — check if macOS completed.
2. If macOS green → `gh run rerun 27253122002 --failed` (rerun ubuntu flake) → merge when all green.
3. If macOS GR4 red → fix bug first, then rerun.
4. Merge: `gh pr merge 1 --squash --delete-branch`

**Then (PAUL loop):** `/paul:apply .paul/phases/10-ui-ux/10-01-PLAN.md`

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
2. Check PR #1: `gh pr checks 1`
3. If macOS done + green → rerun ubuntu → merge. If macOS red → fix GR4 first.
4. After PR merged → `/paul:apply .paul/phases/10-ui-ux/10-01-PLAN.md`

---

*Handoff created: 2026-06-10 (updated session 26)*
