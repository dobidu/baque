#include "../src/plugin_editor.h"
#include "../src/plugin_processor.h"
#include "../src/ui/feel_field_component.h"
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

TEST_CASE("FEEL1 - FeelFieldComponent constructs and timer gates on visibility", "[perform]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    FeelFieldComponent ff(proc, laf);
    ff.setSize(300, 300);
    CHECK_FALSE(ff.isTimerRunning()); // not running before visible
    ff.setVisible(true);
    CHECK(ff.isTimerRunning()); // running when visible
    ff.setVisible(false);
    CHECK_FALSE(ff.isTimerRunning()); // stopped when hidden
}

TEST_CASE("FEEL2 - FeelFieldComponent paint executes without error with active steps", "[perform]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    FeelFieldComponent ff(proc, laf);
    ff.setSize(300, 300);

    // Activate step 0 on lane 0 so the node-drawing branch executes
    StepPattern pat;
    pat.set_active(0, 0, true);
    pat.set_velocity(0, 0, 100);
    proc.set_pattern(pat);

    // Paint onto an off-screen image — any UB/OOB in nodePos() or paint() will crash here
    juce::Image img(juce::Image::ARGB, 300, 300, true);
    juce::Graphics g(img);
    ff.paint(g); // must not throw or crash
    CHECK(true); // reaching here = no crash
}
