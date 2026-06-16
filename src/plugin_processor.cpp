#include "plugin_processor.h"

#include "plugin_editor.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <BinaryData.h>
#include <algorithm>
#include <cmath>

// Layout de parâmetros APVTS
juce::AudioProcessorValueTreeState::ParameterLayout BaqueProcessor::create_parameter_layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"master_gain", 1}, "Master Gain", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"filter_cutoff", 1},
                                                           "Filter Cutoff",
                                                           juce::NormalisableRange<float>{20.0f, 20000.0f, 0.0f, 0.3f},
                                                           20000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filter_res", 1}, "Filter Resonance", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.1f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb_mix", 1}, "Reverb Mix", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"delay_mix", 1}, "Delay Mix", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"delay_time", 1},
                                                           "Delay Time",
                                                           juce::NormalisableRange<float>{0.001f, 2.0f, 0.0f, 0.5f},
                                                           0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sidechain_threshold", 1},
                                                           "Sidechain Threshold",
                                                           juce::NormalisableRange<float>{-60.0f, 0.0f},
                                                           -12.0f));
    // Scatter perf FX (Fase 8-02): type discreto 0-10 (0=off), depth contínuo 0-1. APVTS default
    // neutro + override por p-lock (mesmo contrato de filter_cutoff/etc).
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"scatter_type", 1}, "Scatter Type", juce::NormalisableRange<float>{0.0f, 10.0f, 1.0f}, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"scatter_depth", 1}, "Scatter Depth", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.0f));
    // Perf FX 08-03: tape-stop (0=normal, 1=parado) + gater depth (0=off, 1=chop total)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tape_stop", 1}, "Tape Stop", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gate_depth", 1}, "Gate Depth", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.0f));
    return layout;
}

BaqueProcessor::BaqueProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts_(*this, nullptr, "BAQUE_PARAMS", create_parameter_layout()) {}

void BaqueProcessor::prepareToPlay(double sample_rate, int samples_per_block) {
    // PROTOCOLO DE LOAD SEGURO (auditoria 04-01): reset_all() ANTES de qualquer
    // mutação de buffer de pad — nenhuma voz pode referenciar memória realocada.
    // Seguro para chamar múltiplas vezes.
    voice_pool_.reset_all();
    voice_pool_.prepare(sample_rate);
    scheduler_.prepare(sample_rate);
    feel_engine_.prepare();
    feel_engine_.set_seed(feel_pattern_.seed);
    fx_chain_.prepare(sample_rate, samples_per_block);
    scatter_.prepare(sample_rate, samples_per_block);
    gater_.prepare(sample_rate, samples_per_block);
    tape_stop_.prepare(sample_rate, samples_per_block);
    block_start_sample_ = 0;

    // Decodifica o sample de teste no pad 0 apenas uma vez (guarda de realocação).
    // Preserva o comportamento da Fase 2: nota 36 dispara o sample carregado.
    if (!pad0_loaded_) {
        auto stream =
            std::make_unique<juce::MemoryInputStream>(BinaryData::test_kick_wav, BinaryData::test_kick_wavSize, false);
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(stream.release(), true));

        if (reader != nullptr) {
            const int num_samples = static_cast<int>(reader->lengthInSamples);
            auto& pad_buffer = pad_bank_.pad(0).sample_buffer();
            pad_buffer.setSize(1, num_samples);
            reader->read(&pad_buffer, 0, num_samples, 0, true, false);
        }
        pad0_loaded_ = true;
    }

    // Pré-aloca buffers de mixagem e buffer MIDI do sequenciador
    mix_left_.assign(static_cast<std::vector<float>::size_type>(samples_per_block), 0.0f);
    mix_right_.assign(static_cast<std::vector<float>::size_type>(samples_per_block), 0.0f);
    midi_buffer_seq_.ensureSize(512); // 32 eventos × ~12 bytes + margem
    midi_buffer_ext_.ensureSize(512); // saída MIDI das lanes EXT (Fase 9-01)
    was_playing_ = false;
    midi_clock_.prepare(sample_rate);

    // Inicializa suavização de ganho (10ms de ramp)
    gain_smoother_.reset(sample_rate, 0.01);
    if (auto* param = apvts_.getRawParameterValue("master_gain"))
        gain_smoother_.setCurrentAndTargetValue(*param);
}

