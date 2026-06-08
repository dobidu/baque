#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdint>
#include <vector>

// FX de performance no domínio do buffer (estilo TR-8 scatter): captura o mix estéreo num ring
// pré-alocado e o reproduz através de transforms de glitch — repeat (buffer roll), reverse, gate,
// decimate. type: 0 = off, 1-10 = presets de glitch. depth [0,1] = intensidade (mix wet/dry).
// Comprimento de slice beat-synced a partir do BPM. Sem alocação em process(). Até 2 canais (L+R).
//
// INVARIANTE (audit M1): cada primitiva lê de uma slice CONGELADA, nunca do sample recém-escrito.
// Ao engajar (inativo→ativo) a slice mais recente (trilhando write_pos_ por slice_len) é COPIADA
// para slice_l_/slice_r_ e mantida fixa durante o roll → impossível auto-captura/feedback. A slice
// congelada faz loop, então o conteúdo recorre a cada slice_len (repeat). slice_pos_ persiste entre
// blocos. Fase de slice / máscara de gate / stride de decimate são COMPARTILHADOS entre L e R
// (coerência estéreo — audit SR1); só os dados de amostra diferem por canal.
class ScatterEngine {
  public:
    // Aloca o ring (até 2 s) e os buffers de slice congelada. Chamar off audio thread.
    void prepare(double sample_rate, int max_block_size) noexcept;

    // Aplica scatter in-place ao buffer estéreo. type 0 ou depth<=0 → bypass puro (mas sempre
    // captura no ring). bpm é clampeado a >= 1. RT-safe: sem alocação, sem locks, sem IO.
    void process(juce::AudioBuffer<float>& buffer, int type, float depth, double bpm) noexcept;

    // Zera ring + slice congelada + heads + contadores. Chamar em releaseResources()/stop.
    void reset() noexcept;

    // Comprimento de slice em samples para uma subdivisão de nota (denominador: 4=1/4, 8=1/8,
    // 16=1/16, 32=1/32). = round((sample_rate*60/bpm) * 4 / subdivision), >= 1.
    // Ex.: (48000, 120, 16) = 6000 (uma semicolcheia a 120 BPM em 48 kHz).
    [[nodiscard]] static int slice_length_samples(double sample_rate, double bpm, int subdivision) noexcept;

    static constexpr int k_num_types = 10;

  private:
    enum class Primitive { repeat, reverse, gate, decimate };

    // Mapeamento estático type (1-10) → primitiva + subdivisão de slice + parâmetro auxiliar.
    struct TypeSpec {
        Primitive prim;
        int subdivision;   // denominador de nota (8 = 1/8, 16 = 1/16, 32 = 1/32)
        int param;         // decimate: stride de sample-and-hold; outras: não usado
        bool also_reverse; // type 10: repeat tocado de trás para frente (stutter agressivo)
    };
    static const TypeSpec k_types[k_num_types]; // índice 0 → type 1

    // Copia a slice mais recente (slice_len samples terminando em write_pos_) do ring para a slice
    // congelada. Chamado uma vez por roll, no instante de engajar.
    void latch_slice(int slice_len) noexcept;

    double sample_rate_ = 44100.0;
    int ring_size_ = 0; // capacidade do ring em samples (≈ 2 s)

    std::vector<float> ring_l_;  // captura contínua — canal esquerdo / mono
    std::vector<float> ring_r_;  // captura contínua — canal direito
    std::vector<float> slice_l_; // slice congelada — esquerdo / mono
    std::vector<float> slice_r_; // slice congelada — direito

    int write_pos_ = 0;       // índice de escrita no ring (wrap circular)
    int slice_pos_ = 0;       // posição dentro da slice congelada [0, cur_slice_len_)
    int cur_slice_len_ = 1;   // comprimento da slice congelada do roll ativo
    bool was_active_ = false; // estado de engaje no sample anterior (detecta borda inativo→ativo)
};
