#pragma once
#include <juce_core/juce_core.h>

class BaqueProcessor;

// Gerencia presets de usuário (*.bqpreset) em userApplicationDataDirectory/BAQUE/presets/.
// Presets de fábrica ficam em um subdir "factory/" (Fase 11-02).
// v1: salva/carrega via getStateInformation/setStateInformation — formato binário ValueTree.
class PresetManager {
  public:
    struct PresetInfo {
        juce::String name;
        juce::String category; // "custom" ou tag de estética
        juce::File   file;
    };

    explicit PresetManager(BaqueProcessor& proc);

    // Salva estado atual como preset nomeado em user_preset_dir(). Sobrescreve se já existir.
    // Deve ser chamado da message thread.
    void save(const juce::String& name, const juce::String& category = "custom");

    // Salva para um arquivo arbitrário — para testes e exportação customizada.
    // Deve ser chamado da message thread.
    void save_to_file(const juce::String& name, const juce::String& category, const juce::File& dest);

    // Carrega preset do arquivo. Chama setStateInformation internamente.
    // Deve ser chamado da message thread.
    void load(const juce::File& file);

    // Lista presets do usuário (não inclui factory — esses vêm em 11-02).
    [[nodiscard]] juce::Array<PresetInfo> list_user_presets() const;

    // Diretório canônico de presets do usuário.
    [[nodiscard]] static juce::File user_preset_dir();

    // Extensão canônica de arquivo de preset.
    static constexpr const char* k_extension = ".bqpreset";

  private:
    BaqueProcessor& proc_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
