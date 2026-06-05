---
description: "BAQUE — milestone and phase structure"
type: Roadmap
about: "BAQUE"
---

# Roadmap: BAQUE

## Overview

From empty repo to signed v1.0 installers: plugin skeleton → core audio → sequencer → sample engine → Feel Engine (the product core) → FX/p-locks → lo-fi/granular → performance FX → MIDI/hardware → UI → presets → hardening → release. A parallel R&D track (SoundTouch fork) starts at Phase 4 and feeds quality refinements into Phase 7. Phase numbering maps to ESCOPO §12: PAUL Phase N = ESCOPO Fase N−1.

## Current Milestone

**v1.0 Release** (v1.0.0)
Status: In progress
Phases: 3 of 13 complete

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with [INSERTED])

| Phase | Name | Plans | Status | Completed |
|-------|------|-------|--------|-----------|
| 1 | Setup (Fase 0) | 2 | ✅ Complete | 2026-06-04 |
| 2 | Core Audio (Fase 1) | 2 | ✅ Complete | 2026-06-05 |
| 3 | Sequencer Base (Fase 2) | 2 | ✅ Complete | 2026-06-05 |
| 4 | Sample Engine (Fase 3) | 4 | In progress (1/4) | - |
| 5 | Feel Engine ⭐ (Fase 4) | TBD | Not started | - |
| 6 | FX + P-locks (Fase 5) | TBD | Not started | - |
| 7 | Lo-fi + Granular (Fase 6) | TBD | Not started | - |
| 8 | Scatter / Perf FX (Fase 7) | TBD | Not started | - |
| 9 | MIDI / Hardware (Fase 8) | TBD | Not started | - |
| 10 | UI/UX (Fase 9) | TBD | Not started | - |
| 11 | Presets & Library (Fase 10) | TBD | Not started | - |
| 12 | Hardening (Fase 11) | TBD | Not started | - |
| 13 | Release (Fase 12) | TBD | Not started | - |

**Parallel track — R&D-TS (SoundTouch fork):** runs from Phase 4 onward; fork v1 (varispeed + offline stretch + RT-safety wrapper) is a Phase 4 prerequisite; advanced refinements (transient/formant preservation, character mode, spectral hybrid) mature during Phases 5–7 and integrate in Phase 7.

## Phase Details

### Phase 1: Setup (Fase 0)
**Goal:** CMake + JUCE, plugin skeleton, CI, empty formats loading
**Depends on:** Nothing (first phase)
**Research:** Unlikely (established JUCE/CMake patterns)
**DoD:** VST3/AU/Standalone open in host; basic pluginval green; CI running

**Plans:**
- [x] 01-01: CMake + JUCE 8.0.13 skeleton — VST3/AU/Standalone build, pluginval strictness 5, repo hygiene
- [x] 01-02: Catch2 v3.15.0 + 3-OS CI (macOS universal / Windows / Linux) + pluginval + lint

### Phase 2: Core Audio (Fase 1)
**Goal:** Sample voice, 1 pad, transport
**Depends on:** Phase 1
**Research:** Unlikely
**DoD:** Pad fires sample without clicks; sample-accurate; RT-safe ✅ 2026-06-05

**Plans:**
- [x] 02-01: APVTS + 64-voice pre-allocated VoicePool + WAV decode + MIDI trigger + RT-safety test
- [x] 02-02: Sample-accurate Scheduler (samplePosition dispatch) + host transport + Phase 2 DoD verification

### Phase 3: Sequencer Base (Fase 2)
**Goal:** Step grid, patterns, swing
**Depends on:** Phase 2
**Research:** Unlikely
**DoD:** Pattern loops; global swing; pattern switch without glitch ✅ 2026-06-05

**Plans:**
- [x] 03-01: StepPattern + StepClock + Sequencer::generate() + two-call dispatch
- [x] 03-02: Global swing + seamless pattern switch (release/acquire) + NoteTracker + Phase 3 DoD tests

