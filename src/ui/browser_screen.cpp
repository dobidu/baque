#include "browser_screen.h"
#include "../plugin_processor.h"

BrowserScreen::BrowserScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc), laf_(laf) {
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
                const auto result = ch.getResult();
                if (!result.isDirectory()) return;
                safeThis->changeDirectory(result);
            });
    };
    addAndMakeVisible(root_btn_);

    file_label_.setText("No file selected", juce::dontSendNotification);
    file_label_.setJustificationType(juce::Justification::topLeft);
    file_label_.setMinimumHorizontalScale(0.5f);
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

    changeDirectory(juce::File::getSpecialLocation(juce::File::userHomeDirectory));
}

void BrowserScreen::changeDirectory(const juce::File& dir) {
    files_.clear();
    dir.findChildFiles(files_, juce::File::findFiles, false,
                       "*.wav;*.aif;*.aiff;*.WAV;*.AIF;*.AIFF");
    files_.sort();
    selected_file_ = juce::File{};
    file_label_.setText("No file selected", juce::dontSendNotification);
    status_label_.setText(juce::String(files_.size()) + " files", juce::dontSendNotification);
    file_list_.updateContent();
    file_list_.scrollToEnsureRowIsOnscreen(0);
    repaint();
}

void BrowserScreen::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
    // Separator between left file list and right panel
    g.setColour(laf_.surface());
    g.drawVerticalLine(getWidth() * 6 / 10, 0.0f, static_cast<float>(getHeight()));
}

void BrowserScreen::resized() {
    const int w = getWidth();
    const int h = getHeight();
    const int left_w = w * 6 / 10;
    const int btn_h = 32;

    root_btn_.setBounds(0, h - btn_h, left_w, btn_h);
    file_list_.setBounds(0, 0, left_w, h - btn_h);

    const int rx = left_w + 8;
    const int rw = w - left_w - 16;
    file_label_.setBounds(rx, 8, rw, h / 4);
    pad_label_.setBounds(rx, h / 4 + 8, rw, 24);
    pad_combo_.setBounds(rx, h / 4 + 36, rw, 30);
    load_btn_.setBounds(rx, h / 2, rw, 40);
    status_label_.setBounds(rx, h / 2 + 50, rw, 24);
}

int BrowserScreen::getNumRows() {
    return files_.size();
}

void BrowserScreen::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) {
    if (selected)
        g.fillAll(laf_.accent().withAlpha(0.4f));
    g.setColour(laf_.text());
    g.setFont(13.0f);
    if (row >= 0 && row < files_.size())
        g.drawText(files_[row].getFileName(), 8, 0, w - 12, h, juce::Justification::centredLeft);
}

void BrowserScreen::selectedRowsChanged(int lastRow) {
    if (lastRow >= 0 && lastRow < files_.size()) {
        selected_file_ = files_[lastRow];
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
