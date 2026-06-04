# BAQUE — Beat Machine & Groovebox (JUCE)
## Documento de Escopo do Projeto

> **Status:** Draft v0.2 — escopo para criação do projeto
> **Nome:** `BAQUE` — o baque do beat (o impacto / a queda) com raiz nordestina
> **Licença:** **Open source** (JUCE sob GPLv3; ver §13 / §14)
> **Tipo:** Plugin de áudio (VST3 / AU / Standalone no núcleo; **CLAP em fase posterior**) + host de sample + sequenciador + controlador MIDI de hardware
> **Linguagem:** C++17, framework JUCE 8.x, build via CMake
> **Objetivo:** Ferramenta de grau profissional, real-time-safe, pronta para uso ao fim do desenvolvimento

---

## 1. Visão e Filosofia

BAQUE é uma **groovebox híbrida**: ela toca samples internamente (estilo MPC/SP) **e/ou** controla hardware externo via MIDI (TR-8/TR-8S, drum machines, sintetizadores), permitindo construir beats com o "feel" característico de uma linhagem específica de produtores.

O eixo conceitual não é "mais um sampler". É uma máquina cuja **identidade central é o tempo** — o micro-timing, o swing fora da grade, o "drunk feel" do Dilla, a ausência total de grade do Burial. A maioria dos drum machines força a quantização; BAQUE faz o oposto: trata o desvio rítmico, a degradação do som e o erro controlado como recursos de primeira classe.

**Três princípios de design:**

1. **Feel-first.** O motor de micro-timing/humanização é o módulo mais importante e o mais sofisticado. Tudo mais orbita ao redor dele.
2. **Coloração como instrumento.** Bitcrush, vinyl, tape, saturação não são "efeitos" — são parte do timbre fundamental do gênero. Devem ser tão fáceis de acessar quanto o volume.
3. **Híbrido sem fricção.** A mesma faixa pode tocar um sample interno OU disparar uma nota MIDI para uma TR-8. Beats reais misturam as duas coisas.

---

## 2. Decodificação Estética — de artista para feature

Esta seção é a fonte de verdade do que diferencia BAQUE. Cada estética é traduzida em requisitos concretos de DSP e de fluxo.

### 2.1 J Dilla
- **Assinatura:** quantize desligado no MPC3000; kicks/snares "arrastados" enquanto hi-hats balançam — sensação de duas grades de tempo simultâneas (poliritmia de feel). Velocity humano, baixos filtrados, chops de soul/jazz.
- **Requisitos:** offset de timing por step (±50 ms+); modo **"dual-grid"** (uma lane quantizada + outra deslocada); humanize gaussiano de timing e velocity; presets de feel ("Dilla Drunk"); filtro de grave musical.

### 2.2 Burial
- **Assinatura:** *sem grade nenhuma* (posiciona eventos olhando a waveform); 2-step quebrado; crackle de vinil; percussão de "found sound" (cliques, isqueiro); vocais fantasma com pitch/formant deslocado e wavering de fita; reverb cavernoso; sub-bass; chuva/ambiência.
- **Requisitos:** modo **"no-grid"** (snap desligável, posicionamento livre em ms); **wavering/drift** de pitch (random walk lento, tipo fita gasta); chop vocal com formant shift e reverso; reverb grande com ducking; camadas de ambiência/texturas; biblioteca de percussão "found sound".

### 2.3 Aphex Twin
- **Assinatura:** programação de bateria extrema (drill'n'bass), rolls/ratchets, probabilidade, sequenciamento generativo/algorítmico, time-stretch radical, FM/granular, bitcrush, microtonalidade.
- **Requisitos:** **ratchets/rolls** por step (sub-divisões 2–16); **probabilidade** por step; **trig conditions** estilo Elektron (1:2, fill, A/B, prev/!prev); gerador **euclidiano** por lane; randomização/mutação de padrões; time-stretch independente de pitch.

### 2.4 Flying Lotus
- **Assinatura:** beat scene LA, woozy/jazzy, swing solto, sub pesado, glitch, sidechain pump, pitch bends, maximalismo psicodélico.
- **Requisitos:** swing variável + humanize; sidechain/ducking com trigger interno de qualquer lane; pitch envelope/bend por pad; camadas densas (polifonia alta); modulação de filtros.

