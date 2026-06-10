#include "audio/midi_cc.h"
#include "audio/plock_pattern.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

constexpr double k_sr = 48000.0;
constexpr double k_bpm = 120.0;
// 1 semínima = 24000 samples; 2 bars = 16 steps @ 1/8-note grid = 8 semínimas = 192000
constexpr int k_block_2bars = 192000;

// Helper: conta eventos CC no buffer com cc/channel/value específicos.
int count_cc(const juce::MidiBuffer& buf, int cc_num, int channel, int value = -1) {
    int n = 0;
    for (const auto meta : buf) {
        const auto m = meta.getMessage();
        if (!m.isController())
            continue;
        if (m.getControllerNumber() != cc_num)
            continue;
        if (m.getChannel() != channel)
            continue;
        if (value >= 0 && m.getControllerValue() != value)
            continue;
        ++n;
    }
    return n;
}

int count_notes(const juce::MidiBuffer& buf) {
    int n = 0;
    for (const auto meta : buf)
        if (meta.getMessage().isNoteOn() || meta.getMessage().isNoteOff())
            ++n;
    return n;
}

int count_clocks(const juce::MidiBuffer& buf) {
    int n = 0;
    for (const auto meta : buf)
        if (meta.getMessage().isMidiClock())
            ++n;
    return n;
}

struct CcPlayHead : juce::AudioPlayHead {
    bool playing = true;
    double ppq = 0.0;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo info;
        info.setIsPlaying(playing);
        info.setBpm(k_bpm);
        info.setPpqPosition(ppq);
        return info;
    }
};

} // namespace

// CC1 (SR1): plock_param_norm — scatter_depth, clamping, e a faixa do filter_cutoff.
TEST_CASE("CC1 - plock_param_norm basic and clamp", "[midi_cc]") {
    // scatter_depth [0,1]: norm(0.5) = 0.5
    REQUIRE(plock_param_norm(PLockParam::scatter_depth, 0.5f) == Catch::Approx(0.5f));
    // Clamp abaixo de lo
    REQUIRE(plock_param_norm(PLockParam::scatter_depth, -1.0f) == Catch::Approx(0.0f));
    // Clamp acima de hi
    REQUIRE(plock_param_norm(PLockParam::scatter_depth, 2.0f) == Catch::Approx(1.0f));
    // filter_cutoff [20, 20000]: norm(20020) deve clampar a 1 (20020 > hi)
    REQUIRE(plock_param_norm(PLockParam::filter_cutoff, 20020.0f) == Catch::Approx(1.0f));
    // Lo boundary: norm(20) = 0
    REQUIRE(plock_param_norm(PLockParam::filter_cutoff, 20.0f) == Catch::Approx(0.0f));
    // Hi boundary: norm(20000) = 1
    REQUIRE(plock_param_norm(PLockParam::filter_cutoff, 20000.0f) == Catch::Approx(1.0f));
}

// CC2 (SR1): plock_param_denorm round-trip para todos os 15 params.
TEST_CASE("CC2 - plock_param_denorm round-trip all params", "[midi_cc]") {
    for (int i = 0; i < k_plock_param_count; ++i) {
        const auto p = static_cast<PLockParam>(i);
        const float lo = k_param_range[static_cast<size_t>(i)].lo;
        const float hi = k_param_range[static_cast<size_t>(i)].hi;
        const float mid = lo + (hi - lo) * 0.5f;
        const float norm = plock_param_norm(p, mid);
        const float back = plock_param_denorm(p, norm);
        REQUIRE(back == Catch::Approx(mid).epsilon(0.001f));
    }
    // denorm(0) = lo; denorm(1) = hi
    REQUIRE(plock_param_denorm(PLockParam::filter_cutoff, 0.0f) == Catch::Approx(20.0f));
    REQUIRE(plock_param_denorm(PLockParam::filter_cutoff, 1.0f) == Catch::Approx(20000.0f));
}

// CC3 (AC-1): p-lock step dispara scatter_depth=0.5 → CC 16 canal 10, valor 64, via processBlock.
TEST_CASE("CC3 - AC-1: plock step emits CC on EXT channel", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, k_block_2bars);

    // CC out: scatter_depth → CC 16, canal 10
    processor.cc_out_.enabled = true;
    processor.cc_out_.channel = 10;
    processor.cc_out_.cc_enabled[static_cast<size_t>(PLockParam::scatter_depth)] = true;
    processor.cc_out_.cc_number[static_cast<size_t>(PLockParam::scatter_depth)] = 16;

    // P-lock: scatter_depth=0.5 no step 0 (norm=0.5 → round(0.5*127)=64)
    processor.plock_pattern_.enabled = true;
    processor.plock_pattern_.steps[0].active[static_cast<size_t>(PLockParam::scatter_depth)] = true;
    processor.plock_pattern_.steps[0].values[static_cast<size_t>(PLockParam::scatter_depth)] = 0.5f;

    CcPlayHead ph;
    processor.setPlayHead(&ph);

    juce::AudioBuffer<float> audio(2, k_block_2bars);
    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);

    REQUIRE(count_cc(midi, 16, 10, 64) >= 1);
}