void BaqueProcessor::releaseResources() {
    fx_chain_.reset();
    scatter_.reset();
    gater_.reset();
    tape_stop_.reset();
    midi_clock_.reset();
}

bool BaqueProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

// Despacha um comando da fila UI→engine para o struct de destino correto.
// Chamado no audio thread (drain de processBlock) OU na message thread dentro de
// um bracket suspendProcessing (getState/setState — role-transfer documentado).
// Clamp de índices/valores: bugs de UI não corrompem o estado do engine.
// switch default jassertfalse: enum é exaustivo; nenhum tipo deve ficar sem case.
void BaqueProcessor::dispatch_ui_command(const UiCommand& cmd) noexcept {
    switch (cmd.type) {
    case UiCommandType::set_fill:
        perf_state_.fill_active = (cmd.c != 0);
        break;
    case UiCommandType::set_mute: {
        const int lane = juce::jlimit(0, StepPattern::k_num_lanes - 1, cmd.a);
        perf_state_.mute[static_cast<std::size_t>(lane)] = (cmd.c != 0);
        break;
    }
    case UiCommandType::set_solo: {
        const int lane = juce::jlimit(0, StepPattern::k_num_lanes - 1, cmd.a);
        perf_state_.solo[static_cast<std::size_t>(lane)] = (cmd.c != 0);
        break;
    }
    case UiCommandType::set_lane_mode: {
        const int lane = juce::jlimit(0, StepPattern::k_num_lanes - 1, cmd.a);
        lane_routing_.mode[static_cast<std::size_t>(lane)] = static_cast<LaneMode>(juce::jlimit(0, 2, cmd.c));
        break;
    }
    case UiCommandType::set_lane_channel: {
        const int lane = juce::jlimit(0, StepPattern::k_num_lanes - 1, cmd.a);
        lane_routing_.channel[static_cast<std::size_t>(lane)] = static_cast<uint8_t>(juce::jlimit(0, 16, cmd.c));
        break;
    }
    case UiCommandType::set_clock_master:
        clock_master_ = (cmd.c != 0);
        break;
    case UiCommandType::set_cc_out_enabled:
        cc_out_.enabled = (cmd.c != 0);
        break;
    case UiCommandType::set_cc_out_channel:
        cc_out_.channel = static_cast<uint8_t>(juce::jlimit(1, 16, cmd.c));
        break;
    case UiCommandType::set_cc_slot: {
        const int param = juce::jlimit(0, k_plock_param_count - 1, cmd.a);
        cc_out_.cc_number[static_cast<std::size_t>(param)] = static_cast<uint8_t>(juce::jlimit(0, 127, cmd.b));
        cc_out_.cc_enabled[static_cast<std::size_t>(param)] = (cmd.c != 0);
        break;
    }
    case UiCommandType::set_pad_gain: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).gain = cmd.f;
        break;
    }
    case UiCommandType::set_pad_pan: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).pan = cmd.f;
        break;
    }
    case UiCommandType::set_pad_pitch_semitones: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).pitch_semitones = cmd.c;
        break;
    }
    case UiCommandType::set_pad_pitch_cents: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).pitch_cents = cmd.c;
        break;
    }
    case UiCommandType::set_pad_reverse: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).reverse = (cmd.c != 0);
        break;
    }
    case UiCommandType::set_pad_choke_group: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).choke_group = static_cast<uint8_t>(juce::jlimit(0, 8, cmd.c));
        break;
    }
    case UiCommandType::set_pad_play_mode: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).play_mode = static_cast<PlayMode>(juce::jlimit(0, 2, cmd.c));
        break;
    }
    case UiCommandType::set_pad_adsr_attack: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).adsr.attack_ms = cmd.f;
        break;
    }
    case UiCommandType::set_pad_adsr_decay: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).adsr.decay_ms = cmd.f;
        break;
    }
    case UiCommandType::set_pad_adsr_sustain: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).adsr.sustain = juce::jlimit(0.0f, 1.0f, cmd.f);
        break;
    }
    case UiCommandType::set_pad_adsr_release: {
        const int pad = juce::jlimit(0, PadBank::k_num_pads - 1, cmd.a);
        pad_bank_.pad(pad).adsr.release_ms = cmd.f;
        break;
    }
    case UiCommandType::set_step: {
        const int lane = juce::jlimit(0, StepPattern::k_num_lanes - 1, cmd.a);
        const int step = juce::jlimit(0, StepPattern::k_num_steps - 1, cmd.b);
        // sequencer_.pattern() = acesso mutável audio-thread-only (AC-2).
        sequencer_.pattern().set_active(lane, step, cmd.c != 0);
        break;
    }
    case UiCommandType::set_step_velocity:
        // StepPattern não tem campo velocity por step em v1 — deferred para 10-03.
        break;
    case UiCommandType::set_plock: {
        const int step = juce::jlimit(0, PLockPattern::k_steps - 1, cmd.b);
        const int param = juce::jlimit(0, k_plock_param_count - 1, cmd.c);
        plock_pattern_.steps[step].values[param] = juce::jlimit(k_param_range[static_cast<std::size_t>(param)].lo,
                                                                k_param_range[static_cast<std::size_t>(param)].hi,
                                                                cmd.f);
        plock_pattern_.steps[step].active[param] = true;
        plock_pattern_.enabled = true;
        break;
    }
    case UiCommandType::clear_plock: {
        const int step = juce::jlimit(0, PLockPattern::k_steps - 1, cmd.b);
        const int param = juce::jlimit(0, k_plock_param_count - 1, cmd.c);
        plock_pattern_.steps[step].active[param] = false;
        break;
    }
    case UiCommandType::apply_template:
        if (cmd.a == 0) {
            apply_hardware_template(tr8_template());
        } else if (cmd.a == 1) {
            apply_hardware_template(tr8s_template());
        } else {
            jassertfalse; // id inválido — ignore (SR2: clamp = template errado silencioso)
        }
        break;
    default:
        jassertfalse; // UiCommandType não mapeado — atualizar dispatch ao adicionar tipos
        break;
    }
}

void BaqueProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) {
    juce::ScopedNoDenormals no_denormals;

    const int num_frames = buffer.getNumSamples();
    buffer.clear();

    // Drena comandos UI pendentes antes de qualquer leitura de estado (AC-2).
    // Garante que set_mute/solo/fill/lane_mode/etc estejam aplicados antes de generate().
    ui_commands_.drain([this](const UiCommand& cmd) { dispatch_ui_command(cmd); });

    // Atualiza alvo de ganho a partir do APVTS (thread-safe: getRawParameterValue é atômico)
    if (auto* param = apvts_.getRawParameterValue("master_gain"))
        gain_smoother_.setTargetValue(*param);

    // Lê estado de transporte do host (null-safe — pode ser nullptr em alguns hosts/testes)
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            transport_.is_playing = pos->getIsPlaying();
            if (auto bpm = pos->getBpm())
                transport_.bpm = *bpm;
            if (auto ppq = pos->getPpqPosition())
                transport_.ppq_position = *ppq;
        }
    }
    // Se sem playhead: transport_ mantém valores anteriores (correto para testes unitários)

    // Snapshot de FX params do APVTS para o bloco (RT-safe: getRawParameterValue é atômico)
    FxParams fx_params;
    fx_params.filter_cutoff = *apvts_.getRawParameterValue("filter_cutoff");
    fx_params.filter_res = *apvts_.getRawParameterValue("filter_res");
    fx_params.reverb_mix = *apvts_.getRawParameterValue("reverb_mix");
    fx_params.delay_mix = *apvts_.getRawParameterValue("delay_mix");
    fx_params.delay_time = *apvts_.getRawParameterValue("delay_time");
    fx_params.sidechain_threshold = *apvts_.getRawParameterValue("sidechain_threshold");
    fx_params.scatter_type = *apvts_.getRawParameterValue("scatter_type");
    fx_params.scatter_depth = *apvts_.getRawParameterValue("scatter_depth");
    fx_params.tape_stop = *apvts_.getRawParameterValue("tape_stop");
    fx_params.gate_depth = *apvts_.getRawParameterValue("gate_depth");
    // PRECEDÊNCIA: snapshot do APVTS primeiro; apply_plock_batch (abaixo) sobrescreve por step.

    // Gera eventos MIDI do sequenciador no buffer pré-alocado (limpa antes de preencher)
    midi_buffer_seq_.clear();
    midi_buffer_ext_.clear(); // saída MIDI das lanes EXT (Fase 9-01)
    PLockBatch plock_batch;
    sequencer_.generate(transport_,
                        midi_buffer_seq_,
                        num_frames,
                        getSampleRate(),
                        feel_pattern_.enabled ? &feel_pattern_ : nullptr,
                        feel_pattern_.enabled ? &feel_engine_ : nullptr,
                        block_start_sample_,
                        plock_pattern_.enabled ? &plock_pattern_ : nullptr,
                        &plock_batch,
                        &perf_state_,       // fills (trig conditions) + mute/solo (Fase 8-04)
                        &lane_routing_,     // roteamento INT/EXT/BOTH por lane (Fase 9-01)
                        &midi_buffer_ext_); // saída MIDI das lanes EXT
    // Inbound CC (Fase 9-03): itera midi_messages ANTES de apply_plock_batch → precedência
    // APVTS < inbound CC < p-lock. Iteração read-only: notas chegam ao scheduler depois (intocadas).
    // MIDI learn: só controller event liga (AC-6); note-on enquanto armado é ignorado.
    {
        const int armed = cc_learn_.learn_arm.load(std::memory_order_acquire);
        bool captured = false; // latch: captura só o 1° controller neste bloco enquanto armado
        for (const auto meta : midi_messages) {
            const auto& msg = meta.getMessage();
            if (!msg.isController())
                continue;
            const int cc = msg.getControllerNumber();
            const int ch = msg.getChannel();
            const int val = msg.getControllerValue();

            if (armed != static_cast<int>(PLockParam::count) && !captured) {
                // Reusa binding existente com mesmo target (rebind), senão appende
                const PLockParam tgt = static_cast<PLockParam>(armed);
                int slot = -1;
                for (int j = 0; j < cc_learn_.count; ++j) {
                    if (cc_learn_.bindings[static_cast<size_t>(j)].target == tgt) {
                        slot = j;
                        break;
                    }
                }
                if (slot < 0 && cc_learn_.count < CcLearnMap::k_max)
                    slot = cc_learn_.count++;
                // Silently drops if full (no overflow — array bounded, RT-safe)
                if (slot >= 0) {
                    cc_learn_.set_binding(slot, {static_cast<uint8_t>(cc), static_cast<uint8_t>(ch), tgt});
                }
                cc_learn_.learn_arm.store(static_cast<int>(PLockParam::count),
                                          std::memory_order_release); // disarm
                captured = true;
                // O CC recém-ligado também aplica este bloco (fall-through)
            }

            apply_cc_to_fx(cc, ch, val, fx_params);
        }
    }

    apply_plock_batch(plock_batch, fx_params);
    block_start_sample_ += static_cast<int64_t>(num_frames);

    // CC out (Fase 9-03): emite p-locks como CC MIDI no canal EXT para drive do hardware.
    // Posição: após apply_plock_batch (batch preenchido), antes de stop-flush/clock (invariante 09-02).
    // Offset 0 (block-granular; PLockEvent sem sample offset — sub-bloco CC adiado para v1.x).
    if (cc_out_.enabled) {
        for (int i = 0; i < plock_batch.count; ++i) {
            const PLockParam param = plock_batch.events[i].param;
            const auto idx = static_cast<size_t>(param);
            if (cc_out_.cc_enabled[idx]) {
                const int val = juce::jlimit(
                    0, 127, juce::roundToInt(plock_param_norm(param, plock_batch.events[i].value) * 127.0f));
                midi_buffer_ext_.addEvent(
                    juce::MidiMessage::controllerEvent(cc_out_.channel_of(), cc_out_.cc_number[idx], val), 0);
            }
        }
    }

    // Despacha sequenciador e MIDI externo — duas chamadas separadas (sem merge = sem alloc)
    // Roteamento nota → pad dentro do Scheduler (pad vazio = ignora em silêncio)
    scheduler_.process(midi_buffer_seq_, voice_pool_, pad_bank_, num_frames);
    scheduler_.process(midi_messages, voice_pool_, pad_bank_, num_frames);

    // Processa o pool de vozes para os buffers temporários pré-alocados
    voice_pool_.process_all(mix_left_.data(), mix_right_.data(), num_frames);

    // Aplica ganho suavizado e mistura no buffer de saída
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

    for (int i = 0; i < num_frames; ++i) {
        const float gain = gain_smoother_.getNextValue();
        left[i] += mix_left_[i] * gain;
        right[i] += mix_right_[i] * gain;
    }

    fx_chain_.process(buffer, fx_params);

    // Scatter é a última etapa de performance — opera no mix wet completo (incl. cauda de
    // reverb/delay), estilo TR-8. CLAMP a [0, k_num_types]: um p-lock pode empurrar scatter_type
    // p/ fora de [0,10] → sem clamp, ScatterEngine dispara jassert em debug. bpm de transport_
    // (sem playhead mantém default 120.0).
    const int scatter_type = juce::jlimit(0, ScatterEngine::k_num_types, juce::roundToInt(fx_params.scatter_type));
    scatter_.process(buffer, scatter_type, fx_params.scatter_depth, transport_.bpm);

    // Perf FX pós-scatter (Fase 8-03): gater → tape_stop (tape_stop por último, freio mestre).
    // clamp [0,1] defensivo (p-lock pode sair de range).
    gater_.process(buffer, juce::jlimit(0.0f, 1.0f, fx_params.gate_depth), transport_.bpm);
    tape_stop_.process(buffer, juce::jlimit(0.0f, 1.0f, fx_params.tape_stop));

    // STOP-FLUSH (auditoria 09-01 M1): na borda toca→para, generate() não emite mais note-offs;
    // sem isto o hardware externo seguraria notas. Emite All Notes Off (CC123) por canal EXT/BOTH
    // único. Canais dedup via flags locais (sem alocação).
    if (was_playing_ && !transport_.is_playing) {
        std::array<bool, 17> ch_done{}; // índices 1..16
        for (int lane = 0; lane < StepPattern::k_num_lanes; ++lane) {
            const LaneMode m = lane_routing_.mode[static_cast<size_t>(lane)];
            if (m == LaneMode::external || m == LaneMode::both) {
                const int ch = lane_routing_.channel_of(lane);
                if (!ch_done[static_cast<size_t>(ch)]) {
                    ch_done[static_cast<size_t>(ch)] = true;
                    midi_buffer_ext_.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
                }
            }
        }
    }
    was_playing_ = transport_.is_playing;

    // MIDI clock master (Fase 9-02): emite 0xF8/start/stop/continue no mesmo buffer EXT.
    // Ordem: notas EXT → stop-flush → clock → clear+addEvents (invariante 09-02).
    midi_clock_.process(
        midi_buffer_ext_, transport_.bpm, transport_.ppq_position, num_frames, transport_.is_playing, clock_master_);

    // Saída MIDI das lanes EXT (Fase 9-01): MIDI-in do host já foi consumido acima
    // (scheduler_.process(midi_messages,...)). Substitui o buffer de saída pelos eventos gerados.
    midi_messages.clear();
    midi_messages.addEvents(midi_buffer_ext_, 0, num_frames, 0);

    // Publica snapshot para a message thread (AC-4, Fase 10-01).
    // Campos relaxed: coerência por-campo é suficiente; tearing entre campos aceito por design.
    {
        const double ppq_in = std::fmod(transport_.ppq_position, StepClock::k_ppq_per_pattern);
        const auto snap_step = static_cast<int32_t>(
            transport_.is_playing ? std::min(static_cast<int>(ppq_in / StepClock::k_ppq_per_step), 15) : -1);
        ui_snapshot_.current_step.store(snap_step, std::memory_order_relaxed);
        ui_snapshot_.is_playing.store(transport_.is_playing, std::memory_order_relaxed);
        ui_snapshot_.bpm.store(static_cast<float>(transport_.bpm), std::memory_order_relaxed);
        ui_snapshot_.master_peak_l.store(buffer.getMagnitude(0, 0, num_frames), std::memory_order_relaxed);
        const float peak_r = buffer.getNumChannels() > 1 ? buffer.getMagnitude(1, 0, num_frames)
                                                         : ui_snapshot_.master_peak_l.load(std::memory_order_relaxed);
        ui_snapshot_.master_peak_r.store(peak_r, std::memory_order_relaxed);
        // lane_last_velocity: derivado de midi_buffer_seq_ (SR1 — sem plumbing no Sequencer).
        // Mute/fill-gate já aplicados: steps suprimidos não emitem note-on.
        for (const auto meta : midi_buffer_seq_) {
            const auto& msg = meta.getMessage();
            if (!msg.isNoteOn())
                continue;
            const int lane = msg.getNoteNumber() - PadBank::k_base_note;
            if (lane >= 0 && lane < 16)
                ui_snapshot_.lane_last_velocity[static_cast<std::size_t>(lane)].store(
                    static_cast<uint8_t>(msg.getVelocity()), std::memory_order_relaxed);
        }
    }
}