### 2.5 Madlib
- **Assinatura:** lo-fi empoeirado, workflow rápido (Beat Konducta), som SP-303/SP-1200 (crunch 12-bit), varispeed (pitch muda a duração), saturação de fita, loops com mínima limpeza, "found records".
- **Requisitos:** **redução de bit-depth/sample-rate** por pad (modelos SP-1200 ~26 kHz / SP-303); modo de pitch **varispeed** (altera comprimento); saturação de fita; chop rápido de loop para pads; workflow "drag → fatia → toca" em segundos.

### 2.6 The Alchemist
- **Assinatura:** boom-bap cinematográfico, samples de soul/psych/prog, loops filtrados, baterias secas e pesadas, estética de vinil.
- **Requisitos:** loop-sampler com filtro passa-altas/baixas musical; layering de bateria seca; coloração de vinil; templates de feel "boom-bap" (mais reto que Dilla, mas com peso).

### 2.7 Bonobo
- **Assinatura:** orgânico, downtempo, lush, percussão intrincada e suave, instrumentação ao vivo + eletrônica, texturas de "world music", som polido e quente.
- **Requisitos:** alta qualidade de reprodução (sem crunch obrigatório); round-robin/velocity layers para naturalidade; reverbs/delays sofisticados; modulação evolutiva; suporte a samples longos/melódicos.

### 2.8 Teebs
- **Assinatura:** Brainfeeder pictórico, beats ambientes, texturas, samples de harpa/kalimba/mallets, granular, reverberante, colagem gentil.
- **Requisitos:** **engine granular** robusto (clouds/freeze); reverbs longos modulados; layering textural; spray/jitter de grãos; pitch spread.

### 2.9 Matriz consolidada artista → módulo

| Necessidade central | Módulo BAQUE | Quem puxa |
|---|---|---|
| Micro-timing / off-grid / dual-grid | **Feel Engine** | Dilla, Burial, FlyLo |
| No-grid / wavering de pitch | **Feel Engine** + Sample Engine | Burial |
| Ratchets / probabilidade / trig conditions / euclidiano | **Sequenciador** | Aphex |
| 12-bit / SP / vinyl / tape | **Coloração Lo-fi** | Madlib, Alchemist, Burial |
| Granular / clouds / texturas | **Engine Granular** | Teebs, Aphex, FlyLo |
| Chop vocal / formant / reverso | **Sample Engine** | Burial, FlyLo |
| Sidechain pump | **FX Chain** | FlyLo, Bonobo |
| Loop filtrado boom-bap | **Sample Engine** + **FX** | Alchemist, Madlib |
| Performance glitch ao vivo | **Scatter / Performance FX** | (estilo TR-8) todos |
| Round-robin / qualidade limpa | **Sample Engine** | Bonobo |

---

## 3. Pilares Funcionais (resumo executivo)

1. **Sample Engine** — carregar, fatiar, afinar e modificar samples (MPC + SP).
2. **Sequenciador** — padrões por step, polimetria, probabilidade, ratchets, p-locks, song mode.
3. **Feel Engine** — micro-timing, swing, humanize, dual-grid, no-grid, presets de feel. *(núcleo do produto)*
4. **Coloração Lo-fi** — bitcrush, sample-rate reduction, vinyl, tape, saturação.
5. **Engine Granular** — texturas, clouds, freeze.
6. **FX Chain** — filtros, reverb, delay, modulação, distorção, sidechain, EQ — com automação por step (p-locks).
7. **Scatter / Performance FX** — beat-repeat, gate, stutter, tape-stop, scatter (estilo TR-8), fills.
8. **MIDI I/O** — controlar hardware externo (TR-8/TR-8S), sync de clock, CC out, MIDI learn.
9. **Mixer & Roteamento** — por faixa, sends, multi-out para o DAW.
10. **Browser** — samples e presets, com tags e preview.

---

## 4. Especificação de Módulos

