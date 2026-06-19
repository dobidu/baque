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
Phases: 12 of 13 complete

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with [INSERTED])

| Phase | Name | Plans | Status | Completed |
|-------|------|-------|--------|-----------|
| 1 | Setup (Fase 0) | 2 | ✅ Complete | 2026-06-04 |
| 2 | Core Audio (Fase 1) | 2 | ✅ Complete | 2026-06-05 |
| 3 | Sequencer Base (Fase 2) | 2 | ✅ Complete | 2026-06-05 |
| 4 | Sample Engine (Fase 3) | 4 | ✅ Complete | 2026-06-05 |
| 5 | Feel Engine ⭐ (Fase 4) | 3 | ✅ Complete | 2026-06-06 |
| 6 | FX + P-locks (Fase 5) | 4 | ✅ Complete | 2026-06-07 |
| 7 | Lo-fi + Granular (Fase 6) | 4 | ✅ Complete | 2026-06-08 |
| 8 | Scatter / Perf FX (Fase 7) | 4 | ✅ Complete | 2026-06-08 |
| 9 | MIDI / Hardware (Fase 8) | 4 | ✅ Complete | 2026-06-10 |
| 10 | UI/UX (Fase 9) | 7 | ✅ Complete | 2026-06-18 |
| 11 | Presets & Library (Fase 10) | 2 | ✅ Complete | 2026-06-19 |
| 12 | Hardening (Fase 11) | 3 | ✅ Complete | 2026-06-19 |
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
- [x] 04-01: Pad bank + per-pad playback — note→pad routing, varispeed, reverse, gain/pan ✅ 2026-06-05
- [x] 04-02: Choke groups + velocity layers + round-robin + ADSR/play modes ✅ 2026-06-05
- [x] 04-03: Auto-slice (transient detection) + chop-to-pads ✅ 2026-06-05
- [x] 04-04: R&D-TS fork v1 integration — offline time-stretch ✅ 2026-06-05

### Phase 5: Feel Engine ⭐ (Fase 4) ✅
**Goal:** Micro-timing, humanize, dual-grid, no-grid, feel presets — *product core*
**Depends on:** Phase 4
**Research:** Likely (groove template design, sub-sample scheduling)
**Research topics:** MPC swing curves, Dilla/Burial timing analysis, seed determinism
**DoD:** "Dilla Drunk" and "Burial Broken" perceptible and seed-reproducible ✅ 2026-06-06

**Plans:**
- [x] 05-01: FeelPattern + FeelEngine + per-step timing offset ✅ 2026-06-05
- [x] 05-02: Gaussian humanize (Box-Muller) + xorshift32 PRNG seeding ✅ 2026-06-06
- [x] 05-03: Feel presets (6 named grooves) + Phase 5 DoD verification ✅ 2026-06-06

### Phase 6: FX + P-locks (Fase 5) ✅
**Goal:** Filters, reverb, delay, sidechain, EQ + p-locks
**Depends on:** Phase 5 (p-locks need sequencer + params)
**Research:** Unlikely (juce::dsp + established DSP)
**DoD:** P-lock automates any param per step; sidechain pump functional ✅ 2026-06-07

**Plans:**
- [x] 06-01: P-lock system infrastructure (PLockPattern, FxParams, 92/92 tests) ✅ 2026-06-07
- [x] 06-02: FxChain DSP (LP filter + reverb + delay + SmoothedValue, 98/98 tests) ✅ 2026-06-07
- [x] 06-03: SidechainCompressor (IIR envelope follower + 8:1 gain computer, 104/104 tests) ✅ 2026-06-07
- [x] 06-04: Phase 6 DoD — integration tests (p-lock→FxChain→output, 109/109 tests) ✅ 2026-06-07

### Phase 7: Lo-fi + Granular (Fase 6) ✅
**Goal:** Bitcrush, SR reduction, vinyl, tape; granular engine. Integrates R&D-TS refinements.
**Depends on:** Phase 6 + R&D-TS advanced work
**DoD:** SP-1200/SP-303 presets; granular freeze/clouds stable ✅ 2026-06-08

**Plans:**
- [x] 07-01: LoFiProcessor standalone (bitcrusher + ZOH SR reduction, 114/114 tests) ✅ 2026-06-08
- [x] 07-02: LoFiProcessor wired into FxChain; bit_depth/sr_factor p-lockable (119/119 tests) ✅ 2026-06-08
- [x] 07-03: GranularProcessor standalone (grain pool + freeze + spray + pitch_spread, 124/124 tests) ✅ 2026-06-08
- [x] 07-04: GranularProcessor wired into FxChain; granular params p-lockable; Phase 7 DoD (129/129 tests) ✅ 2026-06-08

### Phase 8: Scatter / Perf FX (Fase 7) ✅
**Goal:** Scatter, beat-repeat, gater, tape-stop, fills
**Depends on:** Phase 7
**Research:** Unlikely
**DoD:** Live scatter without dropout; fills via trig conditions ✅ 2026-06-08