### Phase 4: Sample Engine (Fase 3)
**Goal:** Slice/chop, pitch, choke, velocity layers, reverse
**Depends on:** Phase 3 ✓ + R&D-TS fork v1 (offline stretch only — varispeed in-house, see 2026-06-05 decision)
**Research:** Likely (SoundTouch fork integration, transient detection)
**Research topics:** WSOLA params, transient detection for auto-slice, RT-safety wrapper
**DoD:** Chop-to-pads; varispeed + offline stretch (fork v1); choke working

**Plans (slice-around-fork strategy):**
- [x] 04-01: Pad bank + per-pad playback — note→pad routing, varispeed, reverse, gain/pan (fork-free) ✅ 2026-06-05
- [ ] 04-02: Choke groups + velocity layers + round-robin + ADSR/play modes (fork-free)
- [ ] 04-03: Auto-slice (transient detection) + chop-to-pads (fork-free)
- [ ] 04-04: R&D-TS fork v1 integration — offline time-stretch (blocked on fork repo bootstrap)

### Phase 5: Feel Engine ⭐ (Fase 4)
**Goal:** Micro-timing, humanize, dual-grid, no-grid, feel presets — *product core*
**Depends on:** Phase 4
**Research:** Likely (groove template design, sub-sample scheduling)
**Research topics:** MPC swing curves, Dilla/Burial timing analysis, seed determinism
**DoD:** "Dilla Drunk" and "Burial Broken" perceptible and seed-reproducible

### Phase 6: FX + P-locks (Fase 5)
**Goal:** Filters, reverb, delay, sidechain, EQ + p-locks
**Depends on:** Phase 5 (p-locks need sequencer + params)
**Research:** Unlikely (juce::dsp + established DSP)
**DoD:** P-lock automates any param per step; sidechain pump functional

### Phase 7: Lo-fi + Granular (Fase 6)
**Goal:** Bitcrush, SR reduction, vinyl, tape; granular engine. Integrates R&D-TS refinements.
**Depends on:** Phase 6 + R&D-TS advanced work
**Research:** Likely (SP-1200/303 character modeling, grain pool optimization)
**DoD:** SP-1200/SP-303 presets; granular freeze/clouds stable

### Phase 8: Scatter / Perf FX (Fase 7)
**Goal:** Scatter, beat-repeat, gater, tape-stop, fills
**Depends on:** Phase 7
**Research:** Unlikely
**DoD:** Live scatter without dropout; fills via trig conditions

### Phase 9: MIDI / Hardware (Fase 8)
**Goal:** MIDI out, clock sync, CC, MIDI learn, hybrid mode
**Depends on:** Phase 3 (sequencer); ordered after FX phases per ESCOPO
**Research:** Likely (clock jitter, TR-8 mapping validation)
**DoD:** Drives real TR-8 in sync; internal+external lanes in same pattern

### Phase 10: UI/UX (Fase 9)
**Goal:** LookAndFeel, all 6 screens, visualizers, undo/redo
**Depends on:** Phases 2–9 (UI binds to engine features)
**Research:** Unlikely (BAQUE-UI-SPEC.md + mockup are source of truth)
**DoD:** Complete drag-to-beat workflow; UI spec applied

### Phase 11: Presets & Library (Fase 10)
**Goal:** Preset system, browser, factory content
**Depends on:** Phase 10
**Research:** Unlikely
**DoD:** Save/load faithful; factory library per aesthetic

### Phase 12: Hardening (Fase 11)
**Goal:** RT-safety audit, optimization, cross-platform, beta
**Depends on:** Phase 11
**Research:** Unlikely
**DoD:** pluginval strict green; 64 voices no dropout; zero audio-thread allocation

### Phase 13: Release (Fase 12)
**Goal:** Docs, installer, signing/notarization, packaging
**Depends on:** Phase 12
**Research:** Unlikely
**DoD:** Signed macOS/Windows installers; manual; v1.0

---
*Roadmap created: 2026-06-04*
*Last updated: 2026-06-04*