### 4.1 Sample Engine (MPC / SP)
- Grade de pads configurável (4×4 padrão, expansível a 8×8), múltiplos **banks**.
- Import: WAV, AIFF, FLAC, MP3, OGG; drag-and-drop; resampling automático para a taxa do host.
- **Auto-slice** por detecção de transientes + slice manual com marcadores; **chop-to-pads** (espalha fatias de um loop pelos pads, estilo MPC).
- Por pad: start/end/loop points, gain, pan, **pitch** (semitons + cents finos), envelope de amplitude (ADSR), modos one-shot / gate / loop, **reverso**.
- **Modos de pitch:**
  - `varispeed` — muda a duração junto (caráter SP/vinil; Madlib).
  - `time_stretch` — pitch independente da duração (loops, vocais; via biblioteca — ver §13/§14).
  - `granular` — pitch por granulação (texturas; Teebs/Aphex).
- **Choke groups** (ex.: open/closed hi-hat).
- **Velocity layers** e **round-robin** (naturalidade; Bonobo).
- Filtro por pad + sends de FX por pad.
- **Wavering/drift** de pitch por pad (random walk lento — caráter de fita gasta; Burial).
- Renderização de waveform em thread de background (não bloquear áudio).

### 4.2 Sequenciador & Padrões
- Sequenciador por **steps**, uma lane por pad/faixa.
- **Comprimento de padrão independente por lane** (polimetria) e resolução variável (1/8 a 1/64, tripletos).
- **Probabilidade por step** (0–100%).
- **Ratchets / rolls** por step (2–16 sub-disparos, com curva de velocity).
- **Trig conditions** estilo Elektron: `1:2`, `2:4`, `fill`, `not-fill`, `A/B`, `prev`, `!prev`, `rnd%`.
- **Gerador euclidiano** por lane (pulsos/steps/rotação).
- **P-locks (parameter locks):** qualquer parâmetro de pad/FX automatizável por step.
- **Pattern chaining**, **scenes** e **song mode** (sequência de padrões com repetições).
- **Mutação/randomização** de padrão (knob "amount") — sementes de variação à la Aphex.
- Timing **sample-accurate**; sub-sample para o feel (ver §4.3).

### 4.3 Feel Engine / Micro-timing *(núcleo)*
- **Offset de timing por step** em ms/ticks/samples (faixa ampla, ±50 ms+), por lane.
- **Swing global** (MPC %, 50–75%) + **swing por lane**.
- **Dual-grid / poly-feel:** lanes podem rodar em grades de feel distintas simultaneamente (kick quantizado + snare atrasado + hats adiantados → "Dilla drunk").
- **Humanize:** jitter gaussiano/uniforme em timing **e** velocity, com `amount` e `drift` (random walk lento).
- **No-grid mode:** snap desligável; eventos posicionados livremente em ms (Burial).
- **Feel presets** (templates de groove): `Dilla Drunk`, `Burial Broken`, `FlyLo Wonk`, `Boom-Bap`, `Straight`, `Bonobo Loose` — aplicáveis e editáveis; importação/criação de templates próprios.
- Determinismo opcional via **seed** (para reprodutibilidade ao bouncar).

### 4.4 Coloração Lo-fi
- **Bitcrusher** (bit-depth variável, opções de dither).
- **Sample-rate reducer / decimator** com presets de caráter: `SP-1200 (~26 kHz)`, `SP-303`, `8-bit`.
- **Vinyl sim:** crackle, pops, ruído de superfície, wow de RPM, soma mono, rumble.
- **Tape:** wow & flutter, saturação, hiss, head bump.
- Macro **"Age"** que combina os anteriores num único knob.
- Aplicável por pad, por faixa e no master.

### 4.5 Engine Granular / Textura
- Reprodução granular de samples: grain size, density, position, **spray**, pitch spread, formato de janela.
- Modos **freeze** e **clouds**.
- Grãos reversos, jitter, modulação de posição.
- Otimizado para CPU (pool de grãos pré-alocado).