void BaqueProcessor::apply_plock_batch(const PLockBatch& batch, FxParams& fx) noexcept {
    for (int i = 0; i < batch.count; ++i) {
        switch (batch.events[i].param) {
        case PLockParam::filter_cutoff:
            fx.filter_cutoff = batch.events[i].value;
            break;
        case PLockParam::filter_res:
            fx.filter_res = batch.events[i].value;
            break;
        case PLockParam::reverb_mix:
            fx.reverb_mix = batch.events[i].value;
            break;
        case PLockParam::delay_mix:
            fx.delay_mix = batch.events[i].value;
            break;
        case PLockParam::delay_time:
            fx.delay_time = batch.events[i].value;
            break;
        case PLockParam::sidechain_threshold:
            fx.sidechain_threshold = batch.events[i].value;
            break;
        case PLockParam::bit_depth:
            fx.bit_depth = batch.events[i].value;
            break;
        case PLockParam::sr_factor:
            fx.sr_factor = batch.events[i].value;
            break;
        case PLockParam::granular_spray:
            fx.granular_spray = batch.events[i].value;
            break;
        case PLockParam::granular_pitch_spread:
            fx.granular_pitch_spread = batch.events[i].value;
            break;
        case PLockParam::granular_freeze:
            fx.granular_freeze = batch.events[i].value;
            break;
        case PLockParam::scatter_type:
            fx.scatter_type = batch.events[i].value;
            break;
        case PLockParam::scatter_depth:
            fx.scatter_depth = batch.events[i].value;
            break;
        case PLockParam::tape_stop:
            fx.tape_stop = batch.events[i].value;
            break;
        case PLockParam::gate_depth:
            fx.gate_depth = batch.events[i].value;
            break;
        // INVARIANT: every PLockParam value must have a case above.
        // WARNING: adding a new PLockParam enum value without adding a case here will silently
        // swallow p-lock events for that param. This default exists only as a safety net.
        default:
            break;
        }
    }
}

