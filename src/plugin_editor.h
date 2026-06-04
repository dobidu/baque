#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class BaqueProcessor;

// Editor placeholder — visual completo implementado na Fase 10.
class BaqueEditor : public juce::AudioProcessorEditor {
  public:
    explicit BaqueEditor(BaqueProcessor& p);
    ~BaqueEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    juce::Label title_label_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueEditor)
};