### 4.6 Cadeia de Efeitos + P-locks
- **Filtros:** multimodo (LP/HP/BP/notch) com modelos: ladder 24 dB (Moog-like), "grimy"/diode, **formant/vowel**.
- **Reverb:** algorítmico de alta qualidade (espaços grandes p/ Burial/Teebs), plate, spring; com modulação e **ducking**.
- **Delay:** tape delay (com wow), ping-pong, granular, dub (feedback filtrado).
- **Modulação:** chorus, phaser, flanger.
- **Distorção/saturação:** tube, tape, wavefolder, bitcrush.
- **Sidechain compressor** com trigger interno de qualquer lane (pump FlyLo/Bonobo) + sidechain externo (DAW).
- **EQ** paramétrico.
- **Oversampling** nos estágios não-lineares; **parameter smoothing** em todos os parâmetros contínuos.
- Todos os parâmetros expostos para **p-locks** e automação do host.

### 4.7 Scatter / Performance FX (estilo TR-8)
- **Scatter:** banco de padrões de glitch (repeat/reverse/gate/decimate) com `type` (1–10) e `depth`.
- **Beat-repeat / stutter / buffer roll** (sub-divisões selecionáveis).
- **Gater** com padrões.
- **Tape-stop / vinyl-brake / riser / down-sampler sweep**.
- **Fills** e **mute/solo groups**; **morph** entre cenas/padrões.
- **Performance pads** para disparo ao vivo (mapeáveis a MIDI controller).

### 4.8 MIDI I/O & Controle de Hardware
- **MIDI out** para dirigir hardware (TR-8 / TR-8S, drum machines, sintes): mapeamento de nota por pad/lane, canal configurável, **MIDI clock master/slave**, start/stop/continue, **CC out** (controlar parâmetros/scatter da TR-8 a partir dos p-locks).
- **MIDI in:** tocar pads, **MIDI learn** para qualquer parâmetro.
- Sync: MIDI clock, host transport (modo plugin), opção MTC.
- **Modo híbrido:** lanes internas (sample) e externas (MIDI) coexistindo no mesmo padrão.
- Templates de mapeamento prontos para TR-8/TR-8S.

### 4.9 Mixer & Roteamento
- Por faixa: gain, pan, mute, solo, sends (2+), escolha de saída.
- **Multi-out** para o DAW (stems por faixa) quando em modo plugin.
- Metering (peak/RMS) por faixa e no master.

### 4.10 Browser de Samples & Presets
- Navegação de samples com **preview** (auto-audition) e forma de onda.
- **Tags** e busca; favoritos.
- Browser de presets (fábrica + usuário) com categorias e tags por estética ("Dilla", "Burial", "Teebs"...).

### 4.11 Engine de Time-Stretch & Pitch — fork otimizado do SoundTouch *(trilha de P&D)*

Base = **SoundTouch** (LGPL v2.1, algoritmo WSOLA time-domain). Mantemos um **fork dedicado e otimizado** para beat-making lo-fi. Esta é uma trilha de P&D significativa, com pesquisa + implementação + benchmark.

**Três caminhos de processamento:**
- **Offline (preparo de sample):** stretch de alta qualidade ao carregar/fatiar (tempo-match de loops), renderizado no background thread → custo zero no audio thread.
- **Realtime varispeed:** mudança de rate barata (pitch e duração juntos) no audio thread — caráter SP/vinil (Madlib).
- **Realtime time-stretch:** pitch independente da duração em tempo real (modo leve, com budget de CPU) — tocar pads transpostos preservando o comprimento.

**Eixos de pesquisa/otimização** (cada um com benchmark e DoD próprio):
1. **Preservação de transientes** — detecção de transientes + bypass/realinhamento do WSOLA nos ataques, mantendo o *punch* da bateria (SoundTouch borra transientes).
2. **Adaptação por conteúdo** — auto-seleção de `sequence`/`seekwindow`/`overlap` conforme material (percussivo / tonal / vocal), guiada por análise de conteúdo no preparo offline.
3. **Preservação de formantes** no pitch-shift (vocais — chops fantasma do Burial); correção de envelope espectral.
4. **Coerência estéreo** — seek linkado entre canais para evitar deriva de imagem.
5. **Modos `clean` vs `character`** — abraçar warble/artefato como recurso estético controlável (lo-fi) ou minimizá-lo (limpo).
6. **RT-safety** — wrapper sobre o SoundTouch com buffers **pré-alocados** (o SoundTouch aloca internamente); nenhuma alocação no audio thread; pool por voice.
7. **Híbrido time-domain + espectral (pesquisa)** — avaliar combinar WSOLA (transientes) com phase-vocoder em porções sustentadas; go/no-go após protótipo.
8. **Harness de avaliação** — sinais de referência (golden), métricas objetivas (nitidez de transiente, artefato espectral) e A/B cego contra referências comerciais para fixar os *targets* de qualidade.