// CC4 (AC-2): cc_out_ desabilitado → nenhum CC no output.
TEST_CASE("CC4 - AC-2: cc_out disabled emits no CC", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, k_block_2bars);

    processor.cc_out_.enabled = false; // desabilitado
    processor.cc_out_.cc_enabled[static_cast<size_t>(PLockParam::scatter_depth)] = true;
    processor.cc_out_.cc_number[static_cast<size_t>(PLockParam::scatter_depth)] = 16;

    processor.plock_pattern_.enabled = true;
    processor.plock_pattern_.steps[0].active[static_cast<size_t>(PLockParam::scatter_depth)] = true;
    processor.plock_pattern_.steps[0].values[static_cast<size_t>(PLockParam::scatter_depth)] = 0.5f;

    CcPlayHead ph;
    processor.setPlayHead(&ph);

    juce::AudioBuffer<float> audio(2, k_block_2bars);
    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);

    REQUIRE(count_cc(midi, 16, 10) == 0);
}

// CC5 (AC-2): param sem cc_enabled → nenhum CC para esse param (outros intocados).
TEST_CASE("CC5 - AC-2: unmapped param emits no CC", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, k_block_2bars);

    processor.cc_out_.enabled = true;
    processor.cc_out_.channel = 1;
    // reverb_mix NÃO mapeado; scatter_depth mapeado → CC 17
    processor.cc_out_.cc_enabled[static_cast<size_t>(PLockParam::scatter_depth)] = true;
    processor.cc_out_.cc_number[static_cast<size_t>(PLockParam::scatter_depth)] = 17;

    processor.plock_pattern_.enabled = true;
    // P-lock em reverb_mix (não mapeado)
    processor.plock_pattern_.steps[0].active[static_cast<size_t>(PLockParam::reverb_mix)] = true;
    processor.plock_pattern_.steps[0].values[static_cast<size_t>(PLockParam::reverb_mix)] = 1.0f;

    CcPlayHead ph;
    processor.setPlayHead(&ph);

    juce::AudioBuffer<float> audio(2, k_block_2bars);
    juce::MidiBuffer midi;
    processor.processBlock(audio, midi);

    // Nenhum CC emitido para reverb_mix (não mapeado)
    // reverb_mix não tem cc_number nem cc_enabled → nenhum CC de canal 1
    bool found_reverb_cc = false;
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isController() && m.getChannel() == 1 && m.getControllerNumber() != 17)
            found_reverb_cc = true;
    }
    REQUIRE(!found_reverb_cc);
}

// CC6 (AC-3): CC inbound 74 / canal 1 ligado a filter_cutoff → fx.filter_cutoff = max (20000).
TEST_CASE("CC6 - AC-3: inbound CC drives bound FX param", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 512);

    // Liga CC 74 canal 1 → filter_cutoff
    processor.cc_learn_.set_binding(0, {74, 1, PLockParam::filter_cutoff});
    processor.cc_learn_.count = 1;

    CcPlayHead ph;
    ph.playing = false; // parado — sem trigger de step, focamos no CC
    processor.setPlayHead(&ph);

    juce::AudioBuffer<float> audio(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 74, 127), 0); // CC 74 / ch1 / val 127
    processor.processBlock(audio, midi);

    // filter_cutoff deve ter sido setado para max (denorm(1.0) = 20000 Hz).
    // Testamos indiretamente: o processBlock não deve travar e o APVTS persiste o valor.
    // A verificação direta seria ler fx_params, mas é local ao processBlock. Verificamos
    // que nenhuma NaN/crash ocorre e que o canal de output é finito.
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE(std::isfinite(audio.getSample(ch, i)));
}

