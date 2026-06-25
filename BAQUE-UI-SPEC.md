# BAQUE — Especificação de Interface (UI/UX)
### Beat Machine & Groovebox · documento de referência para implementação JUCE

> Companion do protótipo HTML (`BAQUE.html`). Este documento descreve **o que cada tela faz, como os componentes se organizam e os fluxos de trabalho esperados** — para servir de guia tanto ao design quanto à implementação em C++/JUCE.
>
> **Status:** v1.0 · acompanha o protótipo interativo de 6 telas

---

## 1. Visão geral

BAQUE é um groovebox híbrido (instrumento de palco + ferramenta de estúdio) cujo diferencial é o **Feel Engine** — o micro-timing tratado como protagonista visual, não como um menu escondido. A interface foi desenhada para o produtor que admira a lógica de hardware (MPC, Digitakt, TR-808/909, SP-404) mas quer **ver e sentir o "baque"** do groove.

### 1.1 Princípios de design

| Princípio | Como se manifesta |
|---|---|
| **Tempo é alma** | O Feel Field radial mostra cada nota empurrada para fora da grade perfeita. O groove é uma forma, não um número. |
| **Familiar, com 1 momento radical** | 90% reconhecível (pads 4×4, step grid, knobs). O Feel Field é o salto. |
| **Gesto sobre precisão (no palco)** | A tela PERF FX usa alvos grandes e controles momentâneos (segura-aplica-solta). |
| **Feedback imediato** | Tudo reage ao beat: pads pulsam na velocity, playhead varre, medidores dançam. |
| **Tempero nordestino sem caricatura** | Calor de barro/terra na paleta; instrumentos reais (zabumba, caixa, ganzá, agogô, tamborim, triângulo) ao lado do kit universal. |

### 1.2 Identidade visual (design system)

- **Temas (4):** `Barro` (dark quente padrão), `Cinza Estúdio` (dark neutro), `Maracatu` (dark saturado), `Papel` (claro).
- **Acentos por papel/role:** graves = **ember** (`#e8502e`), caixas = **ocre** (`#e6b34d`), perc/vox = **clay** (`#d98a4b`), chimbais = **osso/bone** (`#f2e9dd`). Automação (p-lock) usa **índigo** (`#5b86a8`).
- **Tipografia:** UI em *Space Grotesk* (alternativas: Archivo, Bricolage); dados/labels em *JetBrains Mono*.
- **Textura:** overlay de grão ajustável (soft-light) + vinheta quente. Reforça o caráter "lo-fi/tape".
- **Controles:** knobs skeuomórficos com indicador + **arco de valor** colorido; faders com cap metálico; medidores segmentados.

Todos esses parâmetros são expostos no painel **Tweaks** (tema, acento, grão, fonte, densidade, grooves do disco).

---

## 2. Estrutura global

