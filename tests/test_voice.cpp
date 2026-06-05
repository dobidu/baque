#include "audio/sample_voice.h"
#include "audio/voice_pool.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <BinaryData.h>
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
