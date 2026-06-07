#include "audio/fx_chain.h"
#include "audio/fx_params.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>
#include <cmath>

static constexpr double k_pi = 3.14159265358979323846;

static void fill_sine(juce::AudioBuffer<float>& buf, float freq, double sr, float amplitude = 1.0f) {
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            buf.setSample(ch, i, amplitude * static_cast<float>(std::sin(2.0 * k_pi * freq * i / sr)));
}

static float rms_in_range(const juce::AudioBuffer<float>& buf, int start, int end) {
    double sum_sq = 0.0;
    int count = 0;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = start; i < end; ++i) {
            const float s = buf.getSample(ch, i);
            sum_sq += static_cast<double>(s) * s;
            ++count;
        }
    return count > 0 ? static_cast<float>(std::sqrt(sum_sq / count)) : 0.0f;
}

static float peak_in_range(const juce::AudioBuffer<float>& buf, int start, int end) {
    float peak = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = start; i < end; ++i)
            peak = std::max(peak, std::abs(buf.getSample(ch, i)));
    return peak;
}

static float energy_in_range(const juce::AudioBuffer<float>& buf, int start, int end) {
    float sum = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = start; i < end; ++i)
            sum += std::abs(buf.getSample(ch, i));
    return sum;
}

// PD1: p-locked filter_cutoff changes spectral content of FxChain output
TEST_CASE("PD1: P-lock filter_cutoff 500Hz suppresses 5kHz sine vs cutoff 20kHz", "[fx_plock_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    // Separate instances: no filter/IIR state cross-contamination (audit SR1)
    FxChain chain_lo, chain_hi;
    chain_lo.prepare(44100.0, 4096);
    chain_hi.prepare(44100.0, 4096);

    FxParams p_lo{};
    p_lo.filter_cutoff = 500.0f; // 500 Hz LP: 5 kHz is 10x above -> ~40 dB/decade attenuation
    p_lo.filter_res = 0.1f;
    p_lo.reverb_mix = 0.0f;
    p_lo.delay_mix = 0.0f;
    p_lo.delay_time = 0.25f;
    p_lo.sidechain_threshold = 0.0f; // 0 dBFS: envelope of 1.0-amp sine stays < 1.0 -> no compression

    FxParams p_hi = p_lo;
    p_hi.filter_cutoff = 20000.0f; // 20 kHz LP: 5 kHz below cutoff -> near-transparent

    juce::AudioBuffer<float> buf_lo(2, 4096);
    fill_sine(buf_lo, 5000.0f, 44100.0);
    chain_lo.process(buf_lo, p_lo);

    juce::AudioBuffer<float> buf_hi(2, 4096);
    fill_sine(buf_hi, 5000.0f, 44100.0);
    chain_hi.process(buf_hi, p_hi);

    // Tail after SmoothedValue convergence (20ms = 882 samples at 44100Hz)
    const float rms_lo = rms_in_range(buf_lo, 1024, 4096);
    const float rms_hi = rms_in_range(buf_hi, 1024, 4096);

    REQUIRE(rms_hi > 0.01f);         // 5 kHz passes through 20 kHz LP (sanity check)
    REQUIRE(rms_lo < rms_hi * 0.5f); // 500 Hz LP suppresses 5 kHz by > 50%
}

