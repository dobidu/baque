#pragma once

#include "audio/scheduler.h"
#include "audio/voice_pool.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Processador principal do plugin BAQUE.
// Fase 2: APVTS + pool de vozes + scheduler sample-accurate + transporte do host.

// Estado de transporte do host — populado por getPlayHead() no audio thread.
struct TransportState {
    bool is_playing = false;
    double bpm = 120.0;
    double ppq_position = 0.0; // posição em quarter notes
    double sample_rate = 44100.0;
};
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
    [[nodiscard]] bool hasEditor() const override { return true; }

    // --- Informações do plugin ---
    [[nodiscard]] const juce::String getName() const override { return "BAQUE"; }
    [[nodiscard]] bool acceptsMidi() const override { return true; }
    [[nodiscard]] bool producesMidi() const override { return true; }
    [[nodiscard]] bool isMidiEffect() const override { return false; }
    [[nodiscard]] double getTailLengthSeconds() const override { return 0.0; }

    // --- Programas (não usados na Fase 2) ---
    [[nodiscard]] int getNumPrograms() override { return 1; }
    [[nodiscard]] int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    [[nodiscard]] const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    // --- Estado / presets ---
    void getStateInformation(juce::MemoryBlock& dest_data) override;
    void setStateInformation(const void* data, int size_in_bytes) override;

    // APVTS — parâmetros automatizáveis expostos ao host
    juce::AudioProcessorValueTreeState apvts_;

  private:
    // Cria o layout de parâmetros do APVTS
    static juce::AudioProcessorValueTreeState::ParameterLayout create_parameter_layout();

    // Pool de vozes pré-alocado — sem new/delete no audio thread
    VoicePool voice_pool_;

    // Buffer de sample decodificado (float, mono)
    juce::AudioBuffer<float> sample_buffer_;
    bool sample_loaded_ = false; // Guarda de decode único (evita realocação em prepareToPlay)

    // Suavização de ganho — elimina ruído de zíper na automação
    juce::SmoothedValue<float> gain_smoother_;

    // Buffers temporários estéreo para mixagem de vozes (pré-alocados em prepareToPlay)
    std::vector<float> mix_left_;
    std::vector<float> mix_right_;

    // Scheduler de eventos MIDI com precisão de sample
    Scheduler scheduler_;

    // Estado de transporte do host (BPM, posição, isPlaying)
    TransportState transport_;

    static constexpr int k_state_version = 2; // Incrementado do v1 (Fase 1)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueProcessor)
};
