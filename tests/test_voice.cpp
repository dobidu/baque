#include "audio/sample_voice.h"
#include "audio/voice_pool.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <BinaryData.h>
#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

// --- Rastreador de alocações para teste de RT-safety ---
// Substitui o operador global new: conta chamadas quando g_count_allocs == true.
// Thread_local: conta apenas o thread de teste, não threads internas do JUCE.
static thread_local bool g_count_allocs = false;
static thread_local int g_alloc_count = 0;

void* operator new(std::size_t size) {
    if (g_count_allocs)
        ++g_alloc_count;
    void* p = std::malloc(size);
    if (p == nullptr)
        throw std::bad_alloc();
    return p;
}

void* operator new[](std::size_t size) {
    if (g_count_allocs)
        ++g_alloc_count;
    void* p = std::malloc(size);
    if (p == nullptr)
        throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete[](void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}

// Fixture do JUCE para testes que usam tipos JUCE
struct JuceFixture {
    juce::ScopedJuceInitialiser_GUI init;
};

// Gera um buffer de seno para testes
static std::vector<float> make_sine(int num_samples, float freq = 440.0f, float sr = 44100.0f) {
    std::vector<float> buf(static_cast<std::size_t>(num_samples));
    for (int i = 0; i < num_samples; ++i)
        buf[static_cast<std::size_t>(i)] = std::sin(2.0f * 3.14159265f * freq * static_cast<float>(i) / sr);
    return buf;
}

// 1. Silêncio quando nenhuma voz está ativa
TEST_CASE("voice pool silence when no voices active", "[voice][smoke]") {
    VoicePool pool;
    constexpr int frames = 512;
    std::vector<float> left(frames, 0.0f), right(frames, 0.0f);
    pool.process_all(left.data(), right.data(), frames);

    for (int i = 0; i < frames; ++i) {
        REQUIRE(left[static_cast<std::size_t>(i)] == Catch::Approx(0.0f));
        REQUIRE(right[static_cast<std::size_t>(i)] == Catch::Approx(0.0f));
    }
}

// 2. Pool aloca e reproduz sample
TEST_CASE("voice pool allocates and plays", "[voice]") {
    VoicePool pool;
    auto sine = make_sine(1000);

    SampleVoice* v = pool.allocate();
    REQUIRE(v != nullptr);
    v->trigger(sine.data(), static_cast<int>(sine.size()), 1.0f);

    constexpr int frames = 512;
    std::vector<float> left(frames), right(frames);

    // Primeiro bloco: deve conter samples não-zero (após fade-in de 32 amostras)
    pool.process_all(left.data(), right.data(), frames);
    bool found_nonzero = false;
    for (int i = 32; i < frames; ++i) {
        if (std::abs(left[static_cast<std::size_t>(i)]) > 1e-6f) {
            found_nonzero = true;
            break;
        }
    }
    REQUIRE(found_nonzero);
}

// 3. Pool cheio: roubo obrigatório da voz mais antiga
TEST_CASE("voice pool capacity steals oldest voice", "[voice]") {
    VoicePool pool;
    auto sine = make_sine(1000);
    constexpr int frames = 100;
    std::vector<float> left(frames), right(frames);

    // Preenche o pool
    for (int i = 0; i < VoicePool::k_pool_size; ++i) {
        SampleVoice* v = pool.allocate();
        REQUIRE(v != nullptr);
        v->trigger(sine.data(), static_cast<int>(sine.size()), 1.0f);
        // Avança a posição de cada voz (vozes posteriores têm posição menor)
        pool.process_all(left.data(), right.data(), frames);
    }

    // Tenta alocar além da capacidade — deve retornar voz roubada (não nullptr)
    SampleVoice* stolen = pool.allocate();
    REQUIRE(stolen != nullptr); // Roubo obrigatório
}

// 4. Fade-in no ataque evita clique
TEST_CASE("voice attack fade-in prevents click", "[voice]") {
    VoicePool pool;
    auto sine = make_sine(1000);

    SampleVoice* v = pool.allocate();
    v->trigger(sine.data(), static_cast<int>(sine.size()), 1.0f);

    constexpr int frames = 64;
    std::vector<float> left(frames), right(frames);
    pool.process_all(left.data(), right.data(), frames);

    // Primeira amostra deve ser (praticamente) silenciosa — fade-in começa do zero
    REQUIRE(std::abs(left[0]) < 1e-6f);

    // Amostra 32 deve ter amplitude maior que a amostra 0 (fade-in em progresso)
    REQUIRE(left[32] > left[0]);
}

// --- Fase 4 (04-01): varispeed, reverso, pan, limites ---

// Conta quantos frames a voz leva até desativar (processa em blocos)
static int frames_until_inactive(SampleVoice& v, int max_frames) {
    constexpr int block = 64;
    std::vector<float> left(block), right(block);
    int total = 0;
    while (v.is_active() && total < max_frames) {
        std::fill(left.begin(), left.end(), 0.0f);
        std::fill(right.begin(), right.end(), 0.0f);
        v.process(left.data(), right.data(), block);
        total += block;
    }
    return total;
}

// A. AC-2: rate 2.0 desativa em metade dos frames de rate 1.0
TEST_CASE("varispeed rate halves duration at plus 12 semitones", "[voice][varispeed]") {
    auto sine = make_sine(1000);

    SampleVoice v1;
    v1.trigger(sine.data(), static_cast<int>(sine.size()), 1.0f, 1.0);
    const int frames_normal = frames_until_inactive(v1, 4000);

    SampleVoice v2;
    v2.trigger(sine.data(), static_cast<int>(sine.size()), 1.0f, 2.0);
    const int frames_double = frames_until_inactive(v2, 4000);

    // rate 2.0 → metade da duração (tolerância de 2 blocos de 64)
    REQUIRE(std::abs(frames_normal - 2 * frames_double) <= 192);
}

// B. AC-2 (auditoria): rate 4.0 em sample curto — sem leitura fora dos limites
TEST_CASE("varispeed rate four no out of bounds on short sample", "[voice][varispeed]") {
    auto sine = make_sine(100); // Sample curto

    SampleVoice v;
    v.trigger(sine.data(), static_cast<int>(sine.size()), 1.0f, 4.0);

    constexpr int frames = 64;
    std::vector<float> left(frames, 0.0f), right(frames, 0.0f);
    v.process(left.data(), right.data(), frames);

    // ~99/4 ≈ 24 frames até o fim → voz desativa dentro do primeiro bloco
    REQUIRE(!v.is_active());

    // Saída finita (sem lixo de leitura fora dos limites; jassert cobre debug)
    for (int i = 0; i < frames; ++i)
        REQUIRE(std::isfinite(left[static_cast<std::size_t>(i)]));
}

// C. AC-3: reverso reproduz o sample espelhado (rampa decrescente)
TEST_CASE("reverse playback outputs mirrored ramp", "[voice][reverse]") {
    // Rampa crescente 0..1
    constexpr int n = 1000;
    std::vector<float> ramp(n);
    for (int i = 0; i < n; ++i)
        ramp[static_cast<std::size_t>(i)] = static_cast<float>(i) / static_cast<float>(n);

    SampleVoice v;
    v.trigger(ramp.data(), n, 1.0f, 1.0, true); // reverse

    constexpr int frames = 128;
    std::vector<float> left(frames, 0.0f), right(frames, 0.0f);
    v.process(left.data(), right.data(), frames);

    // Após o fade-in (32 frames): valores decrescentes (lendo do fim para o início)
    for (int i = 40; i < 60; ++i)
        REQUIRE(left[static_cast<std::size_t>(i)] > left[static_cast<std::size_t>(i + 1)]);
}

// D. AC (auditoria): reverso + start_offset_ — silêncio nos primeiros N frames
TEST_CASE("reverse with trigger offset stays silent before offset", "[voice][reverse]") {
    constexpr int n = 1000;
    std::vector<float> ramp(n);
    for (int i = 0; i < n; ++i)
        ramp[static_cast<std::size_t>(i)] = static_cast<float>(i) / static_cast<float>(n);

    VoicePool pool;
    constexpr int offset = 10;
    pool.trigger_at(offset, ramp.data(), n, 1.0f, 1.0, true, 0.0f);

    constexpr int frames = 64;
    std::vector<float> left(frames), right(frames);
    pool.process_all(left.data(), right.data(), frames);

    // Frames 0..9: silêncio (semântica de offset relativa ao bloco, inalterada)
    for (int i = 0; i < offset; ++i)
        REQUIRE(std::abs(left[static_cast<std::size_t>(i)]) < 1e-10f);

    // Frame do offset: primeiro frame renderizado (fim da rampa × fade 1/32)
    REQUIRE(std::abs(left[offset]) > 1e-6f);
}

// E. Pan -1 silencia o canal direito (equal-power)
TEST_CASE("pan hard left silences right channel", "[voice][pan]") {
    auto sine = make_sine(1000);

    SampleVoice v;
    v.trigger(sine.data(), static_cast<int>(sine.size()), 1.0f, 1.0, false, -1.0f);

    constexpr int frames = 128;
    std::vector<float> left(frames, 0.0f), right(frames, 0.0f);
    v.process(left.data(), right.data(), frames);

    bool left_nonzero = false;
    for (int i = 32; i < frames; ++i) {
        if (std::abs(left[static_cast<std::size_t>(i)]) > 1e-6f)
            left_nonzero = true;
        // sin(0) = 0 → direita silenciosa em pan -1
        REQUIRE(std::abs(right[static_cast<std::size_t>(i)]) < 1e-6f);
    }
    REQUIRE(left_nonzero);
}

// --- Fase 4 (04-02 T3): ADSR + play modes ---

// Auxiliar: buffer DC constante
static std::vector<float> make_dc(int num_samples, float value = 1.0f) {
    return std::vector<float>(static_cast<std::size_t>(num_samples), value);
}

// F1. one_shot ignora note_off — voz continua após note-off
TEST_CASE("one shot ignores note off", "[voice][adsr][playmode]") {
    auto dc = make_dc(1000, 1.0f);
    SampleVoice v;
    v.trigger(
        dc.data(), static_cast<int>(dc.size()), 1.0f, 1.0, false, 0.0f, -1, AdsrParams{}, PlayMode::one_shot, 48000.0);
    v.note_off();

    constexpr int frames = 10;
    std::vector<float> l(frames, 0.0f), r(frames, 0.0f);
    v.process(l.data(), r.data(), frames);

    REQUIRE(v.is_active()); // one_shot ignora note_off
}

// F2. gate: note_off inicia release; voz inativa após ~32 frames (default release = k_fade_frames)
TEST_CASE("gate note off triggers release", "[voice][adsr][playmode]") {
    auto dc = make_dc(10000, 1.0f);
    SampleVoice v;
    v.trigger(
        dc.data(), static_cast<int>(dc.size()), 1.0f, 1.0, false, 0.0f, -1, AdsrParams{}, PlayMode::gate, 48000.0);
    v.note_off(); // release_ms=0 → rel_f=32 (k_fade_frames)

    constexpr int frames = 40; // > 32 (fade-out completo)
    std::vector<float> l(frames, 0.0f), r(frames, 0.0f);
    v.process(l.data(), r.data(), frames);

    REQUIRE(!v.is_active()); // release completo
}

// F3. loop: sample curto continua apos fim (wrap de posição)
TEST_CASE("loop mode wraps at sample end", "[voice][adsr][playmode]") {
    auto dc = make_dc(10, 1.0f); // sample curto — wrap após 10 frames
    SampleVoice v;
    v.trigger(
        dc.data(), static_cast<int>(dc.size()), 1.0f, 1.0, false, 0.0f, -1, AdsrParams{}, PlayMode::loop, 48000.0);

    constexpr int frames = 25; // > 2 × comprimento do sample
    std::vector<float> l(frames, 0.0f), r(frames, 0.0f);
    v.process(l.data(), r.data(), frames);

    REQUIRE(v.is_active()); // voz ainda ativa após o wrap
}

// F4. ADSR attack ramp: amplitude cresce de 0 até gain completo
TEST_CASE("ADSR attack ramp from zero to full gain", "[voice][adsr]") {
    constexpr double k_sr = 48000.0;
    // attack_ms=1ms → 48 frames; decay=0, sustain=1, release=0
    const AdsrParams adsr{1.0f, 0.0f, 1.0f, 0.0f};
    auto dc = make_dc(2000, 1.0f);
    SampleVoice v;
    v.trigger(dc.data(), static_cast<int>(dc.size()), 1.0f, 1.0, false, 0.0f, -1, adsr, PlayMode::one_shot, k_sr);

    constexpr int frames = 100;
    std::vector<float> l(frames, 0.0f), r(frames, 0.0f);
    v.process(l.data(), r.data(), frames);

    // Frame 0: envelope em 0 → saída ≈ 0
    REQUIRE(std::abs(l[0]) < 1e-4f);
    // Frame 49 (após os 48 frames de attack): envelope em 1.0 → amplitude completa
    REQUIRE(l[49] > 0.5f);
    // Amplitude aumentou entre frame 0 e 49
    REQUIRE(l[49] > l[0]);
}

// F5. ADSR decay para sustain: amplitude cai até sustain×gain após decay
TEST_CASE("ADSR decay reaches sustain level", "[voice][adsr]") {
    constexpr double k_sr = 48000.0;
    // decay_ms=1ms (48 frames) → sustain=0.5; sem attack
    const AdsrParams adsr{0.0f, 1.0f, 0.5f, 0.0f};
    auto dc = make_dc(2000, 1.0f);
    SampleVoice v;
    v.trigger(dc.data(), static_cast<int>(dc.size()), 1.0f, 1.0, false, 0.0f, -1, adsr, PlayMode::one_shot, k_sr);

    constexpr int frames = 200;
    std::vector<float> l(frames, 0.0f), r(frames, 0.0f);
    v.process(l.data(), r.data(), frames);

    // Após decay (frame 48+): amplitude ≈ 0.5 × 0.707 (sustain × pan-center)
    constexpr float k_cpan = 0.70710678f;
    REQUIRE(l[100] == Catch::Approx(0.5f * k_cpan).margin(0.05f));
}

// F6. gate + sustain=0: note_off imediatamente desativa (sem vazamento de voz)
TEST_CASE("gate sustain zero note off immediately deactivates", "[voice][adsr][playmode]") {
    constexpr double k_sr = 48000.0;
    // decay_ms=1ms → 48 frames; sustain=0 → env_level_ cai a 0 após decay
    const AdsrParams adsr{0.0f, 1.0f, 0.0f, 0.0f};
    auto dc = make_dc(4000, 1.0f);
    SampleVoice v;
    v.trigger(dc.data(), static_cast<int>(dc.size()), 1.0f, 1.0, false, 0.0f, -1, adsr, PlayMode::gate, k_sr);

    // Processa além do decay para que env_level_ chegue a 0
    constexpr int frames_decay = 60;
    std::vector<float> l(frames_decay, 0.0f), r(frames_decay, 0.0f);
    v.process(l.data(), r.data(), frames_decay);

    // Após decay completo: note_off deve desativar imediatamente (env_level_ <= 0)
    v.note_off();
    REQUIRE(!v.is_active());
}

// 5. processBlock não aloca memória no audio thread
TEST_CASE_METHOD(JuceFixture, "processBlock makes zero allocations", "[voice][rt-safety]") {
    BaqueProcessor processor;
    const double sample_rate = 44100.0;
    const int block_size = 512;
    processor.prepareToPlay(sample_rate, block_size);

    juce::AudioBuffer<float> buffer(2, block_size);
    buffer.clear();
    juce::MidiBuffer midi;

    // Reseta contador e inicia rastreamento apenas durante processBlock
    g_alloc_count = 0;
    g_count_allocs = true;
    processor.processBlock(buffer, midi);
    g_count_allocs = false;

    REQUIRE(g_alloc_count == 0);
}
