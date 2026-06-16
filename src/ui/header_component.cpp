#include "header_component.h"

static constexpr const char* k_nav_labels[HeaderComponent::NUM_SCREENS] = {
    "PERFORM", "FX", "MIX", "PERF FX", "BROWSER", "MIDI"};

HeaderComponent::HeaderComponent(BaqueProcessor& proc, BaqueLookAndFeel& laf)
    : proc_(proc)
    , laf_(laf)
    , meter_(proc.ui_snapshot()) {
    buildNavButtons();

    bpm_label_.setJustificationType(juce::Justification::centred);
    bpm_label_.setFont(juce::Font(juce::FontOptions(laf_.monoTypeface()).withHeight(14.0f)));
    addAndMakeVisible(bpm_label_);
    addAndMakeVisible(meter_);
}

HeaderComponent::~HeaderComponent() {
    stopTimer();
}

void HeaderComponent::buildNavButtons() {
    for (int i = 0; i < NUM_SCREENS; ++i) {
        nav_buttons_[i].setButtonText(k_nav_labels[i]);
        nav_buttons_[i].setClickingTogglesState(true);
        nav_buttons_[i].setRadioGroupId(1);
        nav_buttons_[i].onClick = [this, i]() {
            if (on_screen_changed)
                on_screen_changed(static_cast<Screen>(i));
        };
        addAndMakeVisible(nav_buttons_[i]);
    }
    nav_buttons_[PERFORM].setToggleState(true, juce::dontSendNotification);
}

void HeaderComponent::paint(juce::Graphics& g) {
    g.fillAll(laf_.surface());
    laf_.paintGrainOverlay(g, getLocalBounds());
}

void HeaderComponent::resized() {
    auto r = getLocalBounds().reduced(4);

    // Left: wordmark placeholder (~80px)
    r.removeFromLeft(80);

    // Right: meter (40px) + BPM label (60px)
    meter_.setBounds(r.removeFromRight(40));
    bpm_label_.setBounds(r.removeFromRight(60));

    // Centre: 6 NAV buttons equally distributed
    const int nav_w = r.getWidth() / NUM_SCREENS;
    for (auto& btn : nav_buttons_)
        btn.setBounds(r.removeFromLeft(nav_w));
}

void HeaderComponent::visibilityChanged() {
    if (isVisible())
        startTimerHz(10);
    else
        stopTimer();
}

void HeaderComponent::timerCallback() {
    const auto& snap = proc_.ui_snapshot();
    const float bpm = snap.bpm.load(std::memory_order_relaxed);
    bpm_label_.setText(juce::String(bpm, 1) + " BPM", juce::dontSendNotification);
}

void HeaderComponent::setActiveScreen(Screen s) {
    for (auto& btn : nav_buttons_)
        btn.setToggleState(false, juce::dontSendNotification);
    nav_buttons_[static_cast<int>(s)].setToggleState(true, juce::dontSendNotification);
}
