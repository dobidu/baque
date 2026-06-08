#include "gater_processor.h"

#include <cmath>

int GaterProcessor::gate_period_samples(double sample_rate, double bpm) noexcept {
    if (bpm < 1.0)
        bpm = 1.0;
    const double spq = sample_rate * 60.0 / bpm;            // samples por semínima (1/4)
    const int p = static_cast<int>(std::lround(spq / 4.0)); // 1/16 de nota
    return p < 2 ? 2 : p;
}

void GaterProcessor::prepare(double sample_rate, int /*max_block_size*/) noexcept {
    sample_rate_ = sample_rate > 0.0 ? sample_rate : 44100.0;
    fade_len_ = static_cast<int>(std::lround(sample_rate_ * 0.002)); // ~2 ms anti-click
    if (fade_len_ < 1)
        fade_len_ = 1;
    reset();
}

void GaterProcessor::reset() noexcept {
    sample_counter_ = 0;
}

void GaterProcessor::process(juce::AudioBuffer<float>& buffer, float gate_depth, double bpm) noexcept {
    const int num_ch = juce::jmin(2, buffer.getNumChannels());
    if (num_ch <= 0)
        return;
    const int n = buffer.getNumSamples();

    gate_depth = juce::jlimit(0.0f, 1.0f, gate_depth);
    if (gate_depth <= k_eps) {
        // Bypass: avança a fase p/ manter beat-sync ao re-engajar.
        sample_counter_ += n;
        return;
    }

    const int period = gate_period_samples(sample_rate_, bpm);
    const int half = period / 2;
    int fade = fade_len_;
    if (fade > half)
        fade = half; // fade não pode exceder a meia-janela
    const float on_gain = 1.0f;
    const float off_gain = 1.0f - gate_depth;

    float* chL = buffer.getWritePointer(0);
    float* chR = num_ch > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i) {
        const int phase = static_cast<int>(sample_counter_ % period);

        // Ganho compartilhado L/R (coerência estéreo). Duty 50% + fades lineares anti-click.
        float gain;
        if (phase < fade)
            gain = off_gain + (on_gain - off_gain) * (static_cast<float>(phase) / static_cast<float>(fade));
        else if (phase < half)
            gain = on_gain;
        else if (phase < half + fade)
            gain = on_gain + (off_gain - on_gain) * (static_cast<float>(phase - half) / static_cast<float>(fade));
        else
            gain = off_gain;

        chL[i] *= gain;
        if (chR != nullptr)
            chR[i] *= gain;

        ++sample_counter_;
    }
}