// Aplica CC inbound ao FxParams via bindings de cc_learn_ (AC-3).
// Cobre todos os 15 PLockParams (SR1: sem campo ignorado silenciosamente).
// switch default jassert: enum é exaustivo; nunca deve chegar lá.
void BaqueProcessor::apply_cc_to_fx(int cc, int channel, int value, FxParams& fx) noexcept {
    for (int i = 0; i < cc_learn_.count; ++i) {
        const auto& b = cc_learn_.bindings[static_cast<size_t>(i)];
        if (b.channel == 0 || b.target == PLockParam::count)
            continue;
        if (static_cast<int>(b.cc) != cc || static_cast<int>(b.channel) != channel)
            continue;
        const float denorm = plock_param_denorm(b.target, static_cast<float>(value) / 127.0f);
        switch (b.target) {
        case PLockParam::filter_cutoff:
            fx.filter_cutoff = denorm;
            break;
        case PLockParam::filter_res:
            fx.filter_res = denorm;
            break;
        case PLockParam::reverb_mix:
            fx.reverb_mix = denorm;
            break;
        case PLockParam::delay_mix:
            fx.delay_mix = denorm;
            break;
        case PLockParam::delay_time:
            fx.delay_time = denorm;
            break;
        case PLockParam::sidechain_threshold:
            fx.sidechain_threshold = denorm;
            break;
        case PLockParam::bit_depth:
            fx.bit_depth = denorm;
            break;
        case PLockParam::sr_factor:
            fx.sr_factor = denorm;
            break;
        case PLockParam::granular_spray:
            fx.granular_spray = denorm;
            break;
        case PLockParam::granular_pitch_spread:
            fx.granular_pitch_spread = denorm;
            break;
        case PLockParam::granular_freeze:
            fx.granular_freeze = denorm;
            break;
        case PLockParam::scatter_type:
            fx.scatter_type = denorm;
            break;
        case PLockParam::scatter_depth:
            fx.scatter_depth = denorm;
            break;
        case PLockParam::tape_stop:
            fx.tape_stop = denorm;
            break;
        case PLockParam::gate_depth:
            fx.gate_depth = denorm;
            break;
        default:
            jassertfalse; // PLockParam::count ou desconhecido — inalcançável
            break;
        }
    }
}

