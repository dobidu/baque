#include "plugin_editor.h"

#include "plugin_processor.h"

BaqueEditor::BaqueEditor(BaqueProcessor& p)
    : AudioProcessorEditor(&p) {
    // Tamanho inicial — redimensionável na Fase 10
    setSize(800, 600);
    setResizable(true, true);
    setResizeLimits(600, 400, 2560, 1440);

    title_label_.setText("BAQUE", juce::dontSendNotification);
    title_label_.setFont(juce::Font(48.0f, juce::Font::bold));
    title_label_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(title_label_);
}

void BaqueEditor::paint(juce::Graphics& g) {
    // Fundo escuro quente — paleta "Barro" (Fase 10 aplica design system completo)
    g.fillAll(juce::Colour(0xff1a1410));
    title_label_.setColour(juce::Label::textColourId, juce::Colour(0xffe8502e)); // ember
}

void BaqueEditor::resized() {
    title_label_.setBounds(getLocalBounds());
}
