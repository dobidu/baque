---
phase: 09-midi-hardware
plan: 01
type: Summary
about: "BAQUE"
status: complete
completed: 2026-06-08
tests: "173/173"
---

# 09-01 SUMMARY — Per-lane MIDI routing (INT/EXT/BOTH) + EXT out

## Outcome

Keystone do Phase 9: roteamento por lane. Cada lane do StepPattern tem modo (internal/external/both)
+ canal MIDI. Sequencer emite note-on/off das lanes EXT/BOTH num buffer separado no canal da lane;
processBlock funde no MIDI out do host. Modo híbrido (lanes internas + externas no mesmo padrão)
funcional. Resolve o item deferido desde a Fase 3 (per-lane channel filter). **173/173 tests**
(165 + 8: MR1-6, MR8, MR9); build + clang-format limpos.

## What was built / changed

| File | Change |
|------|--------|
| src/audio/lane_routing.h | NOVO — LaneMode enum + LaneRouting (mode[16]/channel[16]) + channel_of() clamp; doc single-writer |
| src/audio/sequencer.h/.cpp | generate() +const LaneRouting* +MidiBuffer* midi_ext (opcionais); por lane: INT→interno (canal 1), EXT→midi_ext no canal da lane; note-on no gate trig/mute/solo, note-off incondicional; note_triggered só INT |
| src/plugin_processor.h/.cpp | membro público LaneRouting lane_routing_; midi_buffer_ext_ + was_playing_; generate recebe routing+ext; stop-flush All-Notes-Off; midi_messages.clear()+addEvents(ext) após consumir MIDI-in |
| tests/test_midi_routing.cpp | MR1-6 (routing), MR8 (stop-flush via PlayHead mock), MR9 (note-off source) |
| tests/CMakeLists.txt | registrar test_midi_routing.cpp |

producesMidi()=true e CMake NEEDS_MIDI_OUTPUT/INPUT já estavam TRUE (Fase 2) — sem mudança necessária.

## Key decisions

- **EXT note-off note = pattern.get_note(lane, prev_step)** (audit SR1): o NoteTracker é populado só
  no caminho INT; lane external-only teria tracker vazio → note-off errado. Nota do padrão é correta.
- **Stop-flush All-Notes-Off (CC123)** por canal EXT/BOTH dedup na borda toca→para (audit M1):
  generate() retorna cedo quando parado e nunca emitiria o note-off → sem flush o hardware seguraria
  a nota (drone). was_playing_ detecta a borda.
- **Ordem processBlock:** consumir MIDI-in do host (scheduler) ANTES de midi_messages.clear()+EXT out.
- **note_triggered só no INT** (mantém invariante do tracker); vel calculada 1× antes do split INT/EXT
  (preserva ordem PRNG).
- **LaneRouting single-writer** — Phase 10 UI → atomics/command queue.

## AC coverage

| AC | Test | Status |
|----|------|--------|
| AC-1 INT/EXT/BOTH | MR1/MR2/MR4 | ✅ |
| AC-2 canal por lane | MR3 | ✅ |
| AC-3 híbrido mesmo padrão | MR5 | ✅ |
| AC-4 EXT respeita gate | MR6 | ✅ |
| AC-5 wiring + producesMidi | MR8 (processBlock) | ✅ |
| AC-6 compat (nullptr) | MR1 | ✅ |
| AC-7 stop-flush (M1) | MR8 | ✅ CC123 ch9 na borda |
| SR1 note-off source | MR9 | ✅ nota 50, não 0 |

## Deviations from plan

Nenhuma deviation de comportamento. producesMidi()/NEEDS_MIDI_OUTPUT já estavam habilitados (Fase 2),
então Task 2 não precisou mexer no CMake/header — só adicionou membros + wiring + stop-flush.

## Boundaries honored

Caminho de voz interno intacto; StepPattern/PerfState/PLockParam/FxParams inalterados; note-off não
gateado; generate() params opcionais (compat); MIDI-in consumido antes de escrever EXT. Nenhum teste
prévio regrediu.

## Next

09-02: MIDI clock master/slave + start/stop/continue + transport sync (clock jitter research).
Hardware sync real (drives TR-8) = verificação manual no 09-04 (DoD da fase).
