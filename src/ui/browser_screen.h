#pragma once
#include "baque_look_and_feel.h"
#include "../preset_manager.h"
#include "../factory_preset_library.h"
#include <juce_audio_processors/juce_audio_processors.h>

class BaqueProcessor;

// BrowserScreen — BROWSER tab: filesystem sample browser + load-to-pad.
// v1 scope: synchronous file list (*.wav/*.aif) in a selected folder + load to pad.
// PRESETS tab: factory presets (FactoryPresetLibrary) + user presets (PresetManager).
// Deferred to Phase 12: waveform thumbnail, tags, aesthetic filters,
// auto-audition playback, drag-and-drop, recursive scan, async loading.
class BrowserScreen : public juce::Component,
                      private juce::ListBoxModel {
  public:
    BrowserScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~BrowserScreen() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;

    // File browser (left 60%)
    juce::Array<juce::File> files_;
    juce::ListBox            file_list_;
    juce::TextButton         root_btn_{"SET FOLDER"};

    // Preview panel (right 40%)
    juce::Label      file_label_;
    juce::Label      pad_label_;
    juce::ComboBox   pad_combo_;
    juce::TextButton load_btn_{"LOAD"};
    juce::Label      status_label_;

    juce::File selected_file_{};

    void changeDirectory(const juce::File& dir);

    // ListBoxModel (for file_list_)
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int lastRow) override;

    void load_selected_to_pad();

    // Mode tab
    enum class BrowserMode { Samples, Presets };
    BrowserMode mode_ = BrowserMode::Samples;
    juce::TextButton samples_tab_{"SAMPLES"};
    juce::TextButton presets_tab_{"PRESETS"};

    // Preset panel (PRESETS mode)
    struct PresetListModel : public juce::ListBoxModel {
        BrowserScreen& owner;
        PresetListModel(BrowserScreen& o) : owner(o) {}
        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
        void selectedRowsChanged(int lastRow) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    };
    PresetListModel preset_model_{*this};

    juce::Array<PresetManager::PresetInfo> user_presets_;
    juce::ListBox                           preset_list_;
    juce::Label                             preset_name_label_;
    juce::TextButton                        load_preset_btn_{"LOAD PRESET"};
    juce::TextButton                        save_preset_btn_{"SAVE PRESET"};
    juce::TextEditor                        preset_name_editor_;
    PresetManager                           preset_manager_; // declared after proc_

    void switchMode(BrowserMode m);
    void refresh_presets();
    void load_selected_preset();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserScreen)
};
