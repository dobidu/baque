#include "time_stretch.h"

#include <SoundTouch.h>
#include <algorithm>
#include <cmath>
#include <vector>

void TimeStretch::apply(SamplePad& pad, VoicePool& pool, double sample_rate, double tempo_ratio) {
    if (!pad.has_sample())
        return;
    if (tempo_ratio <= 0.0 || std::fabs(tempo_ratio - 1.0) < 0.001)
        return;

    pool.reset_all();

    const int num_input = pad.num_samples();
    std::vector<float> input(pad.data(), pad.data() + num_input);

    if (num_input < 256)
        return;

    soundtouch::SoundTouch st;
    st.setChannels(1);
    st.setSampleRate(static_cast<unsigned int>(sample_rate));
    st.setTempo(tempo_ratio);
    st.putSamples(input.data(), static_cast<unsigned int>(num_input));
    st.flush();

    const int est_out = static_cast<int>(static_cast<double>(num_input) / tempo_ratio * 1.15 + 512);
    std::vector<float> output;
    output.reserve(static_cast<size_t>(est_out));
    static constexpr unsigned int k_chunk = 1024;
    float tmp[k_chunk];
    unsigned int nReceived;
    do {
        nReceived = st.receiveSamples(tmp, k_chunk);
        output.insert(output.end(), tmp, tmp + nReceived);
    } while (nReceived > 0);

    jassert(!output.empty());
    if (output.empty())
        return;

    auto& buf = pad.sample_buffer();
    buf.setSize(1, static_cast<int>(output.size()), false, true, false);
    juce::FloatVectorOperations::copy(buf.getWritePointer(0), output.data(), static_cast<int>(output.size()));
}
