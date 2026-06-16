#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <cstdint>
#include <type_traits>

// Tipo de comando UI→engine (Fase 10-01).
// Mapeamento de campos a/b/c/f por tipo documentado inline.
enum class UiCommandType : uint8_t {
    // --- PerfState: fills, mute, solo ---
    set_fill, // c=fill_active(bool)
    set_mute, // a=lane[0,15], c=muted(bool)
    set_solo, // a=lane[0,15], c=solo(bool)
    // --- Roteamento por lane ---
    set_lane_mode,    // a=lane[0,15], c=LaneMode(0=int,1=ext,2=both)
    set_lane_channel, // a=lane[0,15], c=channel[0,16] (0→1 no emit)
    // --- Clock master ---
    set_clock_master, // c=enabled(bool)
    // --- CC out ---
    set_cc_out_enabled, // c=enabled(bool)
    set_cc_out_channel, // c=channel[1,16]
    set_cc_slot,        // a=param_index[0,14], b=cc_number[0,127], c=enabled(bool)
    // --- Parâmetros de pad ---
    set_pad_gain,            // a=pad[0,15], f=gain
    set_pad_pan,             // a=pad[0,15], f=pan[-1,1]
    set_pad_pitch_semitones, // a=pad[0,15], c=semitones
    set_pad_pitch_cents,     // a=pad[0,15], c=cents
    set_pad_reverse,         // a=pad[0,15], c=reverse(bool)
    set_pad_choke_group,     // a=pad[0,15], c=group[0,8]
    set_pad_play_mode,       // a=pad[0,15], c=PlayMode(int)
    set_pad_adsr_attack,     // a=pad[0,15], f=attack_ms
    set_pad_adsr_decay,      // a=pad[0,15], f=decay_ms
    set_pad_adsr_sustain,    // a=pad[0,15], f=sustain[0,1]
    set_pad_adsr_release,    // a=pad[0,15], f=release_ms
    // --- Edição ao vivo do padrão ativo ---
    set_step,          // a=lane[0,15], b=step[0,15], c=on(bool)
    set_step_velocity, // a=lane[0,15], b=step[0,15], c=velocity[0,127]
    set_plock,         // a=lane[0,15], b=step[0,15], c=param_index[0,14], f=value(engine units)
    clear_plock,       // a=lane[0,15], b=step[0,15], c=param_index[0,14]
    // --- Template de hardware ---
    apply_template, // a=template_id(0=TR-8, 1=TR-8S)
};

// Comando POD tagged-union: message thread → audio thread via UiCommandQueue.
// Trivially copyable — copiado por valor na fila; zero ponteiros, zero alocação.
struct UiCommand {
    UiCommandType type{};
    int32_t a = 0; // significado depende do type (ver UiCommandType acima)
    int32_t b = 0;
    int32_t c = 0;
    float f = 0.0f;
};
static_assert(std::is_trivially_copyable_v<UiCommand>);

// Fila SPSC de comandos UI→engine (Fase 10-01).
//
// CONTRATO SINGLE-PRODUCER / SINGLE-CONSUMER:
//   Produtor  : message thread (UI) — chama push().
//   Consumidor: audio thread — chama drain() no topo de processBlock.
//   EXCEÇÃO de consumer-role: drain() pode ser chamado na message thread SOMENTE
//   dentro de um bracket suspendProcessing(true) / suspendProcessing(false)
//   (getStateInformation / setStateInformation). Com áudio suspenso não há
//   consumidor concorrente — sem data race. Documentado em plugin_processor.cpp.
//
// Implementação: juce::AbstractFifo (SPSC lock-free estabelecido na plataforma
// JUCE) + std::array<UiCommand, k_capacity> pré-alocado. Capacidade fixa = 256.
// Zero alocações após a construção.
class UiCommandQueue {
  public:
    static constexpr int k_capacity = 256;

    // AbstractFifo(N) holds at most N-1 items (one slot reserved for empty/full
    // bookkeeping). Allocate k_capacity+1 internally to expose exactly k_capacity
    // usable slots to callers — the stated 256-command capacity is the public contract.
    UiCommandQueue()
        : fifo_(k_capacity + 1) {}

    // Enfileira um comando. Retorna false (descarta sem bloquear) se fila cheia.
    // DEBUG: jassert verifica que estamos na message thread (único produtor SPSC).
    // Usa getInstanceWithoutCreating() para não disparar em contextos de teste
    // sem MessageManager — comportamento de produção preservado integralmente.
    bool push(const UiCommand& cmd) noexcept {
        jassert(!juce::MessageManager::getInstanceWithoutCreating() ||
                juce::MessageManager::existsAndIsCurrentThread());
        int start1, size1, start2, size2;
        fifo_.prepareToWrite(1, start1, size1, start2, size2);
        const int written = size1 + size2;
        if (written == 0)
            return false;
        buf_[static_cast<std::size_t>(size1 > 0 ? start1 : start2)] = cmd;
        fifo_.finishedWrite(written);
        return true;
    }

    // Drena todos os comandos pendentes chamando apply(cmd) para cada um (FIFO).
    // Zero alocações; zero locks. Chamado pelo audio thread no topo de processBlock.
    template <typename Fn> void drain(Fn&& apply) {
        const int available = fifo_.getNumReady();
        if (available == 0)
            return;
        int start1, size1, start2, size2;
        fifo_.prepareToRead(available, start1, size1, start2, size2);
        for (int i = 0; i < size1; ++i)
            apply(buf_[static_cast<std::size_t>(start1 + i)]);
        for (int i = 0; i < size2; ++i)
            apply(buf_[static_cast<std::size_t>(start2 + i)]);
        fifo_.finishedRead(size1 + size2);
    }

  private:
    juce::AbstractFifo fifo_;
    std::array<UiCommand, k_capacity + 1> buf_{}; // +1: AbstractFifo bookkeeping slot

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UiCommandQueue)
};
