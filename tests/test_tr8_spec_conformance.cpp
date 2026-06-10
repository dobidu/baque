#include "audio/hardware_templates.h"
#include "audio/lane_routing.h"
#include "audio/midi_cc.h"
#include "audio/plock_pattern.h"
#include "audio/step_pattern.h"
#include "audio/tr8_midi_spec.h"

#include <catch2/catch_test_macros.hpp>

// Tier-3 (sem hardware): conformidade do template contra a MIDI Implementation Chart PUBLICADA
// da Roland (TR-8 v1.11, tr8_midi_spec.h). Fecha a metade que o DoD de software (test_phase9_dod)
// NAO cobre: os NUMEROS de nota/CC batem com a spec do fabricante? O template e escrito a mao;
// tr8_spec vem da chart. Este arquivo prova a igualdade das duas fontes — equivalente, a nivel de
// bytes, a confirmar contra o hardware (a chart e a spec de firmware do TR-8).

static int param_idx(PLockParam p) {
    return static_cast<int>(p);
}

// TS1: cada lane EXT do template manda a nota que a Roland documenta para aquele instrumento.
TEST_CASE("TS1 - TR-8 template note map matches the Roland MIDI Implementation Chart", "[hardware][spec]") {
    const auto t = tr8_template();
    REQUIRE(t.channel == tr8_spec::k_default_channel); // Basic Channel Default = 10

    for (int lane = 0; lane <= 10; ++lane) {
        REQUIRE(t.lanes[static_cast<size_t>(lane)].mapped);
        REQUIRE(t.lanes[static_cast<size_t>(lane)].mode == LaneMode::external);
        REQUIRE(t.lanes[static_cast<size_t>(lane)].note == tr8_spec::k_lane_notes[static_cast<size_t>(lane)]);
    }
}

// TS2: os CCs CONTINUOS ligados carregam o numero EXATO da chart (nao um valor arbitrario).
TEST_CASE("TS2 - enabled continuous CCs carry the exact Roland CC numbers", "[hardware][spec]") {
    const auto t = tr8_template();
    REQUIRE(t.cc_out_enabled);

    struct Expect {
        PLockParam param;
        uint8_t cc;
    };
    const Expect cont[] = {
        {PLockParam::scatter_depth, tr8_spec::k_cc_scatter_depth}, // 69
        {PLockParam::reverb_mix, tr8_spec::k_cc_reverb_level},     // 91
        {PLockParam::delay_mix, tr8_spec::k_cc_delay_level},       // 16
        {PLockParam::delay_time, tr8_spec::k_cc_delay_time},       // 17
    };
    for (const auto& e : cont) {
        const auto i = static_cast<size_t>(param_idx(e.param));
        REQUIRE(t.cc_enabled[i]);
        REQUIRE(t.cc_number[i] == e.cc);
    }
}

// TS3: scatter_type/scatter_sw (DISCRETOS) ficam OFF — conhecemos o CC, nao a curva de valor.
// Honestidade: nao dirigimos um selecionador de padrao discreto com lei linear ate confirmar.
TEST_CASE("TS3 - discrete scatter params stay OFF pending value-map confirmation", "[hardware][spec]") {
    const auto t = tr8_template();
    REQUIRE_FALSE(t.cc_enabled[static_cast<size_t>(param_idx(PLockParam::scatter_type))]);
    // O numero do CC discreto existe na spec (documentado), so nao e enviado ainda.
    REQUIRE(tr8_spec::k_cc_scatter_type == 68);
    REQUIRE(tr8_spec::k_cc_scatter_sw == 70);
}

// TS4: params sem equivalente no TR-8 (filter/bit/sr/granular/tape/gate) NAO ligam CC — nao
// inventamos destino. So os 4 continuos mapeados ficam ON; o resto OFF.
TEST_CASE("TS4 - BAQUE-only FX params do not enable any CC", "[hardware][spec]") {
    const auto t = tr8_template();
    const PLockParam off[] = {
        PLockParam::filter_cutoff,
        PLockParam::filter_res,
        PLockParam::sidechain_threshold,
        PLockParam::bit_depth,
        PLockParam::sr_factor,
        PLockParam::granular_spray,
        PLockParam::granular_pitch_spread,
        PLockParam::granular_freeze,
        PLockParam::scatter_type,
        PLockParam::tape_stop,
        PLockParam::gate_depth,
    };
    for (const auto p : off)
        REQUIRE_FALSE(t.cc_enabled[static_cast<size_t>(param_idx(p))]);

    int enabled = 0;
    for (int i = 0; i < k_plock_param_count; ++i)
        if (t.cc_enabled[static_cast<size_t>(i)])
            ++enabled;
    REQUIRE(enabled == 4); // exatamente os 4 continuos mapeados
}

// TS5: apply_template propaga os CCs da chart para o CcOutRouting vivo (channel 10, enabled).
TEST_CASE("TS5 - apply_template propagates chart CCs to live cc_out routing", "[hardware][spec]") {
    LaneRouting routing{};
    CcOutRouting cc{};
    StepPattern pattern{};

    apply_template(tr8_template(), routing, cc, pattern);

    REQUIRE(cc.enabled);
    REQUIRE(cc.channel == tr8_spec::k_default_channel);
    REQUIRE(cc.cc_enabled[static_cast<size_t>(param_idx(PLockParam::scatter_depth))]);
    REQUIRE(cc.cc_number[static_cast<size_t>(param_idx(PLockParam::scatter_depth))] == tr8_spec::k_cc_scatter_depth);
    REQUIRE(cc.cc_number[static_cast<size_t>(param_idx(PLockParam::reverb_mix))] == tr8_spec::k_cc_reverb_level);
}
