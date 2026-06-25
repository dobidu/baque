# BAQUE

Hybrid groovebox plugin — VST3 / AU / Standalone. Micro-timing, lo-fi coloration, and off-grid feel as first-class creative resources.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

---

## What is BAQUE

BAQUE is a drum machine plugin that treats rhythmic imperfection as a feature. The Feel Engine introduces per-step timing offsets, Gaussian humanization, and named groove presets — from locked-in boom-bap to broken scatter. Steps land where you intend them — not always on the grid.

Internally it behaves like a classic sampler: 4×4 pad bank with chop-to-pads, varispeed/time-stretch/granular pitch modes, velocity layers, choke groups, and round-robin. Externally it drives hardware: MIDI clock out (24 ppqn), CC automation, and per-lane routing with a built-in TR-8/TR-8S template that maps note and CC numbers to Roland's published MIDI Implementation Chart.

Named after "baque do beat" — the rhythmic foundation of maracatu and coco de roda from the northeastern Brazilian percussion tradition.

---

## Features

**Feel Engine** (product core)
- Per-step timing offset ±100ms+; Gaussian timing + velocity humanization
- 6 named presets: DL Drunk, BRL Broken, Fly Wonk, Boom-Bap, BNB Loose, Straight
- Seed-based determinism — same seed reproduces identical feel

**Sample Engine**
- 4×4 pad bank; auto-slice + chop-to-pads from transient detection
- 3 pitch modes per pad: varispeed, offline time-stretch (SoundTouch), granular
- Choke groups, velocity layers (per-pad), round-robin playback

**Sequencer**
- 16-step × 16-lane grid; per-step parameter locks (p-locks)
- Trig conditions: fill / not_fill; global MPC-style swing
- Elektron-style seamless pattern switching

**FX Chain**
- LP filter (SVF), reverb, delay, sidechain compressor
- All FX parameters p-lockable per step

**Lo-fi / Granular**
- Bitcrusher + ZOH sample-rate reduction; SP-1200 and SP-303 presets
- Granular engine: grain freeze, spray, pitch spread

**Performance FX**
- Scatter: 10 types + depth (TR-8 style), beat-synced
- Tape Stop, Gater (1/16 beat-synced), trig fills

**MIDI I/O**
- MIDI clock OUT master: 24 ppqn + start/stop/continue, sample-accurate
- CC out via p-locks; MIDI learn (15 targets)
- Per-lane routing: INT / EXT / BOTH + MIDI channel
- TR-8 and TR-8S templates: note + CC map spec-verified vs Roland MIDI Implementation Chart

**UI**
- 6 screens: PERFORM / FX / MIX / PERF FX / MIDI / BROWSER
- 4 themes: Barro, Cinza, Maracatu, Papel
- APVTS-backed undo/redo

**Presets**
- Save/load `.bqpreset` files; 6 factory grooves
- Full engine state v5 serialization (pattern + feel + FX + sample pad params)

---

## Install

Download pre-built installers from the [GitHub Releases page](../../releases).

### Plugin paths

**macOS**
```
~/Library/Audio/Plug-Ins/VST3/BAQUE.vst3
~/Library/Audio/Plug-Ins/Components/BAQUE.component   (AU)
```

**Windows**
```
C:\Program Files\Common Files\VST3\BAQUE.vst3
```

**Linux**
```
~/.vst3/BAQUE.vst3
/usr/lib/vst3/BAQUE.vst3   (system-wide)
```

**Standalone** — run the application directly; no install path required.

---

## Build from source

### Prerequisites

**Linux / WSL2**
```bash
sudo apt install build-essential cmake libasound2-dev \
  libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  libfreetype-dev libfontconfig1-dev libcurl4-openssl-dev xvfb
```

**macOS**
```bash
xcode-select --install
brew install cmake
```

**Windows**
- Visual Studio 2022 (C++ workload)
- CMake 3.22+

### Compile

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

Artifacts at `build/BAQUE_artefacts/Release/`:
- `VST3/BAQUE.vst3` — VST3 plugin bundle
- `Standalone/BAQUE` (Linux) / `Standalone/BAQUE.app` (macOS) / `Standalone/BAQUE.exe` (Windows)
- `AU/BAQUE.component` — AU component (macOS only)

Install to a prefix directory:
```bash
cmake --install build --prefix /path/to/install --config Release
```

---

## Test / Validate

### Unit and integration tests
```bash
ctest --test-dir build --output-on-failure
```

### Plugin validation (pluginval)
```bash
./scripts/run_pluginval.sh
```

Runs pluginval at strictness level 10. Requires pluginval binary in `PATH` or the location expected by the script.

---

## Plugin Identity

Do not change after DAW sessions save state — hosts embed these values in project files.

| Field | Value |
|-------|-------|
| Bundle ID | `br.ufpb.lavid.baque` |
| Manufacturer Code | `Lvd0` |
| Plugin Code | `Bqe1` |
| JUCE version | `8.0.13` |

---

## Contributing

BAQUE is open source under GPLv3. C++17 / JUCE 8.x / CMake. Conventions: snake_case identifiers, English code, Portuguese comments, Catch2 tests required for any new DSP. PRs are welcome.

---

## Third-party / License

- **JUCE 8.0.13** — GPL-3.0 — https://github.com/juce-framework/JUCE
- **SoundTouch fork** — LGPL-2.1 — https://github.com/dobidu/baque-soundtouch
- **Catch2** — Boost-1.0 — tests only
- **pluginval** — ISC — CI/validation only

See [NOTICE](NOTICE) for full attribution.

BAQUE is licensed under the **GNU General Public License v3.0 or later**. See [LICENSE](LICENSE) for the full text.
