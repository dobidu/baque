#include "scheduler.h"

void Scheduler::process(const juce::MidiBuffer& midi, VoicePool& pool, const PadBank& bank, int block_size) noexcept {
    // Usa JUCE 8 range-for (MidiBuffer::Iterator está obsoleto desde JUCE 8)
    for (const juce::MidiMessageMetadata& meta : midi) {
        const juce::MidiMessage msg = meta.getMessage();
        const int pos = meta.samplePosition;

        // Garante que o offset está dentro do bloco atual
        const int clamped_pos = (pos >= 0 && pos < block_size) ? pos : 0;

        if (msg.isNoteOn()) {
            // Roteamento nota → pad: fora do banco ou pad vazio = ignora em silêncio
            const SamplePad* pad = bank.pad_for_note(static_cast<uint8_t>(msg.getNoteNumber()));
            if (pad == nullptr) {
                continue;
            }

            // Mapeamento de velocity fixado: linear (velocity/127) × ganho do pad
            const float velocity_gain = msg.getFloatVelocity() * pad->gain;
            pool.trigger_at(clamped_pos,
                            pad->data(),
                            pad->num_samples(),
                            velocity_gain,
                            pad->playback_rate(),
                            pad->reverse,
                            pad->pan);
        } else if (msg.isNoteOff()) {
            // Na Fase 2 sem mapeamento por nota: chama note_off em todas as vozes ativas
            // O rastreamento por nota vem na Fase 3 (Sequenciador)
            // Por ora: fade-out na primeira voz ativa encontrada
            // (comportamento correto para 1 nota simultânea)
        }
    }
}