```
┌─────────────────────────────────────────────────────────────┐
│  HEADER (sempre visível)                                       │
│  BAQUE | NAV | ▶ ■ ● | BPM/TAP | PADRÃO | FEEL | AGE | MASTER  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ÁREA DE TELA (troca conforme a NAV)                          │
│  PERFORM · FX · MIX · PERF FX · BROWSER · MIDI                │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### 2.1 Header / Transport (persistente em todas as telas)

Da esquerda para a direita:

| Componente | Função | Interação |
|---|---|---|
| **Wordmark BAQUE** + tagline | Identidade. O "B" em ember. | — |
| **NAV** | Troca de tela. 6 botões: PERFORM, FX, MIX, PERF FX, BROWSER, MIDI. | Clique. Botão ativo recebe contorno em acento. |
| **Transport** | Play / Stop / Record. | Clique. **Barra de espaço** = play/stop. Play aceso = acento; Rec aceso = pisca em ember. |
| **BPM** | Andamento (40–200). | Arraste vertical para alterar; mostra 1 casa decimal. |
| **TAP** | Tap tempo. | Toque ritmado ≥ 2× → calcula BPM da média. |
| **PADRÃO** | Slots de pattern (A1, A2, A3, B1…). | Clique seleciona; base para o Song mode futuro. |
| **FEEL** | Seletor de preset de groove. | Dropdown: Straight, DL Drunk, BRL Broken, Fly Wonk, Boom-Bap, BNB Loose. Reconfigura o Feel Engine inteiro. |
| **AGE** | Knob global de envelhecimento (lo-fi: saturação + corte de agudos). | Arraste. 0–100. |
| **MASTER** | Medidores estéreo. | Reagem ao nível de saída em tempo real. |

---

## 3. Tela PERFORM (tela-herói)

A tela de "fazer beat". É onde se passa a maior parte do tempo. Layout em **3 colunas + sequencer full-width embaixo**:

```
┌──────────┬─────────────────────┬──────────────┐
│  PADS    │   O BAQUE           │  SAMPLE       │
│  4×4     │   (Feel Field)      │  (editor)     │
│          │                     │               │
├──────────┴─────────────────────┴──────────────┤
│  SEQUENCER (16 steps × lanes)                  │
└────────────────────────────────────────────────┘
```

### 3.1 PADS (coluna esquerda)

- **Grade 4×4** com 16 pads; abas de banco **A/B/C/D** no topo (16 sons cada → 64 no total).
- Cada pad mostra: **dot colorido** (papel), **mini-waveform** do sample, **nome** (KICK, ZABUMBA, SNARE, CAIXA, CLAP, RIM, HAT CL, HAT OP, GANZÁ, TAMBORIM, AGOGÔ, TRIÂNGULO, PERC, VOX CHOP, SUB 808, KICK B…).
- **Layout por linha:** graves (linha 1), caixas (linha 2), chimbais/metais (linha 3), perc/vox (linha 4).

**Interações:**
- **Clique** → toca o som *e* foca o pad (sincroniza Sample Editor + Sequencer + FX para essa faixa).
- Durante o play, o pad **pulsa (glow)** na intensidade da velocity do trig.
- Pad focado recebe contorno em acento.

### 3.2 O BAQUE — Feel Field (coluna central, peça-assinatura)

Um **disco radial tipo vinil** que materializa o groove:

- **Anel = um compasso.** 16 ticks marcam a grade perfeita; tempos (1·2·3·4) acentuados e numerados.
- **Nós = trigs.** Cada nota é um ponto luminoso. Seu **raio depende do papel** (graves no anel externo → chimbais no interno = 4 faixas concêntricas, o "dual-grid"). O **tamanho do nó = velocity**.
- **Desvio da grade = micro-timing.** Swing e push por papel **empurram o nó para frente/trás** do tick. É possível *ver* o desvio "drunk" como uma ondulação. Quando uma faixa está focada, uma **linha fantasma tracejada** liga o nó à sua posição "perfeita", evidenciando o desvio.
- **Braço/playhead** varre o disco como agulha de toca-discos durante o play; ao passar por um nó, ele **dá um flare**.
- **Humanize** vira um leve *shimmer* (tremor) nos nós.
- **Miolo (readout):** nome do feel + SWING % + DRIFT.
- **Legenda** embaixo: graves · caixas · perc · chimbais (cores).

**Função:** ler o groove de relance e entender *como* o feel está sendo aplicado. (No protótipo é visualização; a edição direta de nós arrastando é um próximo passo opcional.)

### 3.3 SAMPLE (coluna direita)

Editor do **pad em foco**:

- **Título** com o nome do pad; abas de modo **ONE-SHOT / GATE / LOOP**.
- **Waveform grande** com **slice markers** (1–4) tracejados e tags (WAVESPEED, nº de pontos).
- **Banco de 6 knobs:** PITCH (−24…+24 st), FILTER, DECAY, DRIVE, DRIFT, PAN.
  - Knobs com automação ativa (**p-lock**) mostram **ponto índigo** + arco índigo.

**Interações de knob (todos os knobs do app):** arraste vertical; **Shift** = ajuste fino; **duplo-clique** = volta ao centro/default.

### 3.4 SEQUENCER (faixa inferior, full-width)

Step grid clássico, com dois modos via toggle **TODAS / FOCO**:

**Modo TODAS** (default, estilo Digitakt/Drum Rack):
- Cada lane = uma faixa (dot + nome), 16 células à direita.
- Tempos (1,5,9,13) com fundo mais claro.
- Célula ligada: opacidade/cor proporcional à **velocity**; marcadores de **ratchet** (barras), **p-lock** (ponto índigo) e **probabilidade** (número).
- A coluna do step atual acende em ember conforme o playhead avança; a célula "estoura" (scale + brilho) quando dispara.

**Modo FOCO** (compacto, para esta vista):
- Mostra só a lane focada, **grande**, com **barras de velocity** verticais por step, ratchet (×N), p-lock e probabilidade. Libera espaço e dá precisão.

**Interações:** clique liga/desliga step. (Edição de velocity/ratchet/condições por step é simulada — implementação real fica no engine.)

### 3.5 Workflow — montar um beat

1. **Play** (espaço) — o pattern default já toca (groove off-grid).
2. Clique num **pad** para ouvir/escolher o som; ele entra em foco.
3. **Sequencer:** clique nas células para programar a faixa focada.
4. **Sample:** ajuste pitch/filter/decay/drive/pan da faixa.
5. **FEEL** (header): troque o preset e **veja o disco se reorganizar** — escolha o groove no olho e no ouvido.
6. **AGE** para sujar a textura geral.
7. Repita pad a pad. Use **FOCO** quando quiser detalhar uma lane.

---

## 4. Tela FX (FX Rack)

Cadeia de efeitos **em série**, com **escopo FAIXA / MASTER** (segmented control no topo). Em escopo FAIXA, opera sobre a faixa em foco (tag colorida mostra qual).

Layout: **grade de módulos** (4 colunas), cada um um card com power, modelo, **visualizador ao vivo** e knobs.

| Módulo | Modelos | Knobs | Visualizador |
|---|---|---|---|
| **FILTER** | Ladder 24 · Diode · Formant · Grimy 12 | CUTOFF, RESO, ENV (p-lock) + tipo LP/BP/HP | Curva de resposta desenhada ao vivo |
| **DRIVE** | Tape · Tube · Wavefold · Bitcrush | DRIVE, TONE, MIX | Curva de transferência (tanh) |
| **LO-FI · AGE** | SP-1200 · SP-303 · 8-bit · Vinyl · Tape | BITS, RATE, VINYL, AGE | Grade de "poeira" (densidade de bits) |
| **GRANULAR** | — | SIZE, DENS, SPRAY + FREEZE | Nuvem de grãos espalhada |
| **REVERB** | Hall · Plate · Spring · Cavern | SIZE, DECAY, DUCK, MIX | Cauda de decaimento |
| **DELAY** | Tape · Ping-pong · Granular · Dub | TIME (sincronizado), FBK, TONE, MIX | Taps decrescentes |
| **SIDECHAIN** | fonte selecionável (qualquer faixa) | AMT, ATK, REL | Curva de "pump" |
| **EQ** | — | LOW, MID, FREQ, HIGH | Curva de EQ ao vivo |

**Interações:** power liga/desliga (card esmaece quando off); knobs giram e **as curvas redesenham em tempo real**; dropdowns trocam o modelo/fonte.

**Workflow:** foque uma faixa na tela PERFORM → vá em FX → esculpa filtro/drive/lo-fi → adicione reverb/delay → use sidechain (ex: ducking pelo KICK) → alterne para MASTER para a cola final.

---

## 5. Tela MIX (Mixer)

Console com **16 channel strips + master**. Cada strip:

```
NOME ▸ SENDS(A,B) ▸ PAN ▸ [FADER | METER] ▸ M/S ▸ OUT
```

| Componente | Função |
|---|---|
| **Nome** | Faixa (dot colorido). |
| **Sends A/B** | Dois knobs de envio para buses de FX. |
| **PAN** | Knob L/C/R. |
| **Fader** | Volume — **arrastável** (cap metálico). |
| **Meter** | Reage ao beat (pulsa na velocity do trig daquela faixa). |
| **M / S** | Mute (ember) / Solo (ocre). |
| **OUT** | Roteamento multi-out (1-2…), para exportar stems ao DAW. |

A strip **MASTER** (mais larga) tem o fader principal, medidores duplos e tags de bus (AGE, GLUE).

**Workflow:** equilibre os níveis com os faders, posicione no estéreo com PAN, mande reverb/delay via sends, isole faixas com Solo para checar, e use os multi-outs para entregar stems separados ao DAW.

---

## 6. Tela PERF FX (Scatter / Performance ao vivo)

A tela "instrumento de palco" — **destruir e remontar o beat em tempo real** com gestos momentâneos. Tudo é **segura-aplica-solta** e marcado como **mapeável a MIDI** (⌁). Layout em 3 colunas:

```
┌───────────┬──────────────────┬───────────────┐
│  SCATTER  │  PERFORMANCE FX  │  XY / GROUPS / │
│  (dial)   │  (9 pads)        │  MORPH         │
└───────────┴──────────────────┴───────────────┘
```

### 6.1 SCATTER (coluna esquerda)
- **Anel de tipos 1–10** ao redor de um **disco central momentâneo**. Cada tipo é uma receita de glitch diferente (repeat, reverse, gate, decimate, scatter, combinações).
- Knob **DEPTH** controla a intensidade.
- **Segura o disco** → o beat se reorganiza; **solta** → volta ao normal.

### 6.2 PERFORMANCE FX (coluna central)
Grade 3×3 de pads grandes, todos momentâneos:

| Pad | Efeito |
|---|---|
| **STUTTER 1/8 · 1/16 · 1/32** | Congela o step atual e re-dispara na subdivisão. |
| **GATE 1/8 · 1/16** | Corta o master ritmicamente (square LFO). |
| **TAPE STOP** | Freia o andamento + filtro + poeira (efeito "desligar fita"). |
| **RISER** | Sweep de ruído subindo (transição/build). |
| **CRUSH** | Decimate/bitcrush momentâneo. |
| **FILL** | Roll rápido de caixa/snare enquanto segurado. |

### 6.3 XY · GROUPS · MORPH (coluna direita)
- **XY Pad** grande: arraste para varrer **cutoff × ressonância** no master ao vivo; solta = reabre o filtro. Mostra cursor + crosshair.
- **Mute/Solo Groups:** KICK/SUB · SNARE/CLAP · CHIMBAIS · PERC/VOX — silencia/isola grupos num toque (M ember / S ocre).
- **Scene Morph:** crossfader **A↔B** que cruza, ao vivo, entre o pattern e uma variação gerada (probabilístico por step).

**Comportamento:** o beat segue rodando por baixo; o estado de performance **se limpa sozinho** ao trocar de tela.

**Workflow (palco):** com o beat tocando, use Mute groups para construir/derrubar energia, FILL antes da virada, TAPE STOP para a quebra, RISER + Scatter na subida, XY para o varrido de DJ, e Scene Morph para transicionar entre seções — tudo sem olhar muito para a tela.

---

## 7. Tela BROWSER (samples & presets)

Layout em **3 painéis**: categorias · lista · preview.

```
┌────────────┬──────────────────────┬────────────┐
│ CATEGORIAS │  LISTA DE SAMPLES    │  PREVIEW    │
│ ESTÉTICA   │  (waveform/key/tag)  │  (waveform) │
└────────────┴──────────────────────┴────────────┘
```

- **Busca** + toggle **AUTO-AUDITION** no header.
- **Coluna esquerda:** categorias (Tudo, Kits, One-shots, Loops, Found Sound, Texturas, Vox/Chops) com contagem; filtros por **estética** (DL, BRL, Wonk, Boom-Bap, BNB, Granular, Lo-fi).
- **Lista:** cada linha mostra **favorito (★)**, mini-waveform, nome, key, length e **tags**.
- **Preview (direita):** nome, waveform grande, metadados (tipo/key/len), tags, botão **PREVIEW** e dica de **arraste para um pad para carregar**.

**Interações:** clicar numa linha seleciona e (se AUTO-AUDITION ligado) **toca**; estrela favorita; filtros combinam categoria + estética + busca.

**Workflow (Madlib-speed):** filtre por estética/categoria → percorra a lista ouvindo via auto-audition → favorite os candidatos → arraste o escolhido para um pad → corte/ajuste no Sample Editor.

---

## 8. Tela MIDI / SETTINGS

Configuração de I/O e modo híbrido (BAQUE como cérebro de hardware externo). Layout: **coluna de settings + tabela de mapeamento**.

### 8.1 Coluna esquerda (cards)
- **CLOCK & SYNC:** Master/Slave; enviar MIDI Clock; Start/Stop/Continue; MTC; indicador de clock pulsando.
- **DISPOSITIVO:** template de mapeamento (TR-8S, TR-8, TR-6S, Digitakt, SP-404 MK2, Genérico) + "Aplicar template".
- **ÁUDIO:** Oversampling (Off/2×/4×), sample rate, multi-out (stems), latência reportada.

### 8.2 Tabela de mapa de notas (16 faixas)
Colunas: **FAIXA · NOTA · CH · CC · MODO · LEARN**.

- **NOTA** arrastável (note name + nº MIDI).
- **MODO** por faixa: **INT** (som interno) · **EXT** (envia MIDI out) · **BOTH** (híbrido — toca interno *e* dispara hardware no mesmo padrão). Cada modo tem cor (clay/índigo/ocre).
- **LEARN** entra em modo escuta para capturar nota/CC de um controlador.

**Workflow:** escolha o template do seu hardware → ajuste notas/canais por faixa → defina quais faixas são internas, externas ou híbridas → configure clock (BAQUE como master enviando para o TR-8, por ex.) → use LEARN para mapear um controlador.

---

## 9. Tabela-resumo de telas

| Tela | Propósito | Componentes-chave | Estado |
|---|---|---|---|
| **PERFORM** | Fazer o beat | Pads 4×4, Feel Field, Sample Editor, Sequencer | ✅ |
| **FX** | Esculpir o som | 8 módulos em série, curvas ao vivo | ✅ |
| **MIX** | Equilibrar/rotear | 16 strips + master, meters reativos | ✅ |
| **PERF FX** | Performar ao vivo | Scatter dial, 9 pads, XY, groups, morph | ✅ |
| **BROWSER** | Achar sons | Categorias, lista, preview, auto-audition | ✅ |
| **MIDI** | I/O & sync | Clock, templates, mapa de notas híbrido | ✅ |

---

## 10. Convenções de interação (resumo)

| Gesto | Resultado |
|---|---|
| **Espaço** | Play / Stop |
| **Arraste vertical em knob** | Altera valor (Shift = fino) |
| **Duplo-clique em knob** | Reset ao default |
| **Arraste vertical em BPM** | Altera andamento |
| **Clique em pad** | Toca som + foca a faixa |
| **Clique em célula do step** | Liga/desliga trig |
| **Segurar pad de PERF FX** | Aplica efeito enquanto pressionado |
| **Arraste no XY** | Varre dois parâmetros; solta = reseta |
| **Ponto índigo / arco índigo** | Indica automação (p-lock) |

---

## 11. Próximos passos (fora do protótipo atual)

1. **Song mode / pattern chaining** — encadear A1→A2→B1 com repetições e arranjo (único item do escopo ainda não visualizado).
2. **Edição direta no Feel Field** — arrastar nós para empurrar o timing manualmente.
3. **P-lock recording real** — segurar step + mexer knob grava automação (lógica no engine C++/JUCE).
4. **Trig conditions** editáveis (1:4, fill, neighbor, etc.) por step.

---

*Documento gerado como referência de UI para a implementação JUCE. O protótipo HTML (`BAQUE.html`) é a fonte de verdade visual e de interação; este markdown descreve a intenção por trás de cada componente.*
