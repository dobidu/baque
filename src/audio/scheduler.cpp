#include "scheduler.h"

void Scheduler::prepare(double sample_rate) noexcept {
    sample_rate_ = sample_rate;
    runtime_.fill({}); // Limpa rr_index e last_voice — logicamente stale após stop/restart
}

void Scheduler::process(const juce::MidiBuffer& midi, VoicePool& pool, const PadBank& bank, int block_size) noexcept {
    // Usa JUCE 8 range-for (MidiBuffer::Iterator está obsoleto desde JUCE 8)
    for (const juce::MidiMessageMetadata& meta : midi) {
        const juce::MidiMessage msg = meta.getMessage();
        const int pos = meta.samplePosition;

        // Garante que o offset está dentro do bloco atual
        const int clamped_pos = (pos >= 0 && pos < block_size) ? pos : 0;

        // Calcula pad_index antecipado para note-on e note-off
        const int pad_index = static_cast<int>(msg.getNoteNumber()) - PadBank::k_base_note;
        if (pad_index < 0 || pad_index >= PadBank::k_num_pads) {
            continue;
        }

        if (msg.isNoteOn()) {
            // Roteamento nota → pad: fora do banco ou pad vazio = ignora em silêncio
            const SamplePad* pad = bank.pad_for_note(static_cast<uint8_t>(msg.getNoteNumber()));
            if (pad == nullptr) {
                continue;
            }

            // Choke: desativa todas as vozes do mesmo grupo ANTES do novo trigger
            if (pad->choke_group != 0) {
                pool.choke_group(pad->choke_group, bank);
            }

            // Mapeamento de velocity fixado: linear (velocity/127) × ganho do pad
            const float velocity_gain = msg.getFloatVelocity() * pad->gain;

            // Seleção de camada + round-robin (RT-safe: sem alocação, máx 8×2 iterações)
            const float* sample_data = pad->data();
            int num_smp = pad->num_samples();

            if (pad->num_layers() > 0) {
                const auto vel = static_cast<uint8_t>(msg.getVelocity());
                PadRuntime& rt = runtime_[static_cast<size_t>(pad_index)];

                // Passo 1: conta camadas que correspondem à velocity
                int match_count = 0;
                for (int li = 0; li < pad->num_layers(); ++li) {
                    const VelocityLayer& lyr = pad->layer_at(li);
                    if (vel >= lyr.vel_lo && vel <= lyr.vel_hi && lyr.has_sample())
                        ++match_count;
                }

                if (match_count > 0) {
                    // Passo 2: seleciona a (rr_index % match_count)-ésima camada correspondente
                    const int pick = static_cast<int>(rt.rr_index % static_cast<uint8_t>(match_count));
                    int seen = 0;
                    for (int li = 0; li < pad->num_layers(); ++li) {
                        const VelocityLayer& lyr = pad->layer_at(li);
                        if (vel >= lyr.vel_lo && vel <= lyr.vel_hi && lyr.has_sample()) {
                            if (seen == pick) {
                                sample_data = lyr.data();
                                num_smp = lyr.num_samples();
                                break;
                            }
                            ++seen;
                        }
                    }
                    ++rt.rr_index; // overflow de uint8_t é seguro; % match_count sempre válido
                }
                // match_count == 0: usa buffer_ base (já atribuído acima)
            }

            SampleVoice* voice = pool.trigger_at(clamped_pos,
                                                 sample_data,
                                                 num_smp,
                                                 velocity_gain,
                                                 pad->playback_rate(),
                                                 pad->reverse,
                                                 pad->pan,
                                                 pad_index,
                                                 pad->adsr,
                                                 pad->play_mode);

            // Rastreia a voz para note-off por nota (guarda ponteiro não-proprietário)
            runtime_[pad_index].last_voice = voice;

        } else if (msg.isNoteOff()) {
            // Roteia note-off para a voz mais recente do pad.
            // Guarda pad_index() para evitar falso note-off após roubo de voz.
            SampleVoice* v = runtime_[pad_index].last_voice;
            if (v != nullptr && v->is_active() && v->pad_index() == pad_index) {
                v->note_off();
            }
        }
    }
}
