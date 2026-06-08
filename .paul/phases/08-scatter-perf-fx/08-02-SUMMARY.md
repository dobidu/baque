---
phase: 08-scatter-perf-fx
plan: 02
type: Summary
about: "BAQUE"
status: complete
completed: 2026-06-08
tests: "144/144"
---

# 08-02 SUMMARY — ScatterEngine wired into processBlock

## Outcome

ScatterEngine (08-01) integrado no caminho de áudio: instanciado em BaqueProcessor, preparado, e
chamado em processBlock **depois** de fx_chain_.process() (opera no mix wet completo, estilo TR-8).
Dois params novos — scatter_type (0-10) e scatter_depth (0-1) — expostos via APVTS (default neutro)
**e** p-lock (PLockParam 11/12, count 13). **144/144 tests** (139 + 5 SCH), build + clang-format limpos.

## What was built / changed

| File | Change |
|------|--------|
| src/audio/fx_params.h | +scatter_type, +scatter_depth (default 0/0, neutro) |
| src/audio/plock_pattern.h | PLockParam scatter_type=11, scatter_depth=12, count=13 (era 11) |
| src/plugin_processor.h | #include scatter_engine.h; membro ScatterEngine scatter_ |
| src/plugin_processor.cpp | 2 AudioParameterFloat (scatter_type 0-10 passo 1, scatter_depth 0-1); prepare/reset; snapshot APVTS; 2 cases em apply_plock_batch; scatter_.process() pós-FxChain com jlimit clamp |
| tests/test_scatter_chain.cpp | SCH1-5 (APVTS, enum/count, p-lock dispatch+clamp, ordering, processBlock smoke) |
| tests/test_plock.cpp, test_lo_fi_chain.cpp, test_granular_chain.cpp | assert k_plock_param_count 11→13 |
| CMakeLists (tests) | +test_scatter_chain.cpp |

## Key wiring

- **Ordem:** scatter_.process() roda APÓS fx_chain_.process() em processBlock → glitcha o mix final
  (incl. cauda reverb/delay). Pinado como invariante.
- **Controle duplo:** scatter_type/scatter_depth seguem o padrão de Fase 6 (filter_cutoff): snapshot
  APVTS primeiro, apply_plock_batch sobrescreve por step (p-lock vence).
- **Clamp (audit SR1):** `juce::jlimit(0, k_num_types, roundToInt(scatter_type))` antes de despachar
  — p-lock pode empurrar valor fora de [0,10]; clamp evita o jassert de 08-01.
- **bpm:** de transport_.bpm (default 120 sem playhead).

## AC coverage

| AC | Test | Status |
|----|------|--------|
| AC-1 APVTS params + defaults | SCH1 | ✅ |
| AC-2 enum 13, scatter em 11/12 | SCH2 | ✅ |
| AC-3 p-lock override + clamp (15→10) | SCH3 | ✅ |
| AC-4 ordering pós-FxChain (não-vacuoso) | SCH4 | ✅ max_diff>0.01; bypass exato |
| AC-4b processBlock real finito | SCH5 | ✅ 64 blocos, isfinite |
| AC-5 RT-safety | clamp + inspeção | ✅ scatter_.process noexcept, ring pré-alocado |

## Deviations from plan (for UNIFY)

1. **SCH3 replica o dispatch num helper local de teste** (apply_plock_batch é privado de
   BaqueProcessor). Plano antecipou ("replicar o switch num helper local OU testar o observável").
   Não é deviation de comportamento — só de estratégia de teste.
2. **SR2 grep "==11 → zero hits":** resta 1 hit em test_scatter_chain.cpp:39, mas é o assert de
   ÍNDICE do enum (`PLockParam::scatter_type == 11`), não um assert de COUNT. A intenção do SR2
   (nenhum assert de count obsoleto) está satisfeita — os 3 asserts de count migraram p/ 13.

Nenhuma outra deviation — código seguiu o plano (incl. todas as correções de auditoria M1/SR1/SR2).

## Boundaries honored

scatter_engine.{h,cpp} intocado (consumido); fx_chain fora do caminho (scatter é pós-chain); ordem
PLockParam 0-10 preservada (state recall). Nenhum teste prévio regrediu exceto os 3 asserts de count
(11→13, esperado/Task 1).

## Next

08-03: Tape-stop (speed ramp/resample) + gater como FX discreto de performance.
