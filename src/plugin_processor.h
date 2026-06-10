#pragma once

#include "audio/feel_engine.h"
#include "audio/feel_pattern.h"
#include "audio/fx_chain.h"
#include "audio/fx_params.h"
#include "audio/gater_processor.h"
#include "audio/hardware_templates.h"
#include "audio/lane_routing.h"
#include "audio/midi_cc.h"
#include "audio/midi_clock.h"
#include "audio/pad_bank.h"
#include "audio/perf_state.h"
#include "audio/plock_pattern.h"
#include "audio/scatter_engine.h"
#include "audio/scheduler.h"
#include "audio/sequencer.h"
#include "audio/tape_stop_processor.h"
#include "audio/transport_state.h"
#include "audio/voice_pool.h"

#include <juce_audio_processors/juce_audio_processors.h>

// Processador principal do plugin BAQUE.
// Fase 3: APVTS + VoicePool + Scheduler + Sequencer (step grid 16×16) + TransportState.
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

    // --- Programas (não usados na Fase 3) ---
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

    // Roteamento por lane INT/EXT/BOTH + canal MIDI (Fase 9-01) — público p/ teste/UI futura
    // (mesmo padrão de apvts_). Single-writer: UI futura → atomics/command queue.
    LaneRouting lane_routing_;

    // Clock master: true → BAQUE clocka hardware externo via MIDI out (Fase 9-02).
    // Público (como lane_routing_); UI/automação futura. Single-writer.
    bool clock_master_ = false;

    // P-lock pattern — per-step FX automation (Fase 6-01).
    // Público para testes e binding futuro de UI. Single-writer.
    PLockPattern plock_pattern_;

    // CC out: emite p-locks como CC MIDI no canal EXT (Fase 9-03).
    // Público, single-writer (como lane_routing_). Fase 10 (UI) atomiciza se necessário.
    CcOutRouting cc_out_;

    // MIDI learn: inbound CC → PLockParam binding com persistência (Fase 9-03).
    // Threading: learn_arm atômico; bindings owned pela audio thread em v1.
    // Fase 10 lê bindings via snapshot antes de adicionar writer na message thread.
    CcLearnMap cc_learn_;

    // Arma MIDI learn para o target; chamado da message thread.
    // M1: atomic store com release — audio thread captura com acquire.
    void arm_learn(PLockParam target) noexcept {
        cc_learn_.learn_arm.store(static_cast<int>(target), std::memory_order_release);
    }

    // API de padrão para binding de UI futuro (03-02)
    void set_next_pattern(const StepPattern& p) noexcept { sequencer_.set_next_pattern(p); }

    // Define o padrão ativo imediatamente — setup/testes (delegação, como Sequencer::set_pattern).
    // Single-writer; UI live deve usar set_next_pattern (transição de bar).
    void set_pattern(const StepPattern& p) noexcept { sequencer_.set_pattern(p); }

    // Aplica um template de hardware (TR-8/TR-8S, Fase 9-04) numa chamada.
    // RESETA lane_routing_ + cc_out_ para o template; PRESERVA as ativações/trigs/p-locks do
    // padrão ativo (só as notas das lanes mapeadas são reescritas — audit 09-04 SR1).
    // Single-writer (message thread, setup); Fase 10 (UI) atomiciza se necessário.
    void apply_hardware_template(const HardwareTemplate& t) noexcept;

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout create_parameter_layout();
    static void apply_plock_batch(const PLockBatch& batch, FxParams& fx) noexcept;

    VoicePool voice_pool_;
    PadBank pad_bank_;
    bool pad0_loaded_ = false;
    juce::SmoothedValue<float> gain_smoother_;
    std::vector<float> mix_left_;
    std::vector<float> mix_right_;

    Scheduler scheduler_;
    Sequencer sequencer_;

    // Buffer de eventos MIDI do sequenciador (pré-alocado em prepareToPlay)
    juce::MidiBuffer midi_buffer_seq_;

    // Buffer de saída MIDI das lanes EXT (Fase 9-01, pré-alocado); fundido em midi_messages
    juce::MidiBuffer midi_buffer_ext_;
    bool was_playing_ = false; // borda de parada → All Notes Off nos canais EXT (stop-flush)

    TransportState transport_;

    // Feel Engine — micro-timing por step (Fase 5)
    FeelEngine feel_engine_;
    FeelPattern feel_pattern_;       // initially disabled (enabled = false)
    int64_t block_start_sample_ = 0; // acumulado por processBlock; reset em prepareToPlay

    // FX chain — filter/reverb/delay with SmoothedValue (Fase 6-02)
    FxChain fx_chain_;

    // Scatter perf FX — opera no mix wet completo pós-FxChain (Fase 8-02)
    ScatterEngine scatter_;

    // Perf FX pós-scatter (Fase 8-03): gater (gate beat-synced) → tape_stop (freio mestre, último)
    GaterProcessor gater_;
    TapeStopProcessor tape_stop_;

    // Estado de performance do sequenciador (Fase 8-04): fills (trig conditions) + mute/solo.
    // Single-writer (sem UI/APVTS em v1); Fase 10 deve migrar p/ atomics ao adicionar writers.
    PerfState perf_state_;

    // MIDI clock master (Fase 9-02): emite 0xF8 24ppqn + start/stop/continue no midi_buffer_ext_.
    MidiClock midi_clock_;

    void apply_cc_to_fx(int cc, int channel, int value, FxParams& fx) noexcept;

    static constexpr int k_state_version = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueProcessor)
};
