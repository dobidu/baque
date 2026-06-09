#include "plugin_processor.h"

#include "plugin_editor.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <BinaryData.h>

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

void BaqueProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) {
    juce::ScopedNoDenormals no_denormals;

    const int num_frames = buffer.getNumSamples();
    buffer.clear();

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
    apply_plock_batch(plock_batch, fx_params);
    block_start_sample_ += static_cast<int64_t>(num_frames);

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
    midi_clock_.process(midi_buffer_ext_,
                        transport_.bpm,
                        transport_.ppq_position,
                        num_frames,
                        transport_.is_playing,
                        clock_master_);

    // Saída MIDI das lanes EXT (Fase 9-01): MIDI-in do host já foi consumido acima
    // (scheduler_.process(midi_messages,...)). Substitui o buffer de saída pelos eventos gerados.
    midi_messages.clear();
    midi_messages.addEvents(midi_buffer_ext_, 0, num_frames, 0);
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

juce::AudioProcessorEditor* BaqueProcessor::createEditor() {
    return new BaqueEditor(*this);
}

void BaqueProcessor::getStateInformation(juce::MemoryBlock& dest_data) {
    // Serializa o estado do APVTS + versão do schema
    auto state = apvts_.copyState();
    state.setProperty("version", k_state_version, nullptr);
    juce::MemoryOutputStream stream(dest_data, true);
    state.writeToStream(stream);
}

void BaqueProcessor::setStateInformation(const void* data, int size_in_bytes) {
    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(size_in_bytes));
    if (!state.isValid())
        return;

    // Versão futura: migrar campos incompatíveis aqui se necessário
    if (state.hasType(apvts_.state.getType()))
        apvts_.replaceState(state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BaqueProcessor();
}
