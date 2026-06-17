#pragma once

#include "../plugin_processor.h"
#include "baque_look_and_feel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class PadGridComponent;
class SequencerComponent;
class SampleEditorComponent;
class FeelFieldComponent;

class PerformScreen : public juce::Component {
  public:
    explicit PerformScreen(BaqueProcessor& proc, BaqueLookAndFeel& laf);
    ~PerformScreen() override;

    void resized() override;

    void setFocusedPad(int pad_idx) noexcept;
    [[nodiscard]] int focusedPad() const noexcept { return focused_pad_; }

  private:
    BaqueProcessor& proc_;
    BaqueLookAndFeel& laf_;
    int focused_pad_ = 0;

    std::unique_ptr<PadGridComponent> pad_grid_;
    std::unique_ptr<SequencerComponent> sequencer_;
    std::unique_ptr<SampleEditorComponent> sample_editor_;
    std::unique_ptr<FeelFieldComponent> feel_field_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformScreen)
};
