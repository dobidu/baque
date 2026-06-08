---
phase: 08-scatter-perf-fx
plan: 01
type: Summary
about: "BAQUE"
status: complete
completed: 2026-06-08
tests: "139/139"
---

# 08-01 SUMMARY — ScatterEngine standalone

## Outcome

ScatterEngine implemented standalone (no wiring/p-lock/APVTS — those are 08-02). Beat-synced
buffer-domain glitch perf FX: repeat / reverse / gate / decimate, type 0-10 + depth [0,1], RT-safe.
**139/139 tests** (129 prior + 10 new scatter), build clean (no warnings), clang-format clean.

## What was built

- `src/audio/scatter_engine.h` + `.cpp` — ScatterEngine
  - `prepare(sr, max_block)` — pre-aloca ring (2 s) + slice congelada (4 × std::vector, off audio thread)
  - `process(buffer, type, depth, bpm)` — in-place, noexcept, zero-alloc
  - `reset()` — zera ring + slice + heads + contadores
  - `slice_length_samples(sr, bpm, subdivision)` — static, beat-sync helper (AC-8)
  - 4 primitivas: repeat (buffer roll), reverse, gate (duty 50%), decimate (sample-and-hold stride)
  - tabela estática `k_types[10]` → (primitiva, subdivisão, stride, also_reverse)
    - 1-3 repeat 1/8·1/16·1/32 · 4-5 reverse 1/8·1/16 · 6-7 gate 1/16·1/32 · 8-9 decimate stride 4·16 · 10 repeat 1/32 reverso
- `tests/test_scatter.cpp` — SC1..SC10 (ASCII-only, `[scatter]` tag), anti-vacuosos
- Registrado em `CMakeLists.txt` (BAQUE sources) e `tests/CMakeLists.txt`

## Key design decision (audit M1 implemented as freeze-by-copy)

A slice é **congelada por cópia** em `latch_slice()` no instante de engajar (borda inativo→ativo):
copia os últimos `slice_len` samples do ring para `slice_l_/slice_r_` e toca essa cópia em loop
durante todo o roll. Garante o invariante M1 (sem auto-captura/feedback) de forma mais forte que o
âncora-em-índice proposto, pois a região lida é uma cópia isolada — write_pos_ nunca a toca.
`slice_pos_` (membro) persiste entre blocos. Fase/máscara/stride compartilhados L+R (SR1).

## Deviations from plan (for UNIFY reconciliation)

1. **CMake target.** Plan disse `src/CMakeLists.txt`; não existe — sources ficam em
   `CMakeLists.txt` raiz via `target_sources(BAQUE ...)`. Registrado lá. (Mesmo padrão de lo_fi/granular.)
2. **slice_length formula.** O comentário-sugestão do plano `(sr*60/bpm)/subdivision` daria 1500
   para 1/16 — inconsistente com AC-8 (6000). AC-8 é autoritativo. Implementado
   `spq*4/subdivision` = 6000 ✓ (subdivision = denominador de nota: 16 = semicolcheia).
3. **M1 "re-âncora em cada fronteira de slice" → "latch único por roll no engaje".** AC-2 exige que
   o conteúdo RECORRA a cada slice_len; re-ancorar em cada fronteira sobre entrada não-periódica não
   recorreria (tocaria slice diferente a cada período). Latch único + loop satisfaz AC-2 e é o
   comportamento musical correto de buffer-roll/beat-repeat (estilo TR-8). Refinamento de spec
   capturado no APPLY (análogo ao 06-04 PD2). Invariante de no-feedback preservado integralmente.

## AC coverage

| AC | Test | Status |
|----|------|--------|
| AC-1 bypass type 0 | SC1 | ✅ exact (1e-6) |
| AC-2 repeat recorrência | SC2 | ✅ out[j]==out[j+slice], não-vacuoso |
| AC-3 reverse | SC3 | ✅ saída decresce sobre rampa |
| AC-4 gate | SC4 | ✅ região gated + nível preservado |
| AC-5 decimate | SC5 | ✅ 16 samples held, próximo bucket difere |
| AC-6 depth 0 exato / 1 diverge | SC6 | ✅ contraste |
| AC-7 RT-safety / finite | SC7 | ✅ 256 blocos × 11 types, isfinite |
| AC-8 beat-sync | SC8 | ✅ 6000/12000/3000 |
| AC-9 reset | SC9 | ✅ só B, sem resíduo A |
| SR1 estéreo | SC10 | ✅ máscara coincide L/R |

## Boundaries honored

fx_chain / fx_params / plock_pattern / plugin_processor intocados. Nenhum dos 129 testes prévios
regrediu. Sem tape-stop, gater-discreto, fills, mute/solo, scene morph (08-03/08-04).

## Next

08-02: wire ScatterEngine no processBlock (pós-FxChain, opera no mix completo) + scatter_type /
scatter_depth p-lockáveis (PLockParam 11/12) + APVTS params + host ppq alignment.