**Plans (4-plan decomposition):**
- [x] 08-01: ScatterEngine standalone (ring + repeat/reverse/gate/decimate + type 1-10 + depth + beat-sync, 139/139 tests) ✅ 2026-06-08
- [x] 08-02: Wire into processBlock (post-FxChain) + scatter_type/scatter_depth APVTS + p-lockable (PLockParam 11/12, count 13, 144/144 tests) ✅ 2026-06-08
- [x] 08-03: TapeStop (halt→silence) + Gater (1/16 beat-synced) perf FX, post-scatter, APVTS + p-lock (PLockParam 13/14, count 15, 153/153 tests) ✅ 2026-06-08
- [ ] 08-04: Fills via trig conditions (extend StepPattern) + mute/solo groups + Phase 8 DoD (scene morph deferred → Phase 10/11)

### Phase 9: MIDI / Hardware (Fase 8)
**Goal:** MIDI out, clock sync, CC, MIDI learn, hybrid mode
**Depends on:** Phase 3 (sequencer); ordered after FX phases per ESCOPO
**Research:** Likely (clock jitter, TR-8 mapping validation)
**DoD:** Drives real TR-8 in sync; internal+external lanes in same pattern

**DoD:** Drives real TR-8 in sync; internal+external lanes in same pattern ✅ 2026-06-10 (merged MIDI + note/CC map spec-verified vs Roland chart)

**Plans (4-plan decomposition):**
- [x] 09-01: Per-lane routing (INT/EXT/BOTH + channel) + Sequencer EXT MIDI out + processBlock merge + stop-flush (173/173 tests) ✅ 2026-06-08
- [x] 09-02: MIDI clock OUT master (24 ppqn + start/stop/continue + last_tick_ monotonic guard, 182/182 tests) ✅ 2026-06-08
- [x] 09-03: CC out (p-locks→CC) + MIDI in + MIDI learn (195/195 tests) ✅ 2026-06-09
- [x] 09-04: TR-8/TR-8S templates + Phase 9 DoD; AC-6 resolved verified-by-spec (Tier-3: note/CC map machine-checked vs Roland's published MIDI Implementation Chart; value-safe CCs baked; 209/209 tests) ✅ 2026-06-10

### Phase 10: UI/UX (Fase 9)
**Goal:** LookAndFeel, all 6 screens, visualizers, undo/redo
**Depends on:** Phases 2–9 (UI binds to engine features)
**Research:** Unlikely (BAQUE-UI-SPEC.md + mockup are source of truth)
**DoD:** Complete drag-to-beat workflow; UI spec applied

**Plans (7-plan decomposition, confirmed 2026-06-10):**
- [x] 10-01: UI→engine command queue + atomicization (perf_state/lane_routing/clock_master/cc_out/pad-params/pattern/p-lock/template) + UiStateSnapshot ✅ 2026-06-15
- [x] 10-02: LookAndFeel + design system (4 temas, knobs/arcs, faders, meters, fontes, grão) + header/transport + NAV shell ✅ 2026-06-15
- [x] 10-03: PERFORM screen — pads 4×4, sequencer grid (TODAS/FOCO), sample editor ✅ 2026-06-16
- [x] 10-04: Feel Field radial visualizer (parallel candidate after 10-02) ✅ 2026-06-16
- [x] 10-05: FX + MIX screens ✅ 2026-06-17
- [x] 10-06: PERF FX + MIDI screens ✅ 2026-06-17
- [x] 10-07: BROWSER + undo/redo + Phase 10 DoD (drag-to-beat) ✅ 2026-06-18

### Phase 11: Presets & Library (Fase 10)
**Goal:** Preset system, browser, factory content
**Depends on:** Phase 10
**Research:** Unlikely
**DoD:** Save/load faithful; factory library per aesthetic

**Plans (2-plan decomposition):**
- [x] 11-01: Full engine state v5 (StepPattern + PLockPattern + FeelPattern + SamplePad params/paths) + PresetManager (save/load/list *.bqpreset) + round-trip tests ✅ 2026-06-19
- [x] 11-02: Preset browser UI (BrowserScreen Samples|Presets tabs) + FactoryPresetLibrary (6 aesthetics) + Phase 11 DoD ✅ 2026-06-19

### Phase 12: Hardening (Fase 11)
**Goal:** RT-safety audit, optimization, cross-platform, beta
**Depends on:** Phase 11
**Research:** Unlikely
**DoD:** pluginval strict green; 64 voices no dropout; zero audio-thread allocation

**Plans (3-plan decomposition):**
- [x] 12-01: RT-safety audit + zero-alloc hardening (ScopedAudioThreadGuard + MidiBuffer pre-size + P12D1) ✅ 2026-06-19
- [x] 12-02: pluginval strict green + parameter range validation + P12D2 + P12D2b ✅ 2026-06-19
- [x] 12-03: 64-voice polyphony stress test (P12D3) + zero RT-alloc (P12D4) + Phase 12 DoD ✅ 2026-06-19

### Phase 13: Release (Fase 12)
**Goal:** Docs, installer, signing/notarization, packaging
**Depends on:** Phase 12
**Research:** Unlikely
**DoD:** Signed macOS/Windows installers; manual; v1.0

---
*Roadmap created: 2026-06-04*
*Last updated: 2026-06-19 — Phase 12 complete; Phase 13 (Release) next*
