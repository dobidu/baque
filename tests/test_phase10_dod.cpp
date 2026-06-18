#include "../src/plugin_processor.h"
#include "../src/plugin_editor.h"
#include "../src/ui/baque_look_and_feel.h"
#include "../src/ui/browser_screen.h"
#include <BinaryData.h>
#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>

TEST_CASE("P10D1 - BrowserScreen constructs without error", "[p10_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueLookAndFeel laf(BaqueLookAndFeel::Theme::barro);
    BrowserScreen scr(proc, laf);
    scr.setSize(1200, 700);
    CHECK(true);
}

TEST_CASE("P10D2 - load_sample_from_file loads WAV to pad", "[p10_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    // Write embedded test_kick.wav to a temp file, then load it.
    juce::TemporaryFile tmp(".wav");
    {
        juce::FileOutputStream os(tmp.getFile());
        os.write(BinaryData::test_kick_wav, BinaryData::test_kick_wavSize);
    }
    proc.load_sample_from_file(1, tmp.getFile());
    CHECK(proc.current_pad(1).num_samples() > 0);
}

TEST_CASE("P10D3 - UndoManager accessible from processor", "[p10_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    // UndoManager starts empty — verify accessible and history is clear.
    CHECK_FALSE(proc.getUndoManager().canUndo());
    CHECK_FALSE(proc.getUndoManager().canRedo());
}

TEST_CASE("P10D4 - BROWSER screen slot is not ScreenPlaceholder", "[p10_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueEditor editor(proc);
    // SR2 fix: isScreenVisible passes even for ScreenPlaceholder — getScreen + dynamic_cast
    // proves the slot holds a BrowserScreen, not a placeholder.
    auto* comp = editor.getScreen(BaqueEditor::Screen::BROWSER);
    REQUIRE(comp != nullptr);
    CHECK(dynamic_cast<BrowserScreen*>(comp) != nullptr);
}

TEST_CASE("P10D5 - showScreen cycles all 6 screens without error", "[p10_dod]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueEditor editor(proc);
    // Cycle all screens; only the target should be visible.
    for (int i = 0; i < BaqueEditor::k_num_screens; ++i) {
        editor.showScreen(static_cast<BaqueEditor::Screen>(i));
        for (int j = 0; j < BaqueEditor::k_num_screens; ++j)
            CHECK(editor.isScreenVisible(j) == (j == i));
    }
}
