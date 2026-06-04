# BAQUE

**Hybrid groovebox audio plugin** — VST3 / AU / Standalone. Samples internally (MPC/SP style) and drives external hardware via MIDI (TR-8/TR-8S). Core identity: micro-timing, off-grid feel, and lo-fi coloration as first-class resources.

Open source — [GPLv3](LICENSE).

---

## Plugin Identity (permanent — do not change after DAW sessions save state)

| Field | Value |
|-------|-------|
| Bundle ID | `br.ufpb.lavid.baque` |
| Manufacturer Code | `Lvd0` |
| Plugin Code | `Bqe1` |
| JUCE version | `8.0.13` |

---

## Build

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
cmake --build build -j
```

Artifacts at `build/BAQUE_artefacts/Release/`:
- `VST3/BAQUE.vst3` — VST3 plugin
- `Standalone/BAQUE` — standalone application
- `AU/` — AU component (macOS only)

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

### Validate plugin (pluginval)

```bash
./scripts/run_pluginval.sh
```

---

## Documentation

- [BAQUE-ESCOPO.md](BAQUE-ESCOPO.md) — full scope, architecture, roadmap (source of truth)
- [BAQUE-UI-SPEC.md](BAQUE-UI-SPEC.md) — UI specification (6 screens)
- [BAQUE_MOCKUP.html](BAQUE_MOCKUP.html) — interactive prototype (visual source of truth)

---

## Third-party

- **JUCE 8.0.13** — GPL-3.0 — https://github.com/juce-framework/JUCE
- **SoundTouch fork** — LGPL-2.1 — separate repo (planned Phase 4)
- **Catch2** — Boost-1.0 — tests only

See [NOTICE](NOTICE) for full attribution.

---

## License

BAQUE is licensed under the **GNU General Public License v3.0 or later**.
See [LICENSE](LICENSE) for the full text.