void BaqueProcessor::apply_hardware_template(const HardwareTemplate& t) noexcept {
    // Copia o padrão ativo (preserva ativações/trigs/p-locks — audit 09-04 SR1); aplica o template
    // (reseta routing+cc_out, reescreve só as notas das lanes mapeadas); re-instala imediato.
    // NÃO partir de um StepPattern default — apagaria o beat do usuário (perda de dados).
    StepPattern p = sequencer_.pattern();
    apply_template(t, lane_routing_, cc_out_, p);
    sequencer_.set_pattern(p);
}

juce::AudioProcessorEditor* BaqueProcessor::createEditor() {
    return new BaqueEditor(*this);
}

void BaqueProcessor::getStateInformation(juce::MemoryBlock& dest_data) {
    // M1 (audit 10-01): suspend audio + pre-drain queue para (a) eliminar data race com
    // o audio thread nos structs agora owned por ele, e (b) garantir que qualquer comando
    // pushed antes desta chamada está aplicado no estado que vamos serializar.
    // drain() roda na message thread dentro do bracket — role-transfer documentado em
    // UiCommandQueue (sem consumer concorrente com áudio suspenso).
    suspendProcessing(true);
    ui_commands_.drain([this](const UiCommand& cmd) { dispatch_ui_command(cmd); });

    auto state = apvts_.copyState();
    state.setProperty("version", k_state_version, nullptr);

    // state v4: lane_routing_, clock_master_, cc_out_ (Fase 10-01 — AC-5/UI8).
    state.setProperty("clock_master", clock_master_ ? 1 : 0, nullptr);

    juce::ValueTree routing("routing");
    for (int i = 0; i < StepPattern::k_num_lanes; ++i) {
        routing.setProperty(
            "mode" + juce::String(i), static_cast<int>(lane_routing_.mode[static_cast<size_t>(i)]), nullptr);
        routing.setProperty(
            "ch" + juce::String(i), static_cast<int>(lane_routing_.channel[static_cast<size_t>(i)]), nullptr);
    }
    state.addChild(routing, -1, nullptr);

    juce::ValueTree cc_out("cc_out_v4");
    cc_out.setProperty("enabled", cc_out_.enabled ? 1 : 0, nullptr);
    cc_out.setProperty("channel", static_cast<int>(cc_out_.channel), nullptr);
    for (int i = 0; i < k_plock_param_count; ++i) {
        cc_out.setProperty("en" + juce::String(i), cc_out_.cc_enabled[static_cast<size_t>(i)] ? 1 : 0, nullptr);
        cc_out.setProperty(
            "cc" + juce::String(i), static_cast<int>(cc_out_.cc_number[static_cast<size_t>(i)]), nullptr);
    }
    state.addChild(cc_out, -1, nullptr);

    // Persiste cc_learn_ (Fase 9-03, state v3): bindings inbound CC → PLockParam.
    // learn_arm NÃO é persistido (transiente; atômico — M1).
    juce::ValueTree cc_tree("cc_learn");
    cc_tree.setProperty("count", cc_learn_.count, nullptr);
    for (int i = 0; i < cc_learn_.count; ++i) {
        const auto& b = cc_learn_.bindings[static_cast<size_t>(i)];
        juce::ValueTree child("binding");
        child.setProperty("cc", static_cast<int>(b.cc), nullptr);
        child.setProperty("channel", static_cast<int>(b.channel), nullptr);
        child.setProperty("target", static_cast<int>(b.target), nullptr);
        cc_tree.addChild(child, -1, nullptr);
    }
    state.addChild(cc_tree, -1, nullptr);

    juce::MemoryOutputStream stream(dest_data, true);
    state.writeToStream(stream);

    suspendProcessing(false);
}

