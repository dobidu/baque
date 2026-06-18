#pragma once
#include "baque_look_and_feel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

class BaqueProcessor;

// BrowserScreen — BROWSER tab: filesystem sample browser + load-to-pad.
// v1 scope: file list (*.wav/*.aif) + load to selected pad.
// Deferred to Phase 11: waveform thumbnail, tags, aesthetic filters,
// auto-audition playback, drag-and-drop, subdirectory tree navigation.
class BrowserScreen : public juce::Component,
                      private juce::Timer,
                      private juce::ListBoxModel {
  public:
    BrowserScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~BrowserScreen() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;

    // File browser (left 60%)
    juce::WildcardFileFilter file_filter_{"*.wav;*.aif;*.aiff;*.WAV;*.AIF;*.AIFF", {}, "Audio files"};
    juce::TimeSliceThread    thread_{"baqueBrowser"};    // BEFORE contents_ — contents_ takes reference
    juce::DirectoryContentsList contents_{&file_filter_, thread_};
    juce::FileListComponent  file_list_{contents_};
    juce::TextButton         root_btn_{"SET FOLDER"};

    // Preview panel (right 40%)
    juce::Label      file_label_;
    juce::Label      pad_label_;
    juce::ComboBox   pad_combo_;
    juce::TextButton load_btn_{"LOAD"};
    juce::Label      status_label_;

    juce::File selected_file_{};

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int lastRow) override;

    void load_selected_to_pad();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserScreen)
};