**Licença (LGPL v2.1):** o fork do SoundTouch fica em repositório separado sob LGPL; como o BAQUE é open source, a compatibilidade de licenças é direta (LGPL ⊂ GPLv3). Manter o fork versionado e com as modificações documentadas.

---

## 5. Arquitetura Técnica (JUCE)

**Camadas (separação clara entre estado, DSP e UI):**

- `PluginProcessor` (`AudioProcessor`) — entrada/saída, transporte, host sync.
- **Estado:** `AudioProcessorValueTreeState` (APVTS) para parâmetros automatizáveis; `ValueTree` para o estado completo (padrões, samples, mapeamentos) serializado em XML/binário.
- **Engine de áudio (RT-safe):**
  - Pool de **voices** de sample pré-alocado (sem alocação no audio thread); voice stealing configurável.
  - **Feel/sequencer engine** que agenda eventos com precisão de sample, aplicando offsets de micro-timing antes do disparo.
  - **FX graph** por faixa + master.
- **Threading:**
  - *Audio thread* — somente DSP, sem locks, sem alocação, sem I/O.
  - *Message thread* — GUI.
  - *Background thread* — carregamento de samples, análise de transientes, render de waveform.
  - Comunicação por **filas lock-free** (`juce::AbstractFifo`) e troca de objetos imutáveis (pattern de "command queue" + double-buffering de estado de áudio).
- **GUI:** componentes JUCE + `LookAndFeel` custom; possível OpenGL para visualizadores/waveform; resizable + DPI scaling.
- **DSP:** `juce::dsp` + módulos custom; oversampling nos estágios não-lineares.

Diagrama lógico:

```
[ Host / Standalone ]
        │ (transport, MIDI, audio I/O)
        ▼
 PluginProcessor ──── APVTS (params) ──── ValueTree (estado/presets)
        │
        ├── Sequencer Engine ──► Feel Engine (micro-timing) ──► Scheduler
        │                                                          │
        │                                  ┌───────────────────────┤
        │                                  ▼                       ▼
        │                          Sample Voice Pool        MIDI Out (hardware)
        │                                  │
        │                          per-track FX ──► Master FX ──► Multi-out
        │
        └── (background) Sample Loader / Transient Analysis / Waveform Render
```

---

## 6. Modelo de Dados / Estado / Presets

- Estado em `ValueTree`; parâmetros automatizáveis também espelhados na APVTS.
- **Preset = padrões + mapeamento de pads + parâmetros de FX + referências de sample** (não o áudio por padrão).
- **Samples:** referência por caminho relativo/absoluto, com opção de **embed** no preset (decisão em aberto — §14). Tratamento de "missing sample" com relink.
- Versionamento de schema do preset (campo `version`) para migração futura.
- Export/import de **feel templates** e de **mapeamentos MIDI**.

---

## 7. UI/UX & Design System

- Documento `DESIGN.md` próprio (formato Stitch), com tokens de cor/tipografia/spacing, à semelhança dos seus outros projetos.
- **Direção estética sugerida:** dark studio + acento quente (a "poeira"), grão sutil, displays de waveform legíveis, knobs com feedback claro de p-lock.
- **Telas principais:** Pads/Performance · Sequencer (step grid + lanes) · Sample Editor (slice/start/end/pitch) · FX Rack · Mixer · Browser · MIDI/Settings.
- Foco em workflow rápido (Madlib-speed): drag → chop → toca em poucos cliques.
- Acessibilidade: contraste, tamanhos de alvo, atalhos de teclado, undo/redo.

---

## 8. Convenções de Código

