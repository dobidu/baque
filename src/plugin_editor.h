#pragma once

#include "ui/baque_look_and_feel.h"
#include "ui/header_component.h"
#include "ui/perform_screen.h"
#include "ui/fx_screen.h"
#include "ui/mix_screen.h"
#include "ui/perf_fx_screen.h"
#include "ui/midi_screen.h"
#include "ui/browser_screen.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

class BaqueProcessor;

// Placeholder screen — replaced by a full screen component in the relevant plan.
class ScreenPlaceholder : public juce::Component {
  public:
    explicit ScreenPlaceholder(const juce::String& label)
        : label_(label) {}
    ~ScreenPlaceholder() override = default;

    void paint(juce::Graphics& g) override {
        g.fillAll(findColour(juce::Label::backgroundColourId));
        g.setColour(findColour(juce::Label::textColourId));
        g.setFont(14.0f);
        g.drawText(label_, getLocalBounds(), juce::Justification::centred);
    }

  private:
    juce::String label_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScreenPlaceholder)
};

// BaqueEditor — NAV shell. Owns LookAndFeel, Header, and 6 screen slots.
// Member declaration order is intentional: laf_ must outlive header_ and screens_.
class BaqueEditor : public juce::AudioProcessorEditor {
  public:
    using Screen = HeaderComponent::Screen;
    static constexpr int k_num_screens = HeaderComponent::NUM_SCREENS;

    explicit BaqueEditor(BaqueProcessor& p);
    ~BaqueEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Public for test access (NAV1).
    void showScreen(Screen s);
    [[nodiscard]] bool isScreenVisible(int screen_index) const noexcept;
    [[nodiscard]] Screen activeScreen() const noexcept { return active_screen_; }

    // SR2: type-safe screen accessor for tests — returns the actual component pointer so
    // callers can dynamic_cast to verify the slot holds the expected screen type (not ScreenPlaceholder).
    [[nodiscard]] juce::Component* getScreen(Screen s) const noexcept {
        return screens_[static_cast<int>(s)].get();
    }

    bool keyPressed(const juce::KeyPress& key) override;

  private:
    BaqueLookAndFeel look_and_feel_; // must be first — outlives all children
    HeaderComponent header_;
    std::array<std::unique_ptr<juce::Component>, k_num_screens> screens_;
    Screen active_screen_ = Screen::PERFORM;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueEditor)
};
