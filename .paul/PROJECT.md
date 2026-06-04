---
description: "Producers build beats with authentic micro-timing feel — off-grid groove, lo-fi color, and controlled error as first-class features"
type: Project
about: "BAQUE"
---

# BAQUE

## What This Is

A hybrid groovebox audio plugin (VST3 / AU / Standalone; CLAP post-v1): plays internal samples (MPC/SP style) and/or drives external hardware via MIDI (TR-8/TR-8S, drum machines, synths). Its core identity is **time** — micro-timing, off-grid swing, Dilla "drunk feel", Burial's total absence of grid. Where most drum machines force quantization, BAQUE treats rhythmic deviation, sound degradation, and controlled error as first-class resources. Built in C++17 with JUCE 8.x and CMake. Open source (GPLv3).

## Core Value

Producers can build beats with the authentic feel of a specific lineage (Dilla, Burial, Aphex, FlyLo, Madlib, Alchemist, Bonobo, Teebs) — micro-timing, lo-fi coloration, and hybrid sample+MIDI workflow in one real-time-safe instrument.

## Current State

| Attribute | Value |
|-----------|-------|
| Type | Application (audio plugin) |
| Version | 0.0.0 |
| Status | Initializing |
| Last Updated | 2026-06-04 |

## Requirements

### Core Features

1. **Feel Engine** *(product core)* — per-step timing offset (±50ms+), global + per-lane swing, dual-grid/poly-feel, gaussian humanize (timing + velocity), no-grid mode, feel presets (Dilla Drunk, Burial Broken, FlyLo Wonk, Boom-Bap, Bonobo Loose, Straight), seed-based determinism
2. **Sample Engine** — 4×4 pads (expandable 8×8), banks, auto-slice + chop-to-pads, 3 pitch modes (varispeed / time_stretch / granular), choke groups, velocity layers, round-robin, pitch wavering/drift
3. **Sequencer** — per-lane pattern length (polymeter), per-step probability, ratchets (2–16), Elektron-style trig conditions, euclidean generator, p-locks, pattern chaining, mutation/randomization
4. **Lo-fi Coloration** — bitcrush, SR reduction (SP-1200 ~26kHz / SP-303 / 8-bit presets), vinyl sim, tape, "Age" macro — per pad / track / master
5. **Granular Engine** — clouds/freeze, spray, pitch spread, pre-allocated grain pool
6. **FX Chain + P-locks** — multimode filters (ladder/diode/formant), reverb w/ ducking, tape/dub delays, sidechain comp (internal trigger), EQ, oversampling, parameter smoothing
7. **Scatter / Performance FX** — TR-8-style scatter (type 1–10 + depth), beat-repeat, gater, tape-stop, fills, mute/solo groups, scene morph
8. **MIDI I/O** — note map per pad/lane, clock master/slave, CC out, MIDI learn, hybrid mode (INT/EXT/BOTH lanes), TR-8/TR-8S templates
9. **Mixer & Routing** — per-track gain/pan/mute/solo/sends, multi-out stems, metering
10. **Browser** — samples + presets, tags, aesthetic filters, auto-audition

### Validated (Shipped)
None yet.

### Active (In Progress)
None yet.

### Planned (Next)
Full phase breakdown in .paul/ROADMAP.md (13 phases + parallel R&D-TS track, from BAQUE-ESCOPO.md §12).

### Out of Scope
- AAX (requires Avid SDK + signing) — out of v1
- CLAP — post-v1 (via clap-juce-extensions)
- LV2 — out of initial scope
- Full song mode in v1 — pattern chaining ships in v1, full song mode later (open decision §14.6)

## Target Users

**Primary:** Beat-makers / producers who admire hardware logic (MPC, Digitakt, TR-808/909, SP-404) and want feel-first groove programming
- Work inside a DAW (plugin) or standalone
- Mix internal samples with external hardware (TR-8/TR-8S)
- Value lo-fi character and off-grid timing over surgical precision

## Context

**Technical Context:**
- Reference docs in repo root: `BAQUE-ESCOPO.md` (scope, source of truth), `BAQUE-UI-SPEC.md` (UI spec, 6 screens), `BAQUE_MOCKUP.html` (interactive prototype — visual source of truth)
- Time-stretch via dedicated optimized **SoundTouch fork** (LGPL v2.1, separate repo) — significant R&D track (§4.11): transient preservation, content adaptation, formant preservation, stereo coherence, clean vs character modes, RT-safety wrapper, evaluation harness
- Architecture: PluginProcessor + APVTS/ValueTree state, RT-safe voice pool, lock-free queues (juce::AbstractFifo), audio/message/background thread separation

