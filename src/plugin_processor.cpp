#include "plugin_processor.h"

#include "plugin_editor.h"

BaqueProcessor::BaqueProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void BaqueProcessor::prepareToPlay(double /*sample_rate*/, int /*samples_per_block*/) {
    // Fase 0: sem inicialização de DSP necessária
}

void BaqueProcessor::releaseResources() {
    // Fase 0: sem recursos para liberar
}

bool BaqueProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Aceita apenas estéreo ou mono em ambos os lados
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void BaqueProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi_messages*/) {
    // Fase 0: silêncio — limpa o buffer de saída
    juce::ScopedNoDenormals no_denormals;
    buffer.clear();
}

juce::AudioProcessorEditor* BaqueProcessor::createEditor() {
    return new BaqueEditor(*this);
}

void BaqueProcessor::getStateInformation(juce::MemoryBlock& dest_data) {
    // Serializa estado versionado — campo "version" permite migração futura (ESCOPO §6)
    juce::ValueTree state("BAQUE");
    state.setProperty("version", k_state_version, nullptr);
    juce::MemoryOutputStream stream(dest_data, true);
    state.writeToStream(stream);
}

void BaqueProcessor::setStateInformation(const void* data, int size_in_bytes) {
    // Tolera estado ausente ou de versão anterior
    auto state = juce::ValueTree::readFromData(data, static_cast<size_t>(size_in_bytes));
    if (!state.isValid() || state.getType() != juce::Identifier("BAQUE"))
        return;

    // Versão futura: migrar campos aqui se necessário
    // int loaded_version = state.getProperty("version", 0);
}

// Ponto de entrada exigido pelo JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BaqueProcessor();
}
