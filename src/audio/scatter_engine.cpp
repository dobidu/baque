#include "scatter_engine.h"

#include <cmath>

// Tabela de presets type 1-10. Cobre as 4 primitivas em múltiplas subdivisões.
const ScatterEngine::TypeSpec ScatterEngine::k_types[ScatterEngine::k_num_types] = {
    {Primitive::repeat, 8, 0, false},     // 1: repeat 1/8
    {Primitive::repeat, 16, 0, false},    // 2: repeat 1/16
    {Primitive::repeat, 32, 0, false},    // 3: repeat 1/32
    {Primitive::reverse, 8, 0, false},    // 4: reverse 1/8
    {Primitive::reverse, 16, 0, false},   // 5: reverse 1/16
    {Primitive::gate, 16, 0, false},      // 6: gate 1/16
    {Primitive::gate, 32, 0, false},      // 7: gate 1/32
    {Primitive::decimate, 16, 4, false},  // 8: decimate stride leve
    {Primitive::decimate, 16, 16, false}, // 9: decimate stride forte
    {Primitive::repeat, 32, 0, true},     // 10: repeat 1/32 reverso (stutter agressivo)
};

int ScatterEngine::slice_length_samples(double sample_rate, double bpm, int subdivision) noexcept {
    if (bpm < 1.0)
        bpm = 1.0;
    if (subdivision < 1)
        subdivision = 1;
    const double spq = sample_rate * 60.0 / bpm;                       // samples por semínima (1/4)
    const double slice = spq * 4.0 / static_cast<double>(subdivision); // 1 nota inteira / subdivisão
    const int s = static_cast<int>(std::lround(slice));
    return s < 1 ? 1 : s;
}

void ScatterEngine::prepare(double sample_rate, int /*max_block_size*/) noexcept {
    sample_rate_ = sample_rate > 0.0 ? sample_rate : 44100.0;
    ring_size_ = static_cast<int>(sample_rate_ * 2.0); // janela de captura de 2 s
    if (ring_size_ < 2)
        ring_size_ = 2;

    // Aloca off audio thread. Nenhuma destas chamadas ocorre em process().
    ring_l_.assign(static_cast<size_t>(ring_size_), 0.0f);
    ring_r_.assign(static_cast<size_t>(ring_size_), 0.0f);
    slice_l_.assign(static_cast<size_t>(ring_size_), 0.0f);
    slice_r_.assign(static_cast<size_t>(ring_size_), 0.0f);

    reset();
}

void ScatterEngine::reset() noexcept {
    std::fill(ring_l_.begin(), ring_l_.end(), 0.0f);
    std::fill(ring_r_.begin(), ring_r_.end(), 0.0f);
    std::fill(slice_l_.begin(), slice_l_.end(), 0.0f);
    std::fill(slice_r_.begin(), slice_r_.end(), 0.0f);
    write_pos_ = 0;
    slice_pos_ = 0;
    cur_slice_len_ = 1;
    was_active_ = false;
}

void ScatterEngine::latch_slice(int slice_len) noexcept {
    // Copia os últimos slice_len samples (terminando em write_pos_) para a slice congelada.
    // slice_l_[0] = mais antigo; slice_l_[slice_len-1] = mais recente. Trilha write_pos_ por
    // slice_len → a região congelada não é sobrescrita durante o roll (sem feedback).
    for (int j = 0; j < slice_len; ++j) {
        int src = write_pos_ - (slice_len - 1) + j;
        src = ((src % ring_size_) + ring_size_) % ring_size_;
        slice_l_[static_cast<size_t>(j)] = ring_l_[static_cast<size_t>(src)];
        slice_r_[static_cast<size_t>(j)] = ring_r_[static_cast<size_t>(src)];
    }
    cur_slice_len_ = slice_len;
    slice_pos_ = 0;
}

void ScatterEngine::process(juce::AudioBuffer<float>& buffer, int type, float depth, double bpm) noexcept {
    if (ring_size_ <= 0)
        return; // não preparado

    const int num_ch = juce::jmin(2, buffer.getNumChannels());
    if (num_ch <= 0)
        return;
    const int n = buffer.getNumSamples();

    const bool want_active = (type >= 1 && type <= k_num_types) && depth > 0.0f;
    if (bpm < 1.0)
        bpm = 1.0;
    depth = juce::jlimit(0.0f, 1.0f, depth);

    // Resolve a primitiva e o comprimento de slice do roll ativo (compartilhado entre canais — SR1).
    Primitive prim = Primitive::repeat;
    int stride = 1;
    bool also_rev = false;
    int active_slice_len = cur_slice_len_;
    if (want_active) {
        const TypeSpec& ts = k_types[type - 1];
        prim = ts.prim;
        stride = ts.param > 0 ? ts.param : 1;
        also_rev = ts.also_reverse;
        active_slice_len = slice_length_samples(sample_rate_, bpm, ts.subdivision);
        if (active_slice_len >= ring_size_)
            active_slice_len = ring_size_ - 1; // segurança: slice nunca >= ring (sem overlap)
        if (active_slice_len < 1)
            active_slice_len = 1;
        jassert(active_slice_len < ring_size_);
    }

    float* chL = buffer.getWritePointer(0);
    float* chR = num_ch > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i) {
        const float dryL = chL[i];
        const float dryR = chR != nullptr ? chR[i] : 0.0f;

        // 1) Captura contínua: escreve o sinal seco no ring (sempre, ativo ou não).
        ring_l_[static_cast<size_t>(write_pos_)] = dryL;
        ring_r_[static_cast<size_t>(write_pos_)] = dryR;

        float outL = dryL;
        float outR = dryR;

        if (want_active) {
            // Borda inativo→ativo: congela a slice mais recente (uma vez por roll).
            if (!was_active_)
                latch_slice(active_slice_len);

            const int len = cur_slice_len_;
            const int p = slice_pos_; // posição na slice congelada [0, len)

            // Offset de leitura na slice congelada — compartilhado L/R (coerência estéreo).
            int off = p;
            float gain = 1.0f;
            switch (prim) {
            case Primitive::repeat:
                off = also_rev ? (len - 1 - p) : p;
                break;
            case Primitive::reverse:
                off = len - 1 - p;
                break;
            case Primitive::gate:
                off = p;
                gain = (p < len / 2) ? 1.0f : 0.0f; // duty 50%: chop on/off dentro da slice
                break;
            case Primitive::decimate:
                off = (p / stride) * stride; // sample-and-hold: segura o início de cada stride
                break;
            }
            if (off < 0)
                off = 0;
            if (off >= len)
                off = len - 1;

            const float wetL = slice_l_[static_cast<size_t>(off)] * gain;
            const float wetR = slice_r_[static_cast<size_t>(off)] * gain;

            // Mix wet/dry: depth=0 → seco exato; depth=1 → wet puro.
            outL = dryL * (1.0f - depth) + wetL * depth;
            outR = dryR * (1.0f - depth) + wetR * depth;

            if (++slice_pos_ >= len)
                slice_pos_ = 0;
        }

        chL[i] = outL;
        if (chR != nullptr)
            chR[i] = outR;

        was_active_ = want_active;
        if (++write_pos_ >= ring_size_)
            write_pos_ = 0;
    }
}