## Constraints

### Technical Constraints
- **Zero allocation / locks / IO on audio thread** — hard requirement
- ≥64 simultaneous voices without dropout on modern hardware
- Sample-accurate timing; sub-sample for feel engine
- 100% faithful state recall; smooth automation; no clicks on start/stop/loop
- macOS: arm64 + x86_64 universal binary, codesign + notarization; Windows: x64; Linux: x64 (full v1 target — VST3 + Standalone)
- C++17, JUCE 8.x, CMake; snake_case, English identifiers, Portuguese comments, `_` suffix for private members, no raw new/delete
- Catch2 unit tests, pluginval strict in CI, clang-format + clang-tidy

### Business Constraints
- Open source — JUCE under GPLv3; BAQUE license GPLv3 recommended (LGPL ⊂ GPLv3 for SoundTouch fork)
- Test hardware available: MacBook Air M4 + Ryzen/RTX desktop (covers both primary targets)
- MIDI out must be validated against real TR-8/TR-8S

### Compliance Constraints
- LICENSE + NOTICE files (third-party attributions) required in repo
- SoundTouch fork stays in separate repo under LGPL v2.1, modifications documented

## Key Decisions

| Decision | Rationale | Date | Status |
|----------|-----------|------|--------|
| Name: BAQUE | "Baque do beat" + northeastern Brazilian root; not an instrument/rhythm name | 2026-06-04 | Active |
| Time-stretch: optimized SoundTouch fork | LGPL compatible; dedicated R&D track for transient/formant quality | 2026-06-04 | Active |
| CLAP post-v1 | Via clap-juce-extensions, no SDK cost; v1 = VST3 + AU + Standalone | 2026-06-04 | Active |
| Open source, GPLv3 | JUCE free for open source; license compatibility direct | 2026-06-04 | Active |
| Feel Engine is the product core | Differentiator vs "another sampler"; everything orbits it | 2026-06-04 | Active |
| Linux is full v1 target | VST3 + Standalone released; CI matrix 3 OSes from Phase 1 (resolves ESCOPO §14.9) | 2026-06-04 | Active |
| Plugin identity locked | Bundle br.ufpb.lavid.baque, Manufacturer 'Lvd0', Plugin 'Bqe1', JUCE 8.0.13 — permanent once DAW sessions save state | 2026-06-04 | Active |

**Open decisions (ESCOPO §14):** sample embed in presets (#5 — suggested: optional, off by default, "collect & save"), song mode depth v1 (#6), multi-out in v1 vs v1.1 (#7). ~~Linux (#9)~~ resolved: full v1 target.

## Success Metrics

| Metric | Target | Current | Status |
|--------|--------|---------|--------|
| pluginval strict | Green on macOS + Windows + Linux | - | Not started |
| Polyphony | ≥64 voices, no dropout | - | Not started |
| Audio thread allocations | Zero (instrumented asserts) | - | Not started |
| Feel reproducibility | "Dilla Drunk" / "Burial Broken" perceptible + seed-reproducible | - | Not started |
| Hardware sync | Drives real TR-8 in sync (clock jitter measured) | - | Not started |
| SoundTouch fork | Transient preservation beats baseline SoundTouch on harness metrics | - | Not started |
| DAW compatibility | Reaper, Live, Logic, Bitwig, FL | - | Not started |

## Tech Stack / Tools

| Layer | Technology | Notes |
|-------|------------|-------|
| Language | C++17 | snake_case, RAII, no raw new/delete |
| Framework | JUCE 8.x | GPLv3 (free for open source) |
| Build | CMake | clang-format + clang-tidy in CI |
| Tests | Catch2 + pluginval | DSP deterministic tests, golden-file audio regression |
| Time-stretch | SoundTouch fork (LGPL v2.1) | Separate repo, WSOLA + custom R&D |
| DSP | juce::dsp + custom | Oversampling on non-linear stages |
| Formats | VST3, AU, Standalone | CLAP post-v1 |
| CI | Multi-platform (macOS + Windows + Linux) | Build + tests + pluginval |
| UI | JUCE LookAndFeel custom, possible OpenGL | 4 themes (Barro/Cinza/Maracatu/Papel), Space Grotesk + JetBrains Mono |

## Links

| Resource | URL |
|----------|-----|
| Scope doc | BAQUE-ESCOPO.md (repo root) |
| UI spec | BAQUE-UI-SPEC.md (repo root) |
| Prototype | BAQUE_MOCKUP.html (visual source of truth) |

---
*PROJECT.md — Updated when requirements or context change*
*Last updated: 2026-06-04*
