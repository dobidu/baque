#include "midi_clock.h"

#include <cmath>

void MidiClock::prepare(double sample_rate) noexcept {
    sample_rate_ = sample_rate;
    reset();
}

void MidiClock::reset() noexcept {
    was_playing_ = false;
    last_tick_ = -1;
}

void MidiClock::process(
    juce::MidiBuffer& out, double bpm, double ppq_start, int block_size, bool is_playing, bool master) noexcept {
    const double safe_bpm = bpm >= 1.0 ? bpm : 1.0;
    const double samples_per_ppq = sample_rate_ * 60.0 / safe_bpm;

    // REGRESSÃO DE PPQ (M1): loop/restart do DAW move ppq para trás → resync sem re-emitir
    // o histórico. Espelha o guard de regressão do Sequencer (Fase 5).
    if (last_tick_ >= 0) {
        // ppq implicado pelo próximo tick que emitiríamos sem o guard
        const double implied_ppq = static_cast<double>(last_tick_ + 1) / k_ppqn;
        if (ppq_start < implied_ppq - (1.0 / k_ppqn)) {
            last_tick_ = static_cast<int64_t>(std::ceil(ppq_start * k_ppqn)) - 1;
            if (last_tick_ < -1)
                last_tick_ = -1;
        }
    }

    if (!master) {
        // Sem emissão; was_playing_=false evita start espúrio quando master ligar.
        was_playing_ = false;
        return;
    }

    // Bordas de transporte (SR2: emitidas no offset 0 do bloco; sub-bloco exato em v1.x)
    if (is_playing && !was_playing_) {
        if (ppq_start <= 0.01)
            out.addEvent(juce::MidiMessage::midiStart(), 0);
        else
            out.addEvent(juce::MidiMessage::midiContinue(), 0);
    } else if (!is_playing && was_playing_) {
        out.addEvent(juce::MidiMessage::midiStop(), 0);
    }

    // 0xF8 — monotônico por last_tick_ (M1): garante sem dup/perda na fronteira de bloco
    if (is_playing) {
        const double ppq_end = ppq_start + static_cast<double>(block_size) / samples_per_ppq;
        // Primeiro tick candidato: nunca voltar atrás de last_tick_, nunca antes de ppq_start
        const auto first_tick = std::max(last_tick_ + 1, static_cast<int64_t>(std::ceil(ppq_start * k_ppqn - 1e-9)));
        for (int64_t n = first_tick; static_cast<double>(n) / k_ppqn < ppq_end; ++n) {
            const double tick_ppq = static_cast<double>(n) / k_ppqn;
            const int pos = juce::jlimit(
                0, block_size - 1, static_cast<int>(std::lround((tick_ppq - ppq_start) * samples_per_ppq)));
            out.addEvent(juce::MidiMessage::midiClock(), pos);
            last_tick_ = n;
        }
    }

    was_playing_ = is_playing;
}