void BaqueProcessor::setStateInformation(const void* data, int size_in_bytes) {
    // M1 (audit 10-01): mesmo bracket que getStateInformation — estado escrito aqui deve
    // ser coerente; drain garante que comandos pushed antes desta chamada não conflitem com
    // a escrita direta dos structs que fazemos abaixo.
    suspendProcessing(true);
    ui_commands_.drain([this](const UiCommand& cmd) { dispatch_ui_command(cmd); });

    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(size_in_bytes));
    if (!state.isValid()) {
        suspendProcessing(false);
        return;
    }

    if (state.hasType(apvts_.state.getType()))
        apvts_.replaceState(state);

    // Restaura state v4: lane_routing_, clock_master_, cc_out_.
    // Ausente em estado pre-v4 → defaults (compatibilidade retroativa).
    clock_master_ = static_cast<int>(state.getProperty("clock_master", 0)) != 0;

    const auto routing = state.getChildWithName("routing");
    if (routing.isValid()) {
        for (int i = 0; i < StepPattern::k_num_lanes; ++i) {
            const int m = juce::jlimit(0, 2, static_cast<int>(routing.getProperty("mode" + juce::String(i), 0)));
            lane_routing_.mode[static_cast<size_t>(i)] = static_cast<LaneMode>(m);
            const int ch = juce::jlimit(0, 16, static_cast<int>(routing.getProperty("ch" + juce::String(i), 0)));
            lane_routing_.channel[static_cast<size_t>(i)] = static_cast<uint8_t>(ch);
        }
    }

    const auto cc_out_tree = state.getChildWithName("cc_out_v4");
    if (cc_out_tree.isValid()) {
        cc_out_.enabled = static_cast<int>(cc_out_tree.getProperty("enabled", 0)) != 0;
        cc_out_.channel =
            static_cast<uint8_t>(juce::jlimit(1, 16, static_cast<int>(cc_out_tree.getProperty("channel", 1))));
        for (int i = 0; i < k_plock_param_count; ++i) {
            cc_out_.cc_enabled[static_cast<size_t>(i)] =
                static_cast<int>(cc_out_tree.getProperty("en" + juce::String(i), 0)) != 0;
            cc_out_.cc_number[static_cast<size_t>(i)] = static_cast<uint8_t>(
                juce::jlimit(0, 127, static_cast<int>(cc_out_tree.getProperty("cc" + juce::String(i), 0))));
        }
    }

    // Restaura cc_learn_ (Fase 9-03, state v3).
    // Ausente em estado pre-v3 → mapa vazio, sem erro (compatibilidade retroativa).
    // learn_arm sempre resetado: nunca persistir estado de arm (já garantido por clear()).
    cc_learn_.clear();
    const auto cc_tree = state.getChildWithName("cc_learn");
    if (cc_tree.isValid()) {
        const int saved_count = juce::jlimit(0, CcLearnMap::k_max, static_cast<int>(cc_tree.getProperty("count", 0)));
        int idx = 0;
        for (int i = 0; i < cc_tree.getNumChildren() && idx < saved_count; ++i) {
            const auto child = cc_tree.getChild(i);
            if (!child.hasType("binding"))
                continue;
            const int cc = static_cast<int>(child.getProperty("cc", 0));
            const int ch = static_cast<int>(child.getProperty("channel", 0));
            const int tgt = static_cast<int>(child.getProperty("target", static_cast<int>(PLockParam::count)));
            // Descarta binding fora de faixa (estado corrompido/hostil — AC-5)
            if (cc < 0 || cc > 127)
                continue;
            if (ch < 1 || ch > 16)
                continue;
            if (tgt < 0 || tgt >= k_plock_param_count)
                continue;
            cc_learn_.set_binding(idx,
                                  {static_cast<uint8_t>(cc), static_cast<uint8_t>(ch), static_cast<PLockParam>(tgt)});
            cc_learn_.count = ++idx;
        }
    }

    suspendProcessing(false);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BaqueProcessor();
}
