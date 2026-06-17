#include "plugin_editor.h"

#include "plugin_processor.h"

static constexpr const char* k_screen_names[BaqueEditor::k_num_screens] = {
    "PERFORM", "FX", "MIX", "PERF FX", "BROWSER", "MIDI"};

BaqueEditor::BaqueEditor(BaqueProcessor& p)
    : AudioProcessorEditor(&p)
    , look_and_feel_(BaqueLookAndFeel::Theme::barro)
    , header_(p, look_and_feel_) {
    setLookAndFeel(&look_and_feel_);

    screens_[Screen::PERFORM] = std::make_unique<PerformScreen>(p, look_and_feel_);
    screens_[Screen::FX] = std::make_unique<FxScreen>(p, look_and_feel_);
    screens_[Screen::MIX] = std::make_unique<MixScreen>(p, look_and_feel_);
    for (int i = 0; i < k_num_screens; ++i) {
        if (!screens_[i])
            screens_[i] = std::make_unique<ScreenPlaceholder>(k_screen_names[i]);
        addChildComponent(*screens_[i]);
    }

    header_.on_screen_changed = [this](Screen s) { showScreen(s); };
    addAndMakeVisible(header_);

    showScreen(Screen::PERFORM);

    // setSize last — triggers resized() which references screens_ and header_;
    // all members must be initialised before this call.
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(800, 600, 2560, 1440);
}

BaqueEditor::~BaqueEditor() {
    setLookAndFeel(nullptr);
}

void BaqueEditor::paint(juce::Graphics& g) {
    g.fillAll(look_and_feel_.background());
    look_and_feel_.paintGrainOverlay(g, getLocalBounds());
}

void BaqueEditor::resized() {
    auto r = getLocalBounds();
    header_.setBounds(r.removeFromTop(HeaderComponent::k_height));
    for (auto& screen : screens_)
        screen->setBounds(r);
}

void BaqueEditor::showScreen(Screen s) {
    const int idx = static_cast<int>(s);
    for (int i = 0; i < k_num_screens; ++i)
        screens_[i]->setVisible(i == idx);
    header_.setActiveScreen(s);
    active_screen_ = s;
}

bool BaqueEditor::isScreenVisible(int screen_index) const noexcept {
    if (screen_index < 0 || screen_index >= k_num_screens)
        return false;
    return screens_[screen_index]->isVisible();
}
