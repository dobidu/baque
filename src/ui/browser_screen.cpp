#include "browser_screen.h"
#include "../plugin_processor.h"

BrowserScreen::BrowserScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc), laf_(laf) {
    thread_.startThread(juce::Thread::Priority::low);

    const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    contents_.setDirectory(home, true, true);

    file_list_.setModel(this);
    addAndMakeVisible(file_list_);

    // M1: raw [this] capture is UAF if BrowserScreen destroyed while OS dialog open.
    // SafePointer becomes nullptr automatically when component is deleted.
    root_btn_.onClick = [this] {
        juce::Component::SafePointer<BrowserScreen> safeThis(this);
        auto fc = std::make_shared<juce::FileChooser>("Select sample folder",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory));
        fc->launchAsync(juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectDirectories,
            [safeThis, fc](const juce::FileChooser& ch) {
                if (safeThis == nullptr) return;
                if (!ch.getResult().exists()) return;
                safeThis->contents_.setDirectory(ch.getResult(), true, true);
                safeThis->file_list_.updateContent();
                safeThis->startTimerHz(2); // SR3: restart poll for new scan
            });
    };
    addAndMakeVisible(root_btn_);

    file_label_.setText("No file selected", juce::dontSendNotification);
    file_label_.setJustificationType(juce::Justification::centredTop);
    addAndMakeVisible(file_label_);

    pad_label_.setText("Target pad:", juce::dontSendNotification);
    addAndMakeVisible(pad_label_);

    for (int i = 1; i <= 16; ++i)
        pad_combo_.addItem("PAD " + juce::String(i), i);
    pad_combo_.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(pad_combo_);

    load_btn_.onClick = [this] { load_selected_to_pad(); };
    addAndMakeVisible(load_btn_);

    status_label_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(status_label_);
}

BrowserScreen::~BrowserScreen() {
    stopTimer();
    thread_.stopThread(2000); // before contents_ dtor — contents_ holds a reference to thread_
}

void BrowserScreen::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
}

void BrowserScreen::resized() {
    const int w = getWidth();
    const int h = getHeight();
    const int left_w = w * 6 / 10;
    const int btn_h = 32;

    root_btn_.setBounds(0, h - btn_h, left_w, btn_h);
    file_list_.setBounds(0, 0, left_w, h - btn_h);

    const int rx = left_w;
    const int rw = w - left_w;
    file_label_.setBounds(rx + 8, 8, rw - 16, h / 4);
    pad_label_.setBounds(rx + 8, h / 4 + 8, rw - 16, 24);
    pad_combo_.setBounds(rx + 8, h / 4 + 36, rw - 16, 30);
    load_btn_.setBounds(rx + 8, h / 2, rw - 16, 40);
    status_label_.setBounds(rx + 8, h / 2 + 50, rw - 16, 24);
}

void BrowserScreen::visibilityChanged() {
    if (isVisible())
        startTimerHz(2);
    else
        stopTimer();
}

void BrowserScreen::timerCallback() {
    file_list_.updateContent();
    repaint();
    if (!contents_.isStillLoading()) stopTimer(); // SR3: stop 2fps poll after scan completes
}

int BrowserScreen::getNumRows() {
    return contents_.getNumFiles();
}

void BrowserScreen::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) {
    g.fillAll(selected ? laf_.accent().withAlpha(0.25f) : laf_.surface().withAlpha(0.08f));
    g.setColour(laf_.text());
    g.setFont(13.0f);
    const auto f = contents_.getFile(row);
    g.drawText(f.getFileName(), 8, 0, w - 12, h, juce::Justification::centredLeft);
}

void BrowserScreen::selectedRowsChanged(int lastRow) {
    if (lastRow >= 0 && lastRow < contents_.getNumFiles()) {
        selected_file_ = contents_.getFile(lastRow);
        file_label_.setText(selected_file_.getFileName(), juce::dontSendNotification);
        status_label_.setText({}, juce::dontSendNotification);
    }
}

void BrowserScreen::listBoxItemDoubleClicked(int /*row*/, const juce::MouseEvent&) {
    load_selected_to_pad();
}

void BrowserScreen::load_selected_to_pad() {
    if (!selected_file_.existsAsFile()) {
        status_label_.setText("No file selected", juce::dontSendNotification);
        return;
    }
    const int pad = pad_combo_.getSelectedId() - 1; // 0-based
    proc_.load_sample_from_file(pad, selected_file_);
    status_label_.setText("Loaded to PAD " + juce::String(pad + 1), juce::dontSendNotification);
}
