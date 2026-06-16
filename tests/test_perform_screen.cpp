#include "../src/plugin_editor.h"
#include "../src/plugin_processor.h"
#include "../src/ui/sample_editor_component.h"
#include "../src/ui/sequencer_component.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("PERF1 - PerformScreen constructs; PERFORM slot is visible", "[perform]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueEditor editor(proc);
    CHECK(editor.activeScreen() == BaqueEditor::Screen::PERFORM);
    CHECK(editor.isScreenVisible(static_cast<int>(BaqueEditor::Screen::PERFORM)));
    CHECK_FALSE(editor.isScreenVisible(static_cast<int>(BaqueEditor::Screen::FX)));
}

TEST_CASE("PERF2 - SequencerComponent toggleStep pushes command", "[perform]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    SequencerComponent seq(proc, laf);
    seq.setSize(800, 300);
    const bool pushed = seq.toggleStep(0, 0);
    CHECK(pushed == true);
}

TEST_CASE("PERF3 - SampleEditorComponent setPlayMode pushes command", "[perform]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    SampleEditorComponent ed(proc, laf);
    ed.setSize(260, 440);
    ed.setFocusedPad(0);
    CHECK(ed.setPlayMode(PlayMode::gate) == true);
}