- `snake_case` para identificadores; **identificadores em inglês, comentários em português**.
- Sufixo `_` para membros privados (ex.: `voice_pool_`).
- **C++17**.
- **Sem `new`/`delete` cru** — RAII, `std::unique_ptr`/`std::shared_ptr`, contêineres da STL/JUCE; pré-alocação no audio thread.
- **Catch2** para testes unitários.
- CMake como sistema de build; clang-format + clang-tidy no CI.
- DSP isolado da UI (testável offline, sem dependência de GUI).

---

## 9. Performance & Targets

- **Zero alocação/locks/IO no audio thread.**
- Latência adicional do plugin reportada corretamente ao host; opção de oversampling com latência declarada.
- Polifonia alvo: ≥ 64 voices simultâneas em hardware moderno sem dropout.
- CPU por voice e por estágio de FX medidos e orçados (budget por bloco de áudio).
- Recall de estado 100% fiel; automação suave (parameter smoothing).
- Sem clicks em start/stop/loop (crossfades curtos onde necessário).

---

## 10. Plataformas & Formatos

- **Formatos (núcleo v1):** VST3, AU, Standalone. **CLAP em fase posterior** (via `clap-juce-extensions`; sem custo de SDK). AAX/LV2 fora do escopo inicial.
- **macOS:** arm64 + x86_64 (universal binary); assinatura + **notarização**.
- **Windows:** x64.
- Linux: desejável (VST3/Standalone) — confirmar prioridade.
- *(Ambas as suas máquinas — MacBook Air M4 e desktop Ryzen/RTX — cobrem os dois alvos primários para teste.)*

---

## 11. Testes & QA

- **Unit tests (Catch2):** DSP determinístico (filtros, envelopes, bitcrush, feel engine — offsets/seeds reprodutíveis), serialização de estado, sequenciador (probabilidade/ratchets/euclidiano com seeds).
- **pluginval** (nível strict) no CI para validação de formato.
- **Render offline** de referência (golden files) para regressão de áudio.
- Teste de **RT-safety** (sem alocação no audio thread — instrumentação/asserts em debug).
- Teste de compatibilidade em DAWs: Reaper, Live, Logic, Bitwig, FL.
- Teste de **MIDI out** contra TR-8/TR-8S real (sync de clock, jitter, CC).
- CI multiplataforma (macOS + Windows) com build, testes e pluginval.

---

## 12. Roadmap / Fases / Milestones

Cada fase tem um **Definition of Done (DoD)** verificável — adequado para pipeline incremental (inclusive agentes).

| Fase | Entrega | DoD |
|---|---|---|
| **0. Setup** | CMake + JUCE, esqueleto de plugin, CI, formatos carregando vazios | VST3/AU/Standalone abrem em host; pluginval verde básico; CI rodando |
| **1. Core audio** | Voice de sample, 1 pad, transporte | Pad dispara sample sem clicks; sample-accurate; RT-safe |
| **2. Sequenciador base** | Step grid, padrões, swing | Padrão toca em loop; swing global; troca de padrão sem glitch |
| **3. Sample Engine** | Slice/chop, pitch, choke, velocity layers, reverso | Chop-to-pads; varispeed + stretch offline (v1 do fork SoundTouch); choke funcionando |
| **4. Feel Engine** ⭐ | Micro-timing, humanize, dual-grid, no-grid, feel presets | "Dilla Drunk" e "Burial Broken" perceptíveis e reprodutíveis por seed |
| **5. FX + P-locks** | Filtros, reverb, delay, sidechain, EQ + p-locks | P-lock automatiza qualquer parâmetro por step; sidechain pump funcional |
| **6. Lo-fi + Granular** | Bitcrush, SR reduction, vinyl, tape; engine granular | Presets SP-1200/SP-303; granular freeze/clouds estável |
| **7. Scatter / Perf FX** | Scatter, beat-repeat, gater, tape-stop, fills | Scatter ao vivo sem dropout; fills via trig conditions |
| **8. MIDI / Hardware** | MIDI out, clock sync, CC, MIDI learn, modo híbrido | Dirige TR-8 real em sync; lanes internas+externas no mesmo padrão |
| **9. UI/UX** | LookAndFeel, todas as telas, visualizadores, undo/redo | Workflow completo do drag ao beat; DESIGN.md aplicado |
| **10. Presets & Library** | Sistema de presets, browser, conteúdo de fábrica | Salvar/carregar fiel; biblioteca de fábrica por estética |
| **11. Hardening** | Auditoria RT-safety, otimização, cross-platform, beta | pluginval strict verde; 64 voices sem dropout; sem alocação no áudio |
| **12. Release** | Docs, instalador, assinatura/notarização, packaging | Instaladores macOS/Windows assinados; manual; v1.0 |
| **R&D-TS** *(paralela, inicia na Fase 3)* | Fork SoundTouch: harness de avaliação, protótipos de transiente/formante/character, RT-safety wrapper | Harness com métricas objetivas; protótipo de preservação de transiente supera o baseline SoundTouch; wrapper sem alocação no audio thread |

