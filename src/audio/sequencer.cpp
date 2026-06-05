#include "sequencer.h"

#include <algorithm>
#include <cmath>

void Sequencer::generate(const TransportState& transport,
                         juce::MidiBuffer& midi_out,
                         int block_size,
                         double sample_rate) noexcept {
    if (!transport.is_playing)
        return;

    // PPQ ao final do bloco
    const double ppq_at_block_end =
        transport.ppq_position + (static_cast<double>(block_size) / sample_rate) * (transport.bpm / 60.0);

    // Passo 1: step no início do bloco
    const int step_at_start = clock_.current_step(transport.ppq_position);

    // Passo 2: busca todos os 16 steps possíveis dentro do bloco.
    // Importante: usa 'continue' (não 'break') — outros steps podem estar no bloco
    // mesmo que o step atual tenha feito wrap para o próximo ciclo.
    const double cycle_start =
        std::floor(transport.ppq_position / StepClock::k_ppq_per_pattern) * StepClock::k_ppq_per_pattern;

    for (int step = 0; step < StepPattern::k_num_steps; ++step) {
        // PPQ onde este step começa
        double step_ppq = cycle_start + step * StepClock::k_ppq_per_step;

        // Ajuste de wrap-around se o step já passou neste ciclo
        if (step_ppq < transport.ppq_position - 1e-9)
            step_ppq += StepClock::k_ppq_per_pattern;

        // Este step não ocorre neste bloco — continua para o próximo
        if (step_ppq >= ppq_at_block_end)
            continue;

        // Guarda de duplo-disparo — não dispara o mesmo step duas vezes
        if (step == last_step_fired_)
            continue;

        last_step_fired_ = step;

        // Offset de sample dentro do bloco
        const double delta_ppq = step_ppq - transport.ppq_position;
        const double delta_seconds = delta_ppq * (60.0 / transport.bpm);
        const int sample_pos = static_cast<int>(delta_seconds * sample_rate);
        const int clamped_pos = (sample_pos >= 0 && sample_pos < block_size) ? sample_pos : 0;

        // Step anterior para note-off (evita notas presas)
        const int prev_step = (step + StepPattern::k_num_steps - 1) % StepPattern::k_num_steps;

        for (int lane = 0; lane < StepPattern::k_num_lanes; ++lane) {
            // Note-off para o step anterior (se estava ativo)
            if (pattern_.is_active(lane, prev_step)) {
                const uint8_t prev_note = pattern_.get_note(lane, prev_step);
                midi_out.addEvent(juce::MidiMessage::noteOff(1, prev_note), clamped_pos);
            }

            // Note-on para o step atual (se estiver ativo)
            if (pattern_.is_active(lane, step)) {
                const uint8_t note = pattern_.get_note(lane, step);
                midi_out.addEvent(juce::MidiMessage::noteOn(1, note, static_cast<uint8_t>(100)), clamped_pos);
            }
        }
    }
}
