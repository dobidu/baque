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

    // Inicializa suavização de ganho (10ms de ramp)
    gain_smoother_.reset(sample_rate, 0.01);
    if (auto* param = apvts_.getRawParameterValue("master_gain"))
        gain_smoother_.setCurrentAndTargetValue(*param);
}

void BaqueProcessor::releaseResources() {
    // Fase 2: sem recursos externos para liberar
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

    // Gera eventos MIDI do sequenciador no buffer pré-alocado (limpa antes de preencher)
    midi_buffer_seq_.clear();
    PLockBatch plock_batch;
    sequencer_.generate(transport_,
                        midi_buffer_seq_,
                        num_frames,
                        getSampleRate(),
                        feel_pattern_.enabled ? &feel_pattern_ : nullptr,
                        feel_pattern_.enabled ? &feel_engine_ : nullptr,
                        block_start_sample_,
                        plock_pattern_.enabled ? &plock_pattern_ : nullptr,
                        &plock_batch);
    apply_plock_batch(plock_batch, fx_params);
    // fx_params pronto para FxChain::process() (Fase 6-02)
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
