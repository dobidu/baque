#pragma once
#include "baque_look_and_feel.h"
#include <juce_audio_processors/juce_audio_processors.h>

class BaqueProcessor;

// BrowserScreen — BROWSER tab: filesystem sample browser + load-to-pad.
// v1 scope: synchronous file list (*.wav/*.aif) in a selected folder + load to pad.
// Deferred to Phase 11: waveform thumbnail, tags, aesthetic filters,
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

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int lastRow) override;

    void load_selected_to_pad();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserScreen)
};
