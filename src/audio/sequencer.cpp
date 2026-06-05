#include "sequencer.h"

#include <algorithm>
#include <cmath>

void Sequencer::set_pattern(const StepPattern& p) noexcept {
    pattern_ = p;
    pattern_pending_.store(false, std::memory_order_relaxed);
}

void Sequencer::set_next_pattern(const StepPattern& p) noexcept {
    // Escreve next_pattern_ ANTES do store com release — garante visibilidade no audio thread.
    next_pattern_ = p;
    pattern_pending_.store(true, std::memory_order_release);
}

void Sequencer::generate(const TransportState& transport,
                         juce::MidiBuffer& midi_out,
                         int block_size,
                         double sample_rate,
                         const FeelPattern* feel,
                         FeelEngine* feel_engine,
                         int64_t block_start_sample) noexcept {
    // Detecção de regressão de ppq (loop/restart do DAW) — limpa fila de notas diferidas. (M3)
    if (feel_engine && transport.is_playing && last_ppq_ > 0.0 && transport.ppq_position < last_ppq_ - 1e-6) {
        feel_engine->prepare();
    }
    if (transport.is_playing)
        last_ppq_ = transport.ppq_position;

    if (!transport.is_playing)
        return;

    // Flush de notas diferidas do bloco anterior — ANTES de gerar novas notas.
    if (feel && feel->enabled && feel_engine)
        feel_engine->flush_deferred(midi_out, block_start_sample, block_size);

    const double ppq_at_block_end =
        transport.ppq_position + (static_cast<double>(block_size) / sample_rate) * (transport.bpm / 60.0);

    const double cycle_start =
        std::floor(transport.ppq_position / StepClock::k_ppq_per_pattern) * StepClock::k_ppq_per_pattern;

    // Lambda de adição com offset de feel (no-op se feel desativado). Definido uma vez por bloco.
    auto add_with_feel = [&](const juce::MidiMessage& msg, int base_pos, int feel_step) {
        if (!feel || !feel->enabled || !feel_engine) {
            midi_out.addEvent(msg, base_pos);
            return;
        }
        const int offset = FeelEngine::timing_ms_to_samples(feel->steps[feel_step].timing_ms, sample_rate);
        const int64_t abs_target = block_start_sample + static_cast<int64_t>(base_pos) + offset;
        if (abs_target < block_start_sample) {
            midi_out.addEvent(msg, 0); // offset negativo: clamp ao início do bloco (v1)
        } else if (abs_target >= block_start_sample + static_cast<int64_t>(block_size)) {
            feel_engine->defer(msg, abs_target); // offset positivo além do bloco: diferir
        } else {
            midi_out.addEvent(msg, static_cast<int>(abs_target - block_start_sample));
        }
    };

    for (int step = 0; step < StepPattern::k_num_steps; ++step) {
        double step_ppq = cycle_start + step * StepClock::k_ppq_per_step;

        if (step_ppq < transport.ppq_position - 1e-9)
            step_ppq += StepClock::k_ppq_per_pattern;

        if (step_ppq >= ppq_at_block_end)
            continue;

        if (step == last_step_fired_)
            continue;

        // Troca de padrão na transição 15→0 (bar boundary)
        if (step == 0 && last_step_fired_ == 15) {
            if (pattern_pending_.load(std::memory_order_acquire)) {
                pattern_ = next_pattern_;
                pattern_pending_.store(false, std::memory_order_relaxed);
            }
        }

        last_step_fired_ = step;

        // Sample position com swing aplicado pelo StepClock (steps ímpares atrasados)
        const int sample_pos =
            clock_.sample_offset_of_step_start(transport.ppq_position, transport.bpm, sample_rate, step);
        const int clamped_pos = (sample_pos >= 0 && sample_pos < block_size) ? sample_pos : 0;

        const int prev_step = (step + StepPattern::k_num_steps - 1) % StepPattern::k_num_steps;

        for (int lane = 0; lane < StepPattern::k_num_lanes; ++lane) {
            // Note-off via NoteTracker — usa nota rastreada; fallback para nota do padrão
            // (NoteTracker = 0 no primeiro bloco — fallback evita nota presa no início)
            if (pattern_.is_active(lane, prev_step)) {
                const uint8_t tracked = note_tracker_.get_active_note(lane);
                const uint8_t prev_note = (tracked != 0) ? tracked : pattern_.get_note(lane, prev_step);
                add_with_feel(juce::MidiMessage::noteOff(1, prev_note), clamped_pos, prev_step);
            }

            // Note-on: registra no NoteTracker; aplica vel_scale se feel ativo
            if (pattern_.is_active(lane, step)) {
                const uint8_t note = pattern_.get_note(lane, step);
                note_tracker_.note_triggered(lane, note);
                uint8_t vel = 100;
                if (feel && feel->enabled) {
                    const float scaled = std::clamp(100.0f * feel->steps[step].vel_scale, 1.0f, 127.0f);
                    vel = static_cast<uint8_t>(std::round(scaled));
                }
                add_with_feel(juce::MidiMessage::noteOn(1, note, vel), clamped_pos, step);
            }
        }
    }
}
