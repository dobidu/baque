#include "sequencer.h"

#include <cmath>

void Sequencer::set_next_pattern(const StepPattern& p) noexcept {
    // Escreve next_pattern_ ANTES do store com release — garante visibilidade no audio thread.
    next_pattern_ = p;
    pattern_pending_.store(true, std::memory_order_release);
}

void Sequencer::generate(const TransportState& transport,
                         juce::MidiBuffer& midi_out,
                         int block_size,
                         double sample_rate) noexcept {
    if (!transport.is_playing)
        return;

    const double ppq_at_block_end =
        transport.ppq_position + (static_cast<double>(block_size) / sample_rate) * (transport.bpm / 60.0);

    const double cycle_start =
        std::floor(transport.ppq_position / StepClock::k_ppq_per_pattern) * StepClock::k_ppq_per_pattern;

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
                midi_out.addEvent(juce::MidiMessage::noteOff(1, prev_note), clamped_pos);
            }

            // Note-on: registra no NoteTracker antes de adicionar ao buffer
            if (pattern_.is_active(lane, step)) {
                const uint8_t note = pattern_.get_note(lane, step);
                note_tracker_.note_triggered(lane, note);
                midi_out.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<uint8_t>(100)), clamped_pos);
            }
        }
    }
}
