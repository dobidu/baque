# PAUL Handoff

**Date:** 2026-06-10 (session 25)
**Status:** paused — 10-01 planned + audited, APPLY not started

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
- STATE/ROADMAP/paul.toml/ledger synced.

---

## What's In Progress

- Nothing mid-flight in code. 10-01 plan ready for execution; src/ untouched this session.
- Uncommitted: `.paul/` updates (plan, audit, state) + untracked `.claude/` (intentional).

---

## What's Next

**Immediate:** `/paul:apply .paul/phases/10-ui-ux/10-01-PLAN.md` — execute the command-queue plan.

**After that:** `/paul:unify 10-01`, then `/paul:plan` for 10-02 (LookAndFeel + design system).

**Time-critical, separate:** CI `actions/checkout`+`cache` v4→v5, **PR #1** (https://github.com/dobidu/baque/pull/1) — STILL OPEN, **merge before June 16** (Node.js 20 deprecation, ~6 days).

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