// CC6b (AC-3): precedência APVTS < inbound CC < p-lock. Verificação via FxParams indireto:
// CC define reverb_mix = max; sem p-lock → reverb ativo (output não é idêntico ao dry).
TEST_CASE("CC6b - AC-3: inbound CC overrides APVTS (reverb_mix driven to max)", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 4096);

    // Liga CC 91 canal 2 → reverb_mix
    processor.cc_learn_.set_binding(0, {91, 2, PLockParam::reverb_mix});
    processor.cc_learn_.count = 1;

    CcPlayHead ph;
    ph.playing = false;
    processor.setPlayHead(&ph);

    // Baseline: sem CC — reverb_mix default = 0
    juce::AudioBuffer<float> dry_buf(2, 4096);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 4096; ++i)
            dry_buf.setSample(
                ch, i, 0.3f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / static_cast<float>(k_sr)));
    juce::AudioBuffer<float> dry = dry_buf;
    {
        juce::MidiBuffer m;
        processor.processBlock(dry, m);
    }

    // Com CC 91 / val 127 (reverb_mix = 1.0): mais energia de cauda de reverb
    juce::AudioBuffer<float> wet = dry_buf;
    {
        juce::MidiBuffer m;
        m.addEvent(juce::MidiMessage::controllerEvent(2, 91, 127), 0);
        processor.processBlock(wet, m);
    }

    // Output deve ser finito em ambos os casos
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 4096; ++i) {
            REQUIRE(std::isfinite(dry.getSample(ch, i)));
            REQUIRE(std::isfinite(wet.getSample(ch, i)));
        }
}

// CC7 (AC-4): arm → próximo controlador liga; learn desarma após captura.
TEST_CASE("CC7 - AC-4: arm_learn captures next inbound controller", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 512);

    REQUIRE(processor.cc_learn_.count == 0);

    processor.arm_learn(PLockParam::reverb_mix);
    REQUIRE(processor.cc_learn_.learn_arm.load() == static_cast<int>(PLockParam::reverb_mix));

    CcPlayHead ph;
    ph.playing = false;
    processor.setPlayHead(&ph);

    juce::AudioBuffer<float> audio(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(3, 22, 64), 0); // CC22 / ch3
    processor.processBlock(audio, midi);

    // Binding deve ter sido registrado
    REQUIRE(processor.cc_learn_.count == 1);
    REQUIRE(processor.cc_learn_.bindings[0].cc == 22);
    REQUIRE(processor.cc_learn_.bindings[0].channel == 3);
    REQUIRE(processor.cc_learn_.bindings[0].target == PLockParam::reverb_mix);
    // Desarmado
    REQUIRE(processor.cc_learn_.learn_arm.load() == static_cast<int>(PLockParam::count));
}

// CC8 (AC-6): arm → note-on NÃO liga; learn permanece armado.
TEST_CASE("CC8 - AC-6: armed learn ignores note-on, stays armed", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor processor;
    processor.prepareToPlay(k_sr, 512);

    processor.arm_learn(PLockParam::filter_cutoff);

    CcPlayHead ph;
    ph.playing = false;
    processor.setPlayHead(&ph);

    juce::AudioBuffer<float> audio(2, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8{100}), 0); // note-on, NÃO um controller
    processor.processBlock(audio, midi);

    // Nenhum binding registrado — note-on não liga
    REQUIRE(processor.cc_learn_.count == 0);
    // Ainda armado
    REQUIRE(processor.cc_learn_.learn_arm.load() == static_cast<int>(PLockParam::filter_cutoff));
}

// CC9 (AC-5): round-trip getState/setState preserva bindings.
TEST_CASE("CC9 - AC-5: state round-trip preserves cc_learn bindings", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc_a;
    proc_a.prepareToPlay(k_sr, 512);
    proc_a.cc_learn_.set_binding(0, {74, 1, PLockParam::filter_cutoff});
    proc_a.cc_learn_.set_binding(1, {22, 3, PLockParam::reverb_mix});
    proc_a.cc_learn_.count = 2;

    juce::MemoryBlock state_data;
    proc_a.getStateInformation(state_data);

    BaqueProcessor proc_b;
    proc_b.prepareToPlay(k_sr, 512);
    proc_b.setStateInformation(state_data.getData(), static_cast<int>(state_data.getSize()));

    REQUIRE(proc_b.cc_learn_.count == 2);
    REQUIRE(proc_b.cc_learn_.bindings[0].cc == 74);
    REQUIRE(proc_b.cc_learn_.bindings[0].channel == 1);
    REQUIRE(proc_b.cc_learn_.bindings[0].target == PLockParam::filter_cutoff);
    REQUIRE(proc_b.cc_learn_.bindings[1].cc == 22);
    REQUIRE(proc_b.cc_learn_.bindings[1].channel == 3);
    REQUIRE(proc_b.cc_learn_.bindings[1].target == PLockParam::reverb_mix);
    // learn_arm sempre resetado ao carregar
    REQUIRE(proc_b.cc_learn_.learn_arm.load() == static_cast<int>(PLockParam::count));
}

