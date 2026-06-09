#include "audio/midi_clock.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>

namespace {
constexpr double k_sr = 48000.0;
constexpr double k_bpm = 120.0;
// 120 bpm / 48k: samples_per_ppq = 48000*60/120 = 24000; tick spacing = 1000 samples
constexpr int k_ppq_block = 24000; // exatamente 1 semínima = 24 ticks

int count_type(const juce::MidiBuffer& buf, bool (*pred)(const juce::MidiMessage&)) {
    int n = 0;
    for (const auto meta : buf)
        if (pred(meta.getMessage()))
            ++n;
    return n;
}
bool is_clock(const juce::MidiMessage& m) {
    return m.isMidiClock();
}
bool is_start(const juce::MidiMessage& m) {
    return m.isMidiStart();
}
bool is_stop(const juce::MidiMessage& m) {
    return m.isMidiStop();
}
bool is_continue(const juce::MidiMessage& m) {
    return m.isMidiContinue();
}

} // namespace

// MC1 (AC-1): master, 120bpm/48k, 1 semínima → exatamente 24 clocks 0xF8.
TEST_CASE("MC1 - 24 clocks per quarter note", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    clk.process(buf, k_bpm, 0.0, k_ppq_block, true, true);
    REQUIRE(count_type(buf, is_clock) == 24);
}

// MC2 (AC-4): ticks espaçados ~1000 samples; todos em [0, block); tick0 no sample 0.
TEST_CASE("MC2 - tick positions are sample-accurate", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    clk.process(buf, k_bpm, 0.0, k_ppq_block, true, true);

    int prev_pos = -1;
    int tick_idx = 0;
    for (const auto meta : buf) {
        if (!meta.getMessage().isMidiClock())
            continue;
        const int pos = meta.samplePosition;
        REQUIRE(pos >= 0);
        REQUIRE(pos < k_ppq_block);
        if (prev_pos >= 0) {
            // espaçamento deve ser ~1000 amostras (±2 por arredondamento)
            REQUIRE(pos - prev_pos >= 998);
            REQUIRE(pos - prev_pos <= 1002);
        } else {
            REQUIRE(pos <= 1); // tick 0 no sample 0
        }
        prev_pos = pos;
        ++tick_idx;
    }
    REQUIRE(tick_idx == 24);
}

// MC3 (AC-2): borda parado->tocando a ppq 0 → Start (0xFA).
TEST_CASE("MC3 - play from zero emits Start", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    clk.process(buf, k_bpm, 0.0, 512, true, true); // was_playing_=false → start edge
    REQUIRE(count_type(buf, is_start) == 1);
    REQUIRE(count_type(buf, is_continue) == 0);
}

// MC4 (AC-2): borda tocando->parado → Stop (0xFC), 1×.
TEST_CASE("MC4 - stop edge emits Stop once", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    // Bloco 1: tocando (was_playing_ = true após)
    clk.process(buf, k_bpm, 0.0, 512, true, true);
    buf.clear();
    // Bloco 2: parado → borda toca->para
    clk.process(buf, k_bpm, 0.0, 512, false, true);
    REQUIRE(count_type(buf, is_stop) == 1);
    REQUIRE(count_type(buf, is_start) == 0);
}

// MC5 (AC-2): borda parado->tocando a ppq > 0 → Continue (0xFB), não Start.
TEST_CASE("MC5 - resume from mid-song emits Continue not Start", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    clk.process(buf, k_bpm, 2.0, 512, true, true); // ppq=2.0, was_playing_=false
    REQUIRE(count_type(buf, is_continue) == 1);
    REQUIRE(count_type(buf, is_start) == 0);
}

// MC6 (AC-3): master=false → zero mensagens.
TEST_CASE("MC6 - clock off emits nothing", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    clk.process(buf, k_bpm, 0.0, k_ppq_block, true, false);
    REQUIRE(buf.isEmpty());
}

// MC8 (AC-6, M1/SR1): 2 blocos contiguos cobrindo 1 semínima → soma = 24, sem dup na fronteira.
TEST_CASE("MC8 - two contiguous blocks sum to 24 no boundary duplicate", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    // Bloco 1: ppq 0.0 → 0.5 (12000 samples → 12 ticks 0..11)
    clk.process(buf, k_bpm, 0.0, 12000, true, true);
    const int after_block1 = count_type(buf, is_clock);
    REQUIRE(after_block1 == 12);

    // Bloco 2: ppq 0.5 → 1.0 (mais 12000 samples → ticks 12..23)
    clk.process(buf, k_bpm, 0.5, 12000, true, true);
    const int total = count_type(buf, is_clock);
    REQUIRE(total == 24);

    // total==24 prova monotonicidade: dup na fronteira = 25, tick perdido = 23.
    // Nenhuma verificação extra necessária (total é a evidência direta da AC-6/M1).
}

// MC9 (AC-6, regressao): apos avanco de ppq, regressao (loop) → resync sem re-emitir historico.
TEST_CASE("MC9 - ppq regression resyncs without re-emitting history", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    MidiClock clk;
    clk.prepare(k_sr);
    juce::MidiBuffer buf;
    // Bloco 1: 1 semínima completa (24 ticks 0..23), last_tick_=23
    clk.process(buf, k_bpm, 0.0, k_ppq_block, true, true);
    REQUIRE(count_type(buf, is_clock) == 24);

    // Regressao de ppq: DAW loop volta ao ppq=0.5 (tick 12)
    buf.clear();
    clk.process(buf, k_bpm, 0.5, 12000, true, true);
    // Esperado: ticks 12..23 apenas (12 ticks, sem re-emitir 0..11)
    const int after_regression = count_type(buf, is_clock);
    REQUIRE(after_regression == 12);
}

// MC7 (AC-5): BaqueProcessor com clock_master_=true → 0xF8 aparecem no MIDI out do processBlock.
namespace {
struct ClockPlayHead : juce::AudioPlayHead {
    bool playing = true;
    double ppq = 0.0;
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override {
        juce::AudioPlayHead::PositionInfo info;
        info.setIsPlaying(playing);
        info.setBpm(k_bpm);
        info.setPpqPosition(ppq);
        return info;
    }
};
} // namespace

TEST_CASE("MC7 - processBlock emits clock bytes when master enabled", "[midi_clock]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, k_ppq_block);
    processor.clock_master_ = true;

    ClockPlayHead ph;
    processor.setPlayHead(&ph);

    // Bloco cobrindo 1 semínima → deve sair 24 clocks no MIDI out
    juce::AudioBuffer<float> audio_buf(2, k_ppq_block);
    juce::MidiBuffer midi;
    processor.processBlock(audio_buf, midi);

    int clocks = 0;
    for (const auto meta : midi)
        if (meta.getMessage().isMidiClock())
            ++clocks;
    REQUIRE(clocks == 24);

    // Caminho de audio intacto: sem valores nao-finitos
    for (int ch = 0; ch < audio_buf.getNumChannels(); ++ch) {
        const float* data = audio_buf.getReadPointer(ch);
        for (int i = 0; i < k_ppq_block; ++i)
            REQUIRE(std::isfinite(data[i]));
    }
}
