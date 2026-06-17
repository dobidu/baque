#include "../src/plugin_processor.h"
#include "../src/ui/baque_look_and_feel.h"
#include "../src/ui/fx_screen.h"
#include "../src/ui/mix_screen.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("FX1 - FxScreen constructs and paint executes without error", "[fx]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    FxScreen fx(proc, laf);
    fx.setSize(1200, 700);
    juce::Image img(juce::Image::ARGB, 1200, 700, true);
    juce::Graphics g(img);
    fx.paint(g);
    CHECK(true);
}

TEST_CASE("MIX1 - MixScreen timer gates on visibility", "[mix]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    MixScreen mix(proc, laf);
    mix.setSize(1200, 700);
    CHECK_FALSE(mix.isTimerRunning());
    mix.setVisible(true);
    CHECK(mix.isTimerRunning());
    mix.setVisible(false);
    CHECK_FALSE(mix.isTimerRunning());
}

TEST_CASE("MIX2 - MixScreen paint executes without error", "[mix]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    MixScreen mix(proc, laf);
    mix.setSize(1200, 700);
    juce::Image img(juce::Image::ARGB, 1200, 700, true);
    juce::Graphics g(img);
    mix.paint(g);
    CHECK(true);
}
