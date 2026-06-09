#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdint>

// Gerador de MIDI timing clock (0xF8, 24 ppqn) como master.
// Emite no buffer EXT das lanes (Fase 9-01); bytes de clock coexistem com notas EXT.
// Slave/PLL deferido para pós-v1 (necessita filtro de jitter).
//
// INVARIANTES (audit 09-02):
//   - 24 clocks por semínima; tick n em ppq n/24; pos = round((n/24 - ppq_start)*sr*60/bpm)
//   - MONOTÔNICO (M1): last_tick_ índice absoluto; só emite n > last_tick_ → sem dup/perda na
//     fronteira de bloco; ressincroniza em regressão de ppq (loop do DAW)
//   - start (ppq~0) / continue (ppq>0) / stop nas bordas; 1× por borda; offset 0 no bloco (SR2)
//   - master=false → silêncio absoluto; was_playing_ rastreia (is_playing&&master) p/ borda correta
class MidiClock {
  public:
    static constexpr int k_ppqn = 24; // clocks por semínima (MIDI spec 0xF8)

    MidiClock() = default;

    // Inicializa sample_rate e reseta estado. Chamar em prepareToPlay.
    void prepare(double sample_rate) noexcept;

    // Gera bytes de clock/transporte no buffer de saída.
    // master=false → zero emissão; last_tick_ ainda ressincroniza em regressão (sem start espúrio
    // ao ligar master no meio de uma sessão).
    void
    process(juce::MidiBuffer& out, double bpm, double ppq_start, int block_size, bool is_playing, bool master) noexcept;

    // Reseta estado. Chamar em releaseResources.
    void reset() noexcept;

  private:
    double sample_rate_ = 44100.0;
    bool was_playing_ = false;
    // Índice absoluto do último 0xF8 emitido — guard monotônico (audit M1).
    int64_t last_tick_ = -1;
};
