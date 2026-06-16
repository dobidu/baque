#include "plugin_editor.h"
#include "plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("NAV1 - showScreen switches visibility; only target screen is visible", "[nav]") {
    juce::ScopedJuceInitialiser_GUI init;
    BaqueProcessor proc;
    BaqueEditor editor(proc);

    editor.showScreen(BaqueEditor::Screen::FX);

    CHECK(editor.isScreenVisible(static_cast<int>(BaqueEditor::Screen::FX)) == true);
    CHECK(editor.isScreenVisible(static_cast<int>(BaqueEditor::Screen::PERFORM)) == false);
    CHECK(editor.isScreenVisible(static_cast<int>(BaqueEditor::Screen::MIX)) == false);
    CHECK(editor.activeScreen() == BaqueEditor::Screen::FX);
}
