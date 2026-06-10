#pragma once

#include "lane_routing.h"
#include "midi_cc.h"
#include "step_pattern.h"
#include "tr8_midi_spec.h"

#include <array>
#include <cstddef>
#include <cstdint>

// Templates de mapeamento de hardware (Fase 9-04, ESCOPO §4.8 "Templates prontos para TR-8/TR-8S").
// Um template descreve: nota MIDI por lane + canal EXT + slots de CC out. Aplicar um template
// configura lane_routing_ + cc_out_ + as notas do StepPattern numa única chamada.
//
// CONTRATO SINGLE-WRITER (como LaneRouting/CcOutRouting): aplicado da message thread em setup;
// sem writer concorrente em v1. Fase 10 (UI) migra para atomics/command queue se necessário.
//
// HONESTIDADE DE CC (audit 09-04, atualizado 09-04 Tier-3): o note map e a layout publicada da
// Roland (MIDI Implementation Chart TR-8 v1.11) e e enviado preenchido — VERIFICADO contra a spec
// em test_tr8_spec_conformance.cpp. Os numeros de CC vem da MESMA chart (tr8_spec), nao mais
// chutados: os CCs CONTINUOS (scatter_depth/reverb/delay) ficam LIGADOS (mapeamento de valor
// linear e seguro). scatter_type/scatter_sw (discretos) ficam OFF ate confirmar o mapeamento de
// VALOR em hardware — conhecemos o numero do CC, nao a curva de selecao de padrao do TR-8.

// Mapa de nota por lane. mapped=false → lane intocada pelo template (mantém routing/notas atuais).
struct LaneNoteMap {
    bool mapped = false;
    uint8_t note = 36;
    LaneMode mode = LaneMode::external;
};

struct HardwareTemplate {
    const char* name = ""; // rótulo/id para UI ("TR-8", "TR-8S")
    uint8_t channel = 10;  // canal EXT default (TR-8 GM = canal 10)
    std::array<LaneNoteMap, 16> lanes{};

    // Slots de CC out indexados por PLockParam. cc_enabled default false; preencher só CC confirmado.
    std::array<bool, k_plock_param_count> cc_enabled{};
    std::array<uint8_t, k_plock_param_count> cc_number{};
    bool cc_out_enabled = false; // master enable do cc_out_ ao aplicar
};

// Template TR-8: 11 vozes na layout GM da Roland, canal 10.
inline HardwareTemplate tr8_template() noexcept {
    HardwareTemplate t;
    t.name = "TR-8";
    t.channel = 10;

    // Roland TR-8 GM drum map (canal 10):
    t.lanes[0] = {true, 36, LaneMode::external};  // BD  - Bass Drum
    t.lanes[1] = {true, 38, LaneMode::external};  // SD  - Snare Drum
    t.lanes[2] = {true, 43, LaneMode::external};  // LT  - Low Tom
    t.lanes[3] = {true, 47, LaneMode::external};  // MT  - Mid Tom
    t.lanes[4] = {true, 50, LaneMode::external};  // HT  - Hi Tom
    t.lanes[5] = {true, 37, LaneMode::external};  // RS  - Rim Shot
    t.lanes[6] = {true, 39, LaneMode::external};  // HC  - Hand Clap
    t.lanes[7] = {true, 42, LaneMode::external};  // CH  - Closed Hi-Hat
    t.lanes[8] = {true, 46, LaneMode::external};  // OH  - Open Hi-Hat
    t.lanes[9] = {true, 49, LaneMode::external};  // CC  - Crash Cymbal
    t.lanes[10] = {true, 51, LaneMode::external}; // RC  - Ride Cymbal
    // lanes 11..15: mapped=false (intocadas — disponíveis para lanes internas/sample).

    // CC slots: numeros AUTORITATIVOS da MIDI Implementation Chart da Roland (TR-8 v1.11,
    // tr8_midi_spec.h) — nao mais chutados. Resolve o deferral do audit 09-04 para os CC
    // CONTINUOS (knob 0-127 linear), cujo mapeamento de valor e seguro:
    //   scatter_depth -> CC 69 | reverb_mix -> CC 91 (REVERB LEVEL)
    //   delay_mix     -> CC 16 (DELAY LEVEL) | delay_time -> CC 17 (DELAY TIME)
    t.cc_enabled[static_cast<size_t>(PLockParam::scatter_depth)] = true;
    t.cc_number[static_cast<size_t>(PLockParam::scatter_depth)] = tr8_spec::k_cc_scatter_depth; // 69
    t.cc_enabled[static_cast<size_t>(PLockParam::reverb_mix)] = true;
    t.cc_number[static_cast<size_t>(PLockParam::reverb_mix)] = tr8_spec::k_cc_reverb_level; // 91
    t.cc_enabled[static_cast<size_t>(PLockParam::delay_mix)] = true;
    t.cc_number[static_cast<size_t>(PLockParam::delay_mix)] = tr8_spec::k_cc_delay_level; // 16
    t.cc_enabled[static_cast<size_t>(PLockParam::delay_time)] = true;
    t.cc_number[static_cast<size_t>(PLockParam::delay_time)] = tr8_spec::k_cc_delay_time; // 17
    t.cc_out_enabled = true;

    // scatter_type (CC 68 SCATTER TYPE) fica OFF de proposito: e DISCRETO (seleciona padrao de
    // scatter via 0-127). Nossa lei linear 0..10 -> 0..127 pode cair no padrao errado no TR-8 →
    // precisa de confirmacao do mapeamento de VALOR (nao do numero do CC) em hardware. SCATTER SW
    // (CC 70) idem (on/off). Numeros conhecidos (tr8_spec) mas slots desligados ate value-map.
    return t;
}

// Template TR-8S: mesma layout GM de 11 vozes do TR-8 (default de fábrica), nome distinto p/ UI.
// Slots de CC específicos do TR-8S são aditivos e ficam OFF até confirmação de hardware (AC-3).
inline HardwareTemplate tr8s_template() noexcept {
    HardwareTemplate t = tr8_template();
    t.name = "TR-8S";
    return t;
}

// Aplica o template a routing + cc + pattern numa chamada. Single-writer (message thread).
//
// NÃO-DESTRUTIVO (audit 09-04 SR1): rewrita SOMENTE a NOTA de cada step das lanes mapeadas.
// is_active / get_trig / p-locks de TODOS os steps são preservados — um template remapeia a nota
// que a lane manda ao hardware, NÃO apaga o beat programado pelo usuário.
// routing + cc_out SÃO reset completo para o template (ação intencional de setup).
inline void
apply_template(const HardwareTemplate& t, LaneRouting& routing, CcOutRouting& cc, StepPattern& pattern) noexcept {
    for (int lane = 0; lane < 16; ++lane) {
        const auto& lm = t.lanes[static_cast<size_t>(lane)];
        if (!lm.mapped) {
            continue; // lane intocada (AC-2): routing/notas/ativações preservados
        }
        routing.mode[static_cast<size_t>(lane)] = lm.mode;
        routing.channel[static_cast<size_t>(lane)] = t.channel;
        for (int s = 0; s < StepPattern::k_num_steps; ++s) {
            pattern.set_note(lane, s, lm.note); // SR1: SOMENTE set_note; nunca set_active/set_trig/clear
        }
    }

    // Reset completo de cc_out_ para o template (intencional — documentado, sem surpresa silenciosa).
    cc.enabled = t.cc_out_enabled;
    cc.channel = t.channel;
    cc.cc_enabled = t.cc_enabled;
    cc.cc_number = t.cc_number;
}
