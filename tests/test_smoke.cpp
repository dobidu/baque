#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>

// Inicializa o message manager do JUCE para os testes
struct JuceFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

TEST_CASE_METHOD(JuceFixture, "framework wiring", "[smoke]") {
    // Sanidade: o framework de testes esta funcionando
    REQUIRE(1 + 1 == 2);
}

TEST_CASE_METHOD(JuceFixture, "BaqueProcessor processBlock outputs silence", "[smoke][dsp]") {
    // Instancia o processador diretamente (sem necessidade de dispositivo de áudio)
    BaqueProcessor processor;

    // Prepara para reprodução com parâmetros realistas
    const double sample_rate = 44100.0;
    const int block_size = 512;
    processor.prepareToPlay(sample_rate, block_size);

    // Cria buffer estéreo de silêncio
    juce::AudioBuffer<float> buffer(2, block_size);
    buffer.clear();

    // Preenche com ruído para garantir que processBlock limpa de fato
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i, 0.5f);

    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    // Verifica que a saída é silêncio (magnitude máxima = 0)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float magnitude = buffer.getMagnitude(ch, 0, block_size);
        REQUIRE(magnitude == 0.0f);
    }

    processor.releaseResources();
}