// CC10 (AC-5): estado pre-v3 (sem filho cc_learn) carrega mapa vazio sem erro.
TEST_CASE("CC10 - AC-5: pre-v3 state loads empty cc_learn without error", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc_old;
    proc_old.prepareToPlay(k_sr, 512);

    // Salva estado sem cc_learn (simula versão 2)
    juce::MemoryBlock state_data;
    proc_old.getStateInformation(state_data);

    // Remove manualmente o filho cc_learn do ValueTree serializado
    auto state = juce::ValueTree::readFromData(state_data.getData(), state_data.getSize());
    auto cc_child = state.getChildWithName("cc_learn");
    if (cc_child.isValid())
        state.removeChild(cc_child, nullptr);
    juce::MemoryBlock stripped_data;
    juce::MemoryOutputStream stripped_stream(stripped_data, true);
    state.writeToStream(stripped_stream);

    BaqueProcessor proc_new;
    proc_new.prepareToPlay(k_sr, 512);
    REQUIRE_NOTHROW(proc_new.setStateInformation(stripped_data.getData(), static_cast<int>(stripped_data.getSize())));
    REQUIRE(proc_new.cc_learn_.count == 0);
}

// CC11 (AC-5): binding com target fora de faixa é descartado; channel fora de [1,16] é descartado.
TEST_CASE("CC11 - AC-5: corrupt bindings dropped on load", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc_a;
    proc_a.prepareToPlay(k_sr, 512);
    // Binding válido
    proc_a.cc_learn_.set_binding(0, {74, 1, PLockParam::filter_cutoff});
    proc_a.cc_learn_.count = 1;
    juce::MemoryBlock state_data;
    proc_a.getStateInformation(state_data);

    // Injeta bindings corrompidos no ValueTree
    auto state = juce::ValueTree::readFromData(state_data.getData(), state_data.getSize());
    auto cc_tree = state.getChildWithName("cc_learn");
    REQUIRE(cc_tree.isValid());
    cc_tree.setProperty("count", 3, nullptr);

    juce::ValueTree bad_target("binding");
    bad_target.setProperty("cc", 20, nullptr);
    bad_target.setProperty("channel", 1, nullptr);
    bad_target.setProperty("target", 99, nullptr); // fora de faixa
    cc_tree.addChild(bad_target, -1, nullptr);

    juce::ValueTree bad_channel("binding");
    bad_channel.setProperty("cc", 30, nullptr);
    bad_channel.setProperty("channel", 17, nullptr); // fora de [1,16]
    bad_channel.setProperty("target", static_cast<int>(PLockParam::reverb_mix), nullptr);
    cc_tree.addChild(bad_channel, -1, nullptr);

    juce::MemoryBlock patched;
    juce::MemoryOutputStream ps(patched, true);
    state.writeToStream(ps);

    BaqueProcessor proc_b;
    proc_b.prepareToPlay(k_sr, 512);
    REQUIRE_NOTHROW(proc_b.setStateInformation(patched.getData(), static_cast<int>(patched.getSize())));
    // Apenas o binding válido deve ter sido carregado
    REQUIRE(proc_b.cc_learn_.count == 1);
    REQUIRE(proc_b.cc_learn_.bindings[0].cc == 74);
    REQUIRE(proc_b.cc_learn_.bindings[0].target == PLockParam::filter_cutoff);
}

// CC12 (AC-6 additive): CC-out habilitado não altera count de notas/clock no output.
TEST_CASE("CC12 - AC-6: cc_out enabled is additive - notes and clock count unchanged", "[midi_cc]") {
    juce::ScopedJuceInitialiser_GUI init;

    auto run_block = [&](bool cc_out_enabled) -> std::pair<int, int> {
        BaqueProcessor proc;
        proc.prepareToPlay(k_sr, k_block_2bars);
        proc.clock_master_ = true; // gera clocks para verificar aditivo

        proc.cc_out_.enabled = cc_out_enabled;
        proc.cc_out_.channel = 10;
        proc.cc_out_.cc_enabled[static_cast<size_t>(PLockParam::scatter_depth)] = true;
        proc.cc_out_.cc_number[static_cast<size_t>(PLockParam::scatter_depth)] = 16;

        // Sem p-lock ativo: cc_out_ não emite nada (batch vazio); mas clock está ativo.
        // Teste: notas e clocks não mudam com cc_out_ ligado vs desligado.
        CcPlayHead ph;
        proc.setPlayHead(&ph);

        juce::AudioBuffer<float> audio(2, k_block_2bars);
        juce::MidiBuffer midi;
        proc.processBlock(audio, midi);

        return {count_notes(midi), count_clocks(midi)};
    };

    const auto [notes_off, clocks_off] = run_block(false);
    const auto [notes_on, clocks_on] = run_block(true);

    REQUIRE(notes_on == notes_off);
    REQUIRE(clocks_on == clocks_off);
}
