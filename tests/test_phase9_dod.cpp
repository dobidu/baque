#include "audio/hardware_templates.h"
#include "audio/plock_pattern.h"
#include "audio/step_pattern.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>
#include <cmath>

// Phase 9 DoD (ESCOPO Fase 8): "Dirige TR-8 real em sync; lanes internas+externas no mesmo
// padrao." Estes testes provam a metade automatica do DoD: um padrao HIBRIDO (lanes EXT + lane
// INT) gera um stream MIDI fundido correto via BaqueProcessor::processBlock (M1: nunca
// Sequencer::generate direto — so o processBlock prova lane_routing_ + cc_out_ + clock juntos).

namespace {
constexpr double k_sr = 48000.0;
constexpr double k_bpm = 120.0;
constexpr int k_block = 480;                              // 480/24000 = 0.02 ppq por bloco
constexpr double k_samples_per_ppq = k_sr * 60.0 / k_bpm; // 24000
constexpr double k_ppq_per_block = static_cast<double>(k_block) / k_samples_per_ppq;

// Playhead de teste: dirige is_playing/bpm/ppq pelo caminho real do processBlock.
struct TestPlayHead : juce::AudioPlayHead {
    bool playing = true;
    double bpm = k_bpm;
    double ppq = 0.0;

    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo info;
        info.setIsPlaying(playing);
        info.setBpm(bpm);
        info.setPpqPosition(ppq);
        return info;
    }
};

int idx(PLockParam p) {
    return static_cast<int>(p);
}

// prepareToPlay direto nao seta getSampleRate(); o host chama setRateAndBufferSizeDetails antes.
// Sem isto sr=0 no processBlock e o sequenciador trata o bloco como largura infinita em ppq.
void prepare(BaqueProcessor& p) {
    p.setRateAndBufferSizeDetails(k_sr, 512);
    p.prepareToPlay(k_sr, 512);
}

// Padrao hibrido: lane 7 (CH=note42) EXT ativa no step 0; lane 11 INTERNA com nota 36 (pad0 =
// kick carregado) ativa no step 0. Lane 0 (BD default ativa) DESLIGADA p/ que note36 NAO saia
// pela porta EXT — assim "sem note36 no output" prova que a lane interna nao vazou MIDI.
StepPattern make_hybrid_pattern() {
    StepPattern p{};
    for (int s = 0; s < StepPattern::k_num_steps; ++s)
        p.set_active(0, s, false); // limpa lane 0 (BD) — sem EXT note36 neste teste
    p.set_active(7, 0, true);
    p.set_note(7, 0, 42); // CH (reescrito igual pelo template)
    p.set_active(11, 0, true);
    p.set_note(11, 0, 36); // nota 36 → pad0 (kick) → voz interna soa
    return p;
}
} // namespace

// P9D1 + P9D1b + P9D2 (AC-4, AC-4b, SR2): stream MIDI fundido correto + INT nao-vacuo + clock exato.
TEST_CASE("P9D1 - hybrid pattern emits EXT notes, internal stays internal, exact clock", "[dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    prepare(processor);

    processor.set_pattern(make_hybrid_pattern());
    processor.apply_hardware_template(tr8_template()); // routing lanes0-10 EXT ch10; preserva ativacoes
    processor.clock_master_ = true;

    TestPlayHead ph;
    processor.setPlayHead(&ph);

    int note_on_ch10_n42 = 0;  // CH EXT note-on
    int note_off_ch10_n42 = 0; // CH EXT note-off (gate close — sem nota presa)
    int note_on_n36_any = 0;   // INT nao deve vazar nota 36 no output
    int clock_count = 0;       // 0xF8
    double max_abs = 0.0;      // prova que a voz interna disparou (nao-vacuo)

    // 50 blocos × 0.02 ppq = 1.0 ppq (uma seminima = steps 0..3). step0 dispara uma vez.
    for (int b = 0; b < 50; ++b) {
        ph.ppq = static_cast<double>(b) * k_ppq_per_block;
        juce::AudioBuffer<float> buf(2, k_block);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < k_block; ++i)
                max_abs = std::max(max_abs, static_cast<double>(std::abs(buf.getSample(ch, i))));

        for (const auto meta : midi) {
            const auto& m = meta.getMessage();
            if (m.isMidiClock())
                ++clock_count;
            if (m.isNoteOn()) {
                if (m.getNoteNumber() == 36)
                    ++note_on_n36_any;
                if (m.getNoteNumber() == 42 && m.getChannel() == 10)
                    ++note_on_ch10_n42;
            }
            if (m.isNoteOff() && m.getNoteNumber() == 42 && m.getChannel() == 10)
                ++note_off_ch10_n42;
        }
    }
    processor.setPlayHead(nullptr);

    // P9D1 (AC-4): nota EXT no note/canal corretos.
    REQUIRE(note_on_ch10_n42 == 1);

    // P9D1b (AC-4b NAO-VACUO, M1): a voz interna PROVAVELMENTE disparou (audio != 0) E nenhuma
    // nota da lane interna (36) vazou para o output MIDI. As duas metades sao obrigatorias.
    REQUIRE(max_abs > 0.0);
    REQUIRE(note_on_n36_any == 0);

    // P9D2 (AC-4 + SR2): clock 0xF8 EXATAMENTE 24 (n=0..23, n/24 < 1.0 ppq) e note-OFF EXT presente.
    REQUIRE(clock_count == 24);
    REQUIRE(note_off_ch10_n42 == 1);
}

