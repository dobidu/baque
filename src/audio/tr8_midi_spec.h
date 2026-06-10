#pragma once

#include <array>
#include <cstdint>

// Roland TR-8 MIDI Implementation Chart — fonte autoritativa (NAO derivada do nosso template).
// Source: https://cdn.roland.com/assets/media/pdf/TR-8_MIDI_Imple_Chart_e02_W.pdf
//   "MIDI Implementation Chart — RHYTHM PERFORMER — Model: TR-8 — Date: Nov. 18, 2014 — Version: 1.11"
//
// Este header existe para o teste de conformidade (test_tr8_spec_conformance.cpp) cruzar o
// hardware_templates.h contra os numeros PUBLICADOS pela Roland. O template e escrito a mao
// (independente); o teste prova que bate com a spec. NAO faca o template ler deste header —
// isso tornaria o teste tautologico. Duas fontes, uma assercao de igualdade.
//
// HONESTIDADE DE CC (resolve o deferral do audit 09-04): os numeros de CC do TR-8 nao sao mais
// "chutados / confirmar em hardware". Esta e a propria spec de firmware da Roland (v1.11). O
// risco residual NAO e o numero do CC e sim o mapeamento de VALOR de params discretos
// (ex.: SCATTER TYPE seleciona padrao via 0-127 — nossa lei linear pode cair no padrao errado).
// Por isso o template liga so os CCs continuos (knob 0-127 linear) e deixa scatter_type OFF.

namespace tr8_spec {

// --- Canal default (Basic Channel: Default 10) -----------------------------------------------
inline constexpr uint8_t k_default_channel = 10;

// --- Note Number por instrumento -------------------------------------------------------------
// A chart lista pares com acento (ex.: BD 35,36); a coluna "7X7-TR8 instalado" da o numero
// primario unico. Usamos o primario — o trigger canonico de cada instrumento.
//   BASS DRUM 35,36 -> 36 | SNARE 38,40 -> 38 | LOW TOM 41,43 -> 43 | MID TOM 45,47 -> 47
//   HI TOM 48,50 -> 50 | RIM SHOT 37 | HAND CLAP 39 | CLOSED HH 42,44 -> 42 | OPEN HH 46
//   CRASH 49 | RIDE 51
inline constexpr uint8_t k_note_bd = 36; // BASS DRUM
inline constexpr uint8_t k_note_sd = 38; // SNARE DRUM
inline constexpr uint8_t k_note_lt = 43; // LOW TOM
inline constexpr uint8_t k_note_mt = 47; // MID TOM
inline constexpr uint8_t k_note_ht = 50; // HIGH TOM
inline constexpr uint8_t k_note_rs = 37; // RIM SHOT
inline constexpr uint8_t k_note_hc = 39; // HAND CLAP
inline constexpr uint8_t k_note_ch = 42; // CLOSED HI-HAT
inline constexpr uint8_t k_note_oh = 46; // OPEN HI-HAT
inline constexpr uint8_t k_note_cc = 49; // CRASH CYMBAL
inline constexpr uint8_t k_note_rc = 51; // RIDE CYMBAL

// Notas na ordem das lanes 0..10 do template (BD SD LT MT HT RS HC CH OH CC RC).
inline constexpr std::array<uint8_t, 11> k_lane_notes = {k_note_bd,
                                                         k_note_sd,
                                                         k_note_lt,
                                                         k_note_mt,
                                                         k_note_ht,
                                                         k_note_rs,
                                                         k_note_hc,
                                                         k_note_ch,
                                                         k_note_oh,
                                                         k_note_cc,
                                                         k_note_rc};

// --- Control Change (coluna Recognized = o, i.e. o TR-8 RECEBE estes CCs) --------------------
// Subconjunto relevante para o BAQUE. Lista completa da chart inclui tune/decay/level por inst.
inline constexpr uint8_t k_cc_shuffle = 9;
inline constexpr uint8_t k_cc_delay_level = 16;
inline constexpr uint8_t k_cc_delay_time = 17;
inline constexpr uint8_t k_cc_delay_feedback = 18;
inline constexpr uint8_t k_cc_scatter_type = 68;  // discreto: seleciona padrao de scatter
inline constexpr uint8_t k_cc_scatter_depth = 69; // continuo
inline constexpr uint8_t k_cc_scatter_sw = 70;    // scatter on/off
inline constexpr uint8_t k_cc_accent = 71;
inline constexpr uint8_t k_cc_reverb_time = 89;
inline constexpr uint8_t k_cc_reverb_gate = 90;
inline constexpr uint8_t k_cc_reverb_level = 91;

} // namespace tr8_spec
