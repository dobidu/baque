#include "scheduler.h"

void Scheduler::process(
    const juce::MidiBuffer& midi, VoicePool& pool, const float* sample_data, int num_samples, int block_size) noexcept {
    // Usa JUCE 8 range-for (MidiBuffer::Iterator está obsoleto desde JUCE 8)
    for (const juce::MidiMessageMetadata& meta : midi) {
        const juce::MidiMessage msg = meta.getMessage();
        const int pos = meta.samplePosition;

        // Garante que o offset está dentro do bloco atual
        const int clamped_pos = (pos >= 0 && pos < block_size) ? pos : 0;

        if (msg.isNoteOn()) {
            const float velocity_gain = msg.getFloatVelocity();
            pool.trigger_at(clamped_pos, sample_data, num_samples, velocity_gain);
        } else if (msg.isNoteOff()) {
            // Na Fase 2 sem mapeamento por nota: chama note_off em todas as vozes ativas
            // O rastreamento por nota vem na Fase 3 (Sequenciador)
            // Por ora: fade-out na primeira voz ativa encontrada
            // (comportamento correto para 1 nota simultânea)
        }
    }
}
