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
                         int64_t block_start_sample,
                         const PLockPattern* plock_pattern,
                         PLockBatch* plock_batch_out,
                         const PerfState* perf,
                         const LaneRouting* routing,
                         juce::MidiBuffer* midi_ext) noexcept {
    // any_solo: 1×/bloco (não por lane). Se alguma lane está soloed, só soloed são audíveis.
    const bool any_solo =
        perf != nullptr && std::any_of(perf->solo.begin(), perf->solo.end(), [](bool b) { return b; });
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
        int offset = FeelEngine::timing_ms_to_samples(feel->steps[feel_step].timing_ms, sample_rate);
        // Gaussian humanize: note-on only — note-off uses deterministic offset of its step.
        // INVARIANT: vel jitter (applied before this call) consumes PRNG first; timing second.
        // Reversing this order silently breaks seed reproducibility for saved presets.
        if (msg.isNoteOn() && feel->humanize_timing_ms > 0.0f) {
            offset += feel_engine->next_timing_jitter_samples(feel->humanize_timing_ms, sample_rate);
        }
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

        // Emite p-lock events para o step que está disparando.
        if (plock_pattern != nullptr && plock_pattern->enabled && plock_batch_out != nullptr) {
            const auto& pstep = plock_pattern->steps[step];
            for (int pi = 0; pi < k_plock_param_count; ++pi) {
                if (pstep.active[pi])
                    plock_batch_out->push(static_cast<PLockParam>(pi), pstep.values[pi]);
            }
        }

        // Sample position com swing aplicado pelo StepClock (steps ímpares atrasados)
        const int sample_pos =
            clock_.sample_offset_of_step_start(transport.ppq_position, transport.bpm, sample_rate, step);
        const int clamped_pos = (sample_pos >= 0 && sample_pos < block_size) ? sample_pos : 0;

        const int prev_step = (step + StepPattern::k_num_steps - 1) % StepPattern::k_num_steps;

        for (int lane = 0; lane < StepPattern::k_num_lanes; ++lane) {
            // Roteamento por lane (Fase 9-01): internal toca voz, external emite MIDI out, both ambos.
            const LaneMode mode = routing != nullptr ? routing->mode[static_cast<size_t>(lane)] : LaneMode::internal;
            const bool is_int = (mode == LaneMode::internal || mode == LaneMode::both);
            const bool is_ext = (mode == LaneMode::external || mode == LaneMode::both);

            // Note-off via NoteTracker — usa nota rastreada; fallback para nota do padrão
            // (NoteTracker = 0 no primeiro bloco — fallback evita nota presa no início)
            if (pattern_.is_active(lane, prev_step)) {
                // INT: nota rastreada (fallback padrão). EXT: nota do padrão DIRETA (auditoria SR1 —
                // o NoteTracker só é populado no caminho INT; lane external-only teria tracker vazio).
                if (is_int) {
                    const uint8_t tracked = note_tracker_.get_active_note(lane);
                    const uint8_t prev_note = (tracked != 0) ? tracked : pattern_.get_note(lane, prev_step);
                    add_with_feel(juce::MidiMessage::noteOff(1, prev_note), clamped_pos, prev_step);
                }
                if (is_ext && midi_ext != nullptr) {
                    const uint8_t prev_note = pattern_.get_note(lane, prev_step);
                    midi_ext->addEvent(juce::MidiMessage::noteOff(routing->channel_of(lane), prev_note), clamped_pos);
                }
            }

            // Trig condition (fill) + audibilidade (mute/solo) — Fase 8-04.
            // GATE A FIRA INTEIRA como unidade (auditoria M1): note_triggered + note-on só quando
            // dispara de verdade. Step suprimido NÃO toca o NoteTracker (senão o próximo note-off
            // referenciaria nota que nunca soou). Note-off (acima) permanece incondicional.
            const StepPattern::TrigCondition cond = pattern_.get_trig(lane, step);
            const bool trig_ok =
                (cond == StepPattern::TrigCondition::always) ||
                (cond == StepPattern::TrigCondition::fill && perf != nullptr && perf->fill_active) ||
                (cond == StepPattern::TrigCondition::not_fill && (perf == nullptr || !perf->fill_active));
            const bool audible = perf == nullptr || (!perf->mute[static_cast<size_t>(lane)] &&
                                                     (!any_solo || perf->solo[static_cast<size_t>(lane)]));

            // Note-on: registra no NoteTracker; aplica vel_scale + humanize se feel ativo.
            // INVARIANT: vel jitter applied here (before add_with_feel) so PRNG advances
            // in order vel[×2] → timing[×2] per note-on. Must not be reordered.
            if (pattern_.is_active(lane, step) && trig_ok && audible) {
                const uint8_t note = pattern_.get_note(lane, step);
                uint8_t vel = 100;
                if (feel && feel->enabled) {
                    const float scaled = std::clamp(100.0f * feel->steps[step].vel_scale, 1.0f, 127.0f);
                    vel = static_cast<uint8_t>(std::round(scaled));
                    if (feel->humanize_vel_pct > 0.0f && feel_engine) { // feel_engine guard (M1)
                        vel = feel_engine->apply_vel_jitter(vel, feel->humanize_vel_pct);
                    }
                }
                if (is_int) {
                    note_tracker_.note_triggered(lane, note); // tracker só no caminho INT (SR1)
                    add_with_feel(juce::MidiMessage::noteOn(1, note, vel), clamped_pos, step);
                }
                if (is_ext && midi_ext != nullptr) {
                    // EXT: clamped_pos (com swing); humanize/defer do feel não em v1 (refinamento futuro)
                    midi_ext->addEvent(juce::MidiMessage::noteOn(routing->channel_of(lane), note, vel), clamped_pos);
                }
            }
        }
    }
}
