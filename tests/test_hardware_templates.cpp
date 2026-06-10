#include "audio/hardware_templates.h"
#include "audio/lane_routing.h"
#include "audio/midi_cc.h"
#include "audio/step_pattern.h"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

// Templates de mapeamento TR-8/TR-8S (Fase 9-04). Cobre AC-1..AC-3 + AC-2b (nao-destrutivo).

// AC-1: TR-8 carrega o note/channel map correto (Roland GM, canal 10).
TEST_CASE("HT1 - TR-8 template carries the Roland GM note map on channel 10", "[hardware]") {
    const auto t = tr8_template();
    REQUIRE(std::strcmp(t.name, "TR-8") == 0);
    REQUIRE(t.channel == 10);

    // lanes 0..10 mapeadas, external, notas GM.
    const uint8_t expected[11] = {36, 38, 43, 47, 50, 37, 39, 42, 46, 49, 51};
    for (int lane = 0; lane <= 10; ++lane) {
        REQUIRE(t.lanes[static_cast<size_t>(lane)].mapped);
        REQUIRE(t.lanes[static_cast<size_t>(lane)].mode == LaneMode::external);
        REQUIRE(t.lanes[static_cast<size_t>(lane)].note == expected[lane]);
    }
    // lanes 11..15 nao mapeadas.
    for (int lane = 11; lane < 16; ++lane)
        REQUIRE_FALSE(t.lanes[static_cast<size_t>(lane)].mapped);

    // CC continuos LIGADOS com numeros da chart da Roland (Tier-3); discretos/sem-equivalente OFF.
    // Conformidade exaustiva dos numeros vive em test_tr8_spec_conformance.cpp (TS2..TS4); aqui so
    // o invariante de que o master esta on e scatter_type (discreto) permanece off.
    REQUIRE(t.cc_out_enabled);
    REQUIRE_FALSE(t.cc_enabled[static_cast<size_t>(PLockParam::scatter_type)]);
}

// AC-2: apply_template configura routing + notas do pattern + cc_out numa chamada; lanes 11..15 intocadas.
TEST_CASE("HT2 - apply_template configures routing, pattern notes and cc_out", "[hardware]") {
    LaneRouting routing{}; // tudo internal por default
    CcOutRouting cc{};
    StepPattern pattern{};

    apply_template(tr8_template(), routing, cc, pattern);

    const uint8_t expected[11] = {36, 38, 43, 47, 50, 37, 39, 42, 46, 49, 51};
    for (int lane = 0; lane <= 10; ++lane) {
        REQUIRE(routing.mode[static_cast<size_t>(lane)] == LaneMode::external);
        REQUIRE(routing.channel_of(lane) == 10);
        for (int s = 0; s < StepPattern::k_num_steps; ++s)
            REQUIRE(pattern.get_note(lane, s) == expected[lane]);
    }
    // lanes 11..15 mantem internal (intocadas).
    for (int lane = 11; lane < 16; ++lane)
        REQUIRE(routing.mode[static_cast<size_t>(lane)] == LaneMode::internal);

    // cc_out resetado para o template (channel 10, CCs continuos da chart ligados — ver TS5).
    REQUIRE(cc.channel == 10);
    REQUIRE(cc.enabled);
}

// AC-2b (SR1): apply_template e NAO-DESTRUTIVO — preserva ativacoes e trigs; so reescreve notas.
TEST_CASE("HT3 - apply_template preserves step activations and trigs", "[hardware]") {
    LaneRouting routing{};
    CcOutRouting cc{};
    StepPattern pattern{};

    // Programa lane 1 (mapeada): step 2 ativo com trig fill e nota arbitraria.
    pattern.set_active(1, 2, true);
    pattern.set_trig(1, 2, StepPattern::TrigCondition::fill);
    pattern.set_note(1, 2, 99);
    // Programa lane 13 (NAO mapeada): step 5 ativo, nota 60.
    pattern.set_active(13, 5, true);
    pattern.set_note(13, 5, 60);

    apply_template(tr8_template(), routing, cc, pattern);

    // Lane mapeada: ativacao + trig PRESERVADOS; so a nota foi reescrita (SD = 38).
    REQUIRE(pattern.is_active(1, 2));
    REQUIRE(pattern.get_trig(1, 2) == StepPattern::TrigCondition::fill);
    REQUIRE(pattern.get_note(1, 2) == 38);

    // Lane nao mapeada: nota E ativacao totalmente preservadas.
    REQUIRE(pattern.is_active(13, 5));
    REQUIRE(pattern.get_note(13, 5) == 60);
    REQUIRE(routing.mode[13] == LaneMode::internal);
}

// AC-3: TR-8S reusa o note map do TR-8 e tem nome distinto.
TEST_CASE("HT4 - TR-8S template shares the TR-8 note map with a distinct name", "[hardware]") {
    const auto t8 = tr8_template();
    const auto t8s = tr8s_template();

    REQUIRE(std::strcmp(t8s.name, "TR-8S") == 0);
    REQUIRE(std::strcmp(t8s.name, t8.name) != 0); // nome distinto p/ UI
    REQUIRE(t8s.channel == t8.channel);

    for (int lane = 0; lane < 16; ++lane) {
        REQUIRE(t8s.lanes[static_cast<size_t>(lane)].mapped == t8.lanes[static_cast<size_t>(lane)].mapped);
        REQUIRE(t8s.lanes[static_cast<size_t>(lane)].note == t8.lanes[static_cast<size_t>(lane)].note);
        REQUIRE(t8s.lanes[static_cast<size_t>(lane)].mode == t8.lanes[static_cast<size_t>(lane)].mode);
    }
}
