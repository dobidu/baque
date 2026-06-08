#include "tape_stop_processor.h"

#include <cmath>

float TapeStopProcessor::read_ring(const std::vector<float>& ring, int ring_size, double pos) noexcept {
    // Interpolação linear com wrap circular.
    const int i0 = static_cast<int>(std::floor(pos));
    const double frac = pos - static_cast<double>(i0);
    int a = i0 % ring_size;
    if (a < 0)
        a += ring_size;
    int b = a + 1;
    if (b >= ring_size)
        b -= ring_size;
    const float sa = ring[static_cast<size_t>(a)];
    const float sb = ring[static_cast<size_t>(b)];
    return sa + static_cast<float>(frac) * (sb - sa);
}

void TapeStopProcessor::prepare(double sample_rate, int /*max_block_size*/) noexcept {
    sample_rate_ = sample_rate > 0.0 ? sample_rate : 44100.0;
    ring_size_ = static_cast<int>(sample_rate_ * 2.0);
    if (ring_size_ < 2)
        ring_size_ = 2;
    ring_l_.assign(static_cast<size_t>(ring_size_), 0.0f);
    ring_r_.assign(static_cast<size_t>(ring_size_), 0.0f);
    rate_smoother_.reset(sample_rate_, 0.03); // ramp de 30 ms (per-sample) — sem zipper
    reset();
}

void TapeStopProcessor::reset() noexcept {
    std::fill(ring_l_.begin(), ring_l_.end(), 0.0f);
    std::fill(ring_r_.begin(), ring_r_.end(), 0.0f);
    write_pos_ = 0;
    read_pos_ = 0.0;
    rate_smoother_.setCurrentAndTargetValue(1.0f);
    was_active_ = false;
}

void TapeStopProcessor::process(juce::AudioBuffer<float>& buffer, float tape_stop) noexcept {
    if (ring_size_ <= 0)
        return;
    const int num_ch = juce::jmin(2, buffer.getNumChannels());
    if (num_ch <= 0)
        return;
    const int n = buffer.getNumSamples();

    tape_stop = juce::jlimit(0.0f, 1.0f, tape_stop);
    const bool active = tape_stop > k_eps;
    rate_smoother_.setTargetValue(active ? (1.0f - tape_stop) : 1.0f);

    float* chL = buffer.getWritePointer(0);
    float* chR = num_ch > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i) {
        const float dryL = chL[i];
        const float dryR = chR != nullptr ? chR[i] : 0.0f;

        // Captura contínua no ring.
        ring_l_[static_cast<size_t>(write_pos_)] = dryL;
        ring_r_[static_cast<size_t>(write_pos_)] = dryR;

        if (!active) {
            // Bypass exato; mantém read_pos_ sincronizado p/ re-engajar sem salto.
            read_pos_ = static_cast<double>(write_pos_);
            rate_smoother_.getNextValue(); // mantém o smoother andando ao alvo 1.0
            // out = dry (inalterado)
        } else {
            if (!was_active_)
                read_pos_ = static_cast<double>(write_pos_); // engaje: ancora no presente

            const float rate = rate_smoother_.getNextValue();
            // HALT = SILÊNCIO: ganho = rate → rate 0 (full stop) = saída 0; sem DC.
            const float gain = rate;
            const float wetL = read_ring(ring_l_, ring_size_, read_pos_) * gain;
            const float wetR = read_ring(ring_r_, ring_size_, read_pos_) * gain;
            chL[i] = wetL;
            if (chR != nullptr)
                chR[i] = wetR;

            read_pos_ += static_cast<double>(rate);
            if (read_pos_ >= static_cast<double>(ring_size_))
                read_pos_ -= static_cast<double>(ring_size_);
        }

        was_active_ = active;
        if (++write_pos_ >= ring_size_)
            write_pos_ = 0;
    }
}