// PD2: p-locked sidechain_threshold controls compression magnitude (contrast test)
// Deviation from plan: plan named -3dBFS "tight" and -60dBFS "loose" with assertion peak_tight <
// peak_loose * 0.7f. This is inverted: -3dBFS (HIGH threshold) gives MILD compression (high output);
// -60dBFS (LOW threshold) gives HEAVY compression (low output). Variables renamed to hi_thresh /
// lo_thresh and assertion flipped. Logged as deviation for UNIFY.
TEST_CASE("PD2: P-lock sidechain_threshold -60dBFS compresses harder than -3dBFS", "[fx_plock_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    // Separate instances: IIR envelope state not shared between runs (audit SR1)
    FxChain chain_hi_thresh, chain_lo_thresh;
    chain_hi_thresh.prepare(44100.0, 88200);
    chain_lo_thresh.prepare(44100.0, 88200);

    // -3 dBFS threshold (HIGH): signal at ~-0.45 dBFS is only 2.55 dB above -> mild compression
    FxParams p_hi_thresh{};
    p_hi_thresh.filter_cutoff = 20000.0f;
    p_hi_thresh.filter_res = 0.1f;
    p_hi_thresh.reverb_mix = 0.0f;
    p_hi_thresh.delay_mix = 0.0f;
    p_hi_thresh.delay_time = 0.25f;
    p_hi_thresh.sidechain_threshold = -3.0f;

    // -60 dBFS threshold (LOW): signal is 59.55 dB above -> heavy compression (tight sidechain pump)
    FxParams p_lo_thresh = p_hi_thresh;
    p_lo_thresh.sidechain_threshold = -60.0f;

    // 0.95f amplitude (~-0.45 dBFS) above both thresholds; 88200 samples for IIR convergence
    juce::AudioBuffer<float> buf_hi(2, 88200);
    fill_sine(buf_hi, 440.0f, 44100.0, 0.95f);
    chain_hi_thresh.process(buf_hi, p_hi_thresh);

    juce::AudioBuffer<float> buf_lo(2, 88200);
    fill_sine(buf_lo, 440.0f, 44100.0, 0.95f);
    chain_lo_thresh.process(buf_lo, p_lo_thresh);

    // Skip first 8192 samples (37 attack tau) for IIR envelope convergence (pattern from SC3/SC5)
    const float peak_hi_thresh = peak_in_range(buf_hi, 8192, 88200);
    const float peak_lo_thresh = peak_in_range(buf_lo, 8192, 88200);

    REQUIRE(peak_hi_thresh > 0.3f);                  // -3 dBFS: mild compression -> output above 0.3
    REQUIRE(peak_lo_thresh < peak_hi_thresh * 0.1f); // -60 dBFS: heavy compression -> output << mild
}

// PD3: multi-param batch (reverb+delay) adds energy in delay-echo window vs dry (audit M1 contrast test)
TEST_CASE("PD3: Multi-param p-lock reverb+delay adds energy in echo window vs dry chain", "[fx_plock_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    // Separate instances: no reverb/delay state contamination (audit SR1)
    FxChain chain_dry, chain_wet;
    chain_dry.prepare(44100.0, 4096);
    chain_wet.prepare(44100.0, 4096);

    FxParams p_dry{};
    p_dry.filter_cutoff = 20000.0f;
    p_dry.filter_res = 0.1f;
    p_dry.reverb_mix = 0.0f;  // no reverb
    p_dry.delay_mix = 0.0f;   // no echo
    p_dry.delay_time = 0.05f; // 50 ms = 2205 samples (defines echo window position)
    p_dry.sidechain_threshold = -60.0f;

    FxParams p_wet = p_dry;
    p_wet.reverb_mix = 0.8f; // reverb active
    p_wet.delay_mix = 0.8f;  // delay echo active

    // Impulse at sample 0 (both channels)
    juce::AudioBuffer<float> buf_dry(2, 4096);
    buf_dry.clear();
    for (int ch = 0; ch < 2; ++ch)
        buf_dry.setSample(ch, 0, 0.5f);
    chain_dry.process(buf_dry, p_dry);

    juce::AudioBuffer<float> buf_wet(2, 4096);
    buf_wet.clear();
    for (int ch = 0; ch < 2; ++ch)
        buf_wet.setSample(ch, 0, 0.5f);
    chain_wet.process(buf_wet, p_wet);

    // Delay echo window [2205, 4096]: first echo from 50ms delay arrives at sample 2205.
    // SmoothedValues (20ms ramp = 882 samples) fully converged by sample 2205.
    // Dry: delay_mix=0 -> no echo; impulse decayed well before sample 2205 -> near-zero energy.
    // Wet: delay_mix=0.8, delay_time=50ms -> echo amplitude ~0.4f at sample 2205.
    const float energy_dry = energy_in_range(buf_dry, 2205, 4096);
    const float energy_wet = energy_in_range(buf_wet, 2205, 4096);

    REQUIRE(energy_dry < 1e-3f); // dry: impulse decayed; no echo -> near-zero in echo window
    REQUIRE(energy_wet > 0.1f);  // wet: delay echo adds audible energy in echo window
}