> **Trilha paralela R&D-TS:** o fork do SoundTouch corre como trilha de pesquisa contínua a partir da Fase 3. A v1 do fork (varispeed + stretch offline + RT-safety) é pré-requisito da Fase 3; os refinamentos avançados (preservação de transiente/formante, modo `character`, híbrido espectral) amadurecem entre as Fases 4 e 6 e são integrados na Fase 6.

---

## 13. Stack & Dependências

- **JUCE 8.x** sob **GPLv3** (gratuito para projeto open source).
- **CMake**, clang-format, clang-tidy.
- **Catch2** (testes).
- **pluginval** (validação).
- **Time-stretch:** **fork otimizado do SoundTouch** (LGPL v2.1) — trilha de P&D dedicada (ver §4.11). Repositório separado para o fork.
- `juce::dsp` para filtros/oversampling/FFT; FFT própria/JUCE para granular.
- **CLAP** via `clap-juce-extensions` (fase posterior).
- macOS: codesign + notarytool. Windows: code signing cert.

---

## 14. Riscos & Decisões em Aberto

1. **Nome** — ✅ **resolvido: `BAQUE`** (o baque do beat + raiz nordestina; não é instrumento nem ritmo). Aplicar em repositório, bundle IDs e nomes de produto.
2. **Time-stretch:** ✅ **resolvido** — fork otimizado do SoundTouch com trilha de P&D dedicada (§4.11). Projeto open source; compatibilidade de licença direta (LGPL ⊂ GPLv3).
3. **CLAP** — ✅ **resolvido: fase posterior** (pós-v1, via `clap-juce-extensions`). Núcleo v1 = VST3 + AU + Standalone.
4. **AAX** — exige SDK Avid e assinatura; provavelmente **fora do v1**.
5. **Embed de samples no preset** — conveniência vs. tamanho de arquivo. Sugestão: opcional, off por padrão, com "collect & save".
6. **Profundidade do Song Mode no v1** — completo vs. apenas pattern chaining (entregar chaining no v1, song mode completo depois).
7. **Multi-out no v1** — stems por faixa aumentam complexidade; pode ser v1 ou v1.1.
8. **Licença** — ✅ **resolvido: open source, JUCE sob GPLv3.** Definir a licença do próprio BAQUE (GPLv3 recomendado, por compatibilidade com JUCE/SoundTouch) e adicionar `LICENSE` + `NOTICE` (atribuições de terceiros) ao repositório.
9. **Linux** — confirmar se é alvo de v1.

---

## 15. Conteúdo de Fábrica (Factory Library)

- **Feel templates** por estética (Dilla, Burial, FlyLo, Boom-Bap, Bonobo).
- **Kits** de bateria (incl. "found sound" para Burial; secos boom-bap para Alchemist; SP-crunch para Madlib).
- **Texturas** granulares (mallets/harpa/ambiências para Teebs).
- **Presets de scatter** e **mapeamentos MIDI** prontos (TR-8 / TR-8S).
- Demo patterns de cada estética.

---

## 16. Próximos Passos

1. Decisões de §14 travadas (nome, time-stretch, CLAP, licença). Restam abertas: embed de samples (#5), profundidade do song mode (#6), multi-out v1 (#7), Linux (#9).
2. Gerar `DESIGN.md` (Stitch) da UI.
3. Criar esqueleto de projeto (Fase 0): CMake + JUCE + CI + pluginval.
4. Implementar incrementalmente seguindo o roadmap, com DoD por fase como critério de merge.
