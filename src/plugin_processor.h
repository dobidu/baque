#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Processador principal do plugin BAQUE.
// Fase 0: apenas silêncio + estado versionado. DSP real começa na Fase 2.
class BaqueProcessor : public juce::AudioProcessor {
  public:
    BaqueProcessor();
    ~BaqueProcessor() override = default;

    // --- Ciclo de vida de áudio ---
    void prepareToPlay(double sample_rate, int samples_per_block) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) override;

    // --- Layout de barramento ---
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // --- Editor ---
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // --- Informações do plugin ---
    const juce::String getName() const override { return "BAQUE"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // --- Programas (não usados na Fase 0) ---
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    // --- Estado / presets ---
    void getStateInformation(juce::MemoryBlock& dest_data) override;
    void setStateInformation(const void* data, int size_in_bytes) override;

  private:
    // Esquema do ValueTree — incrementar ao adicionar campos incompatíveis
    static constexpr int k_state_version = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueProcessor)
};