// P9D3 (AC-5): run sustentado hibrido fica finito por 200 blocos atravessando loop boundaries.
TEST_CASE("P9D3 - sustained hybrid run stays finite over 200 blocks", "[dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    prepare(processor);

    processor.set_pattern(make_hybrid_pattern());
    processor.apply_hardware_template(tr8_template());
    processor.clock_master_ = true;

    TestPlayHead ph;
    processor.setPlayHead(&ph);

    for (int b = 0; b < 200; ++b) { // 200 × 0.02 = 4.0 ppq > 1 pattern (loop wrap incluso)
        ph.ppq = static_cast<double>(b) * k_ppq_per_block;
        juce::AudioBuffer<float> buf(2, k_block);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < k_block; ++i)
                REQUIRE(std::isfinite(buf.getSample(ch, i)));
    }
    processor.setPlayHead(nullptr);
}

// P9D4 (AC-5): borda toca→para emite All-Notes-Off (CC123) no canal EXT (stop-flush 09-01).
TEST_CASE("P9D4 - stop-flush emits all-notes-off on EXT channel", "[dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    prepare(processor);

    processor.set_pattern(make_hybrid_pattern());
    processor.apply_hardware_template(tr8_template());
    processor.clock_master_ = true;

    TestPlayHead ph;
    processor.setPlayHead(&ph);

    // Alguns blocos tocando → was_playing_ = true.
    for (int b = 0; b < 5; ++b) {
        ph.ppq = static_cast<double>(b) * k_ppq_per_block;
        juce::AudioBuffer<float> buf(2, k_block);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);
    }

    // Para o transporte → bloco seguinte deve emitir CC123 no canal 10.
    ph.playing = false;
    juce::AudioBuffer<float> buf(2, k_block);
    juce::MidiBuffer midi;
    processor.processBlock(buf, midi);
    processor.setPlayHead(nullptr);

    bool all_notes_off_ch10 = false;
    for (const auto meta : midi) {
        const auto& m = meta.getMessage();
        if (m.isController() && m.getControllerNumber() == 123 && m.getChannel() == 10)
            all_notes_off_ch10 = true;
    }
    REQUIRE(all_notes_off_ch10);
}

// P9D5 (AC-4 CC aditivo): p-lock → CC out no canal EXT, sem deslocar as notas (mirror 09-03 AC-6).
TEST_CASE("P9D5 - p-lock CC out is additive, does not displace EXT notes", "[dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    prepare(processor);

    processor.set_pattern(make_hybrid_pattern());
    processor.apply_hardware_template(tr8_template()); // reseta cc_out_ → configurar DEPOIS

    // CC out p/ scatter_depth. O template ja liga este CC com o numero da chart da Roland (69 =
    // SCATTER DEPTH); reafirmamos aqui para o teste ser auto-contido. (Antes usava 16, que na
    // chart e DELAY LEVEL — corrigido para o numero real do scatter.)
    processor.cc_out_.enabled = true;
    processor.cc_out_.channel = 10;
    processor.cc_out_.cc_enabled[static_cast<size_t>(idx(PLockParam::scatter_depth))] = true;
    processor.cc_out_.cc_number[static_cast<size_t>(idx(PLockParam::scatter_depth))] = 69;

    // p-lock scatter_depth = 0.5 no step 0.
    processor.plock_pattern_.enabled = true;
    processor.plock_pattern_.steps[0].active[idx(PLockParam::scatter_depth)] = true;
    processor.plock_pattern_.steps[0].values[idx(PLockParam::scatter_depth)] = 0.5f;

    processor.clock_master_ = true;

    TestPlayHead ph;
    processor.setPlayHead(&ph);

    int cc69_ch10 = 0;
    int note_on_ch10_n42 = 0;
    for (int b = 0; b < 50; ++b) {
        ph.ppq = static_cast<double>(b) * k_ppq_per_block;
        juce::AudioBuffer<float> buf(2, k_block);
        juce::MidiBuffer midi;
        processor.processBlock(buf, midi);
        for (const auto meta : midi) {
            const auto& m = meta.getMessage();
            if (m.isController() && m.getControllerNumber() == 69 && m.getChannel() == 10)
                ++cc69_ch10;
            if (m.isNoteOn() && m.getNoteNumber() == 42 && m.getChannel() == 10)
                ++note_on_ch10_n42;
        }
    }
    processor.setPlayHead(nullptr);

    REQUIRE(cc69_ch10 >= 1);        // CC out emitido a partir do p-lock
    REQUIRE(note_on_ch10_n42 == 1); // nota EXT inalterada (aditivo — sem deslocar/derrubar)
}

// P9D6: marcador de DoD da Fase 9.
TEST_CASE("P9D6 - Phase 9 DoD marker", "[dod]") {
    SUCCEED("Phase 9 DoD (software half): hybrid INT/EXT merged MIDI via processBlock — EXT notes "
            "on TR-8 note/channel + note-off, internal lane fires audio without MIDI leak, exact "
            "24ppqn clock, stop-flush, additive CC out. Real-TR-8 sign-off = manual checkpoint.");
}
