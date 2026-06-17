#include "../src/plugin_processor.h"
#include "../src/ui/baque_look_and_feel.h"
#include "../src/ui/perf_fx_screen.h"
#include "../src/ui/midi_screen.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("PERF1 - PerfFxScreen constructs without error", "[perf_fx]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    PerfFxScreen scr(proc, laf);
    scr.setSize(1200, 700);
    CHECK(true);
}

TEST_CASE("PERF2 - PerfFxScreen paint smoke", "[perf_fx]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    PerfFxScreen scr(proc, laf);
    scr.setSize(1200, 700);
    juce::Image img(juce::Image::ARGB, 1200, 700, true);
    juce::Graphics g(img);
    scr.paint(g);
    CHECK(true);
}

TEST_CASE("PERF3 - PerfFxScreen XyPad paint smoke", "[perf_fx]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    PerfFxScreen scr(proc, laf);
    scr.setSize(1200, 700);
    scr.resized();
    juce::Image img(juce::Image::ARGB, 1200, 700, true);
    juce::Graphics g(img);
    scr.paint(g);
    CHECK(true);
}

TEST_CASE("PERF4 - MidiScreen constructs without error", "[midi_screen]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    MidiScreen scr(proc, laf);
    scr.setSize(1200, 700);
    CHECK(true);
}

TEST_CASE("PERF5 - MidiScreen timer gates on visibility", "[midi_screen]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    MidiScreen scr(proc, laf);
    scr.setSize(1200, 700);
    CHECK_FALSE(scr.isTimerRunning());
    scr.setVisible(true);
    CHECK(scr.isTimerRunning());
    scr.setVisible(false);
    CHECK_FALSE(scr.isTimerRunning());
}
