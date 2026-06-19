#include "browser_screen.h"
#include "../plugin_processor.h"

BrowserScreen::BrowserScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc), laf_(laf), preset_manager_(proc) {
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

    // Tab buttons
    samples_tab_.setClickingTogglesState(false);
    samples_tab_.onClick = [this] { switchMode(BrowserMode::Samples); };
    addAndMakeVisible(samples_tab_);

    presets_tab_.setClickingTogglesState(false);
    presets_tab_.onClick = [this] { switchMode(BrowserMode::Presets); };
    addAndMakeVisible(presets_tab_);

    // Preset panel
    preset_list_.setModel(&preset_model_);
    addChildComponent(preset_list_);

    preset_name_label_.setText("No preset selected", juce::dontSendNotification);
    preset_name_label_.setJustificationType(juce::Justification::topLeft);
    addChildComponent(preset_name_label_);

    load_preset_btn_.onClick = [this] { load_selected_preset(); };
    addChildComponent(load_preset_btn_);

    // M1: Do NOT use AlertWindow::showInputBox — blocks host message loop in plugin context.
    // Inline TextEditor: always visible in PRESETS panel, no modal event loop.
    preset_name_editor_.setTextToShowWhenEmpty("Preset name...", juce::Colours::grey);
    preset_name_editor_.setMultiLine(false);
    addChildComponent(preset_name_editor_);

    save_preset_btn_.onClick = [this] {
        const auto n = preset_name_editor_.getText().trim();
        if (n.isEmpty()) {
            status_label_.setText("Enter a preset name", juce::dontSendNotification);
            return;
        }
        preset_manager_.save(n, "custom");
        preset_name_editor_.setText({}, false);
        refresh_presets();
        status_label_.setText("Saved: " + n, juce::dontSendNotification);
    };
    addChildComponent(save_preset_btn_);
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

void BrowserScreen::switchMode(BrowserMode m) {
    mode_ = m;
    const bool samples = (m == BrowserMode::Samples);

    file_list_.setVisible(samples);
    root_btn_.setVisible(samples);
    file_label_.setVisible(samples);
    pad_label_.setVisible(samples);
    pad_combo_.setVisible(samples);
    load_btn_.setVisible(samples);

    preset_list_.setVisible(!samples);
    preset_name_label_.setVisible(!samples);
    load_preset_btn_.setVisible(!samples);
    preset_name_editor_.setVisible(!samples);
    save_preset_btn_.setVisible(!samples);

    if (!samples) refresh_presets();
    repaint();
}

void BrowserScreen::refresh_presets() {
    user_presets_ = preset_manager_.list_user_presets();
    preset_list_.updateContent();
}

void BrowserScreen::load_selected_preset() {
    const int row = preset_list_.getSelectedRow();
    if (row < 0) {
        status_label_.setText("No preset selected", juce::dontSendNotification);
        return;
    }
    if (row < FactoryPresetLibrary::k_count) {
        FactoryPresetLibrary::load_into(proc_, row);
        status_label_.setText(juce::String("Loaded: ") + FactoryPresetLibrary::name(row),
                               juce::dontSendNotification);
    } else {
        const int user_idx = row - FactoryPresetLibrary::k_count;
        if (user_idx >= user_presets_.size()) {
            status_label_.setText("Preset unavailable - refresh list", juce::dontSendNotification);
            return;
        }
        const auto& info = user_presets_[user_idx];
        preset_manager_.load(info.file);
        status_label_.setText("Loaded: " + info.name, juce::dontSendNotification);
    }
}

void BrowserScreen::paint(juce::Graphics& g) {
    g.fillAll(laf_.background());
    const int tab_h = 32;
    g.setColour(laf_.surface());
    g.drawVerticalLine(getWidth() * 6 / 10, static_cast<float>(tab_h), static_cast<float>(getHeight()));
}

void BrowserScreen::resized() {
    const int tab_h = 32;
    const int w = getWidth();
    const int h = getHeight();

    samples_tab_.setBounds(0, 0, w / 2, tab_h);
    presets_tab_.setBounds(w / 2, 0, w / 2, tab_h);

    // SAMPLES layout (content below tab bar)
    const int left_w = w * 6 / 10;
    const int btn_h = 32;
    const int content_h = h - tab_h;

    root_btn_.setBounds(0, tab_h + content_h - btn_h, left_w, btn_h);
    file_list_.setBounds(0, tab_h, left_w, content_h - btn_h);

    const int rx = left_w + 8;
    const int rw = w - left_w - 16;
    file_label_.setBounds(rx, tab_h + 8, rw, content_h / 4);
    pad_label_.setBounds(rx, tab_h + content_h / 4 + 8, rw, 24);
    pad_combo_.setBounds(rx, tab_h + content_h / 4 + 36, rw, 30);
    load_btn_.setBounds(rx, tab_h + content_h / 2, rw, 40);
    status_label_.setBounds(rx, tab_h + content_h / 2 + 50, rw, 24);

    // PRESETS layout
    const int left_p = w * 6 / 10;
    preset_list_.setBounds(0, tab_h, left_p, content_h);
    const int rpx = left_p + 8;
    const int rpw = w - left_p - 16;
    preset_name_label_.setBounds(rpx, tab_h + 8, rpw, content_h / 4);
    load_preset_btn_.setBounds(rpx, tab_h + content_h / 4 + 8, rpw, 40);
    preset_name_editor_.setBounds(rpx, tab_h + content_h / 4 + 60, rpw, 28);
    save_preset_btn_.setBounds(rpx, tab_h + content_h / 4 + 96, rpw, 40);
}

// --- BrowserScreen::ListBoxModel (file_list_) ---

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

// --- BrowserScreen::PresetListModel (preset_list_) ---

int BrowserScreen::PresetListModel::getNumRows() {
    return FactoryPresetLibrary::k_count + owner.user_presets_.size();
}

void BrowserScreen::PresetListModel::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) {
    if (selected)
        g.fillAll(owner.laf_.accent().withAlpha(0.4f));
    g.setFont(13.0f);
    if (row < FactoryPresetLibrary::k_count) {
        g.setColour(owner.laf_.text());
        g.drawText(juce::String("[F] ") + FactoryPresetLibrary::name(row),
                   8, 0, w - 12, h, juce::Justification::centredLeft);
    } else {
        const int user_idx = row - FactoryPresetLibrary::k_count;
        if (user_idx < owner.user_presets_.size()) {
            g.setColour(owner.laf_.text().withAlpha(0.8f));
            g.drawText(juce::String("[U] ") + owner.user_presets_[user_idx].name,
                       8, 0, w - 12, h, juce::Justification::centredLeft);
        }
    }
}

void BrowserScreen::PresetListModel::selectedRowsChanged(int lastRow) {
    if (lastRow < 0) {
        owner.preset_name_label_.setText({}, juce::dontSendNotification);
    } else if (lastRow < FactoryPresetLibrary::k_count) {
        owner.preset_name_label_.setText(FactoryPresetLibrary::name(lastRow),
                                         juce::dontSendNotification);
    } else {
        const int user_idx = lastRow - FactoryPresetLibrary::k_count;
        if (user_idx < owner.user_presets_.size())
            owner.preset_name_label_.setText(owner.user_presets_[user_idx].name,
                                             juce::dontSendNotification);
    }
}

void BrowserScreen::PresetListModel::listBoxItemDoubleClicked(int /*row*/, const juce::MouseEvent&) {
    owner.load_selected_preset();
}