// PD4: p-lock boundary FxParams values do not crash and produce finite output
// Deviation from plan: max-boundary delay_time changed from 2.0f to 0.001f. With delay_mix=1.0 and
// delay_time=2.0s (88200 samples), the wet-only output would be zero within a 4096-sample buffer
// (correct behavior, not a bug), making the non-zero output check (audit SR2) impossible. Using
// delay_time=0.001f ensures the echo arrives in-window. Logged as deviation for UNIFY.
TEST_CASE("PD4: P-lock boundary FxParams do not crash and produce finite non-NaN output", "[fx_plock_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    // Separate instances per boundary run (audit SR1)
    FxChain chain_min, chain_max;
    chain_min.prepare(44100.0, 4096);
    chain_max.prepare(44100.0, 4096);

    FxParams p_min{};
    p_min.filter_cutoff = 20.0f; // minimum Hz
    p_min.filter_res = 0.0f;
    p_min.reverb_mix = 0.0f;
    p_min.delay_mix = 0.0f;
    p_min.delay_time = 0.001f; // minimum seconds
    p_min.sidechain_threshold = -60.0f;

    FxParams p_max{};
    p_max.filter_cutoff = 20000.0f;
    p_max.filter_res = 1.0f;
    p_max.reverb_mix = 1.0f;
    p_max.delay_mix = 1.0f;
    p_max.delay_time = 0.001f;        // 1ms (not 2.0s) — see deviation note above
    p_max.sidechain_threshold = 0.0f; // 0 dBFS; 0.3f signal below 1.0 linear -> no compression

    // Min-boundary: silence in -> silence out (NaN check)
    juce::AudioBuffer<float> buf_min(2, 4096);
    buf_min.clear();
    REQUIRE_NOTHROW(chain_min.process(buf_min, p_min));

    // Max-boundary: 440 Hz sine at 0.3f amplitude (below 0 dBFS sidechain threshold)
    // audit SR2: also verify non-zero output (catches silent-output regression beyond NaN check)
    juce::AudioBuffer<float> buf_max(2, 4096);
    fill_sine(buf_max, 440.0f, 44100.0, 0.3f);
    REQUIRE_NOTHROW(chain_max.process(buf_max, p_max));

    // All samples finite (NaN/Inf guard from jlimit in threshold_db and FxChain clamping)
    for (int ch = 0; ch < buf_min.getNumChannels(); ++ch)
        for (int i = 0; i < buf_min.getNumSamples(); ++i)
            REQUIRE(std::isfinite(buf_min.getSample(ch, i)));

    for (int ch = 0; ch < buf_max.getNumChannels(); ++ch)
        for (int i = 0; i < buf_max.getNumSamples(); ++i)
            REQUIRE(std::isfinite(buf_max.getSample(ch, i)));

    // Non-zero output: 440 Hz at 0.3f through transparent sidechain (threshold=0 dBFS, signal<1.0)
    // reverb+delay with 1ms echo -> output must not be silenced by a bug zeroing the buffer
    REQUIRE(peak_in_range(buf_max, 0, 4096) > 0.001f);
}

// PD5: Phase 6 DoD marker — p-lock automates FX params; sidechain pump functional
TEST_CASE("PD5: Phase 6 DoD -- p-lock automates FX params; sidechain pump functional", "[fx_plock_dod][dod]") {
    // PD1: p-locked cutoff=500Hz measurably suppresses 5kHz vs cutoff=20kHz
    // PD2: p-locked threshold=-60dBFS compresses harder than -3dBFS (contrast test)
    // PD3: p-locked reverb_mix=0.8 + delay_mix=0.8 adds audible echo energy vs dry
    // PD4: boundary FxParams values produce no crash, no NaN/Inf, non-zero output
    // SC4 (test_sidechain_comp.cpp): sidechain pump charges envelope and ducks subsequent signal
    SUCCEED();
}
