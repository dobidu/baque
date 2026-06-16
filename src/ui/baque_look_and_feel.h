#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Design system for BAQUE — 4 themes, 5 role colours, custom knob/fader/button/meter rendering.
// Instantiated once by BaqueEditor; propagated to all children via setLookAndFeel() inheritance.
class BaqueLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    enum class Theme { barro, cinza, maracatu, papel };

    // Role colours — brand constants, same across all themes
    static constexpr uint32_t k_ember  = 0xffe8502eu;
    static constexpr uint32_t k_ocre   = 0xffe6b34du;
    static constexpr uint32_t k_clay   = 0xffd98a4bu;
    static constexpr uint32_t k_bone   = 0xfff2e9ddu;
    static constexpr uint32_t k_indigo = 0xff5b86a8u;

    explicit BaqueLookAndFeel(Theme t = Theme::barro);
    ~BaqueLookAndFeel() override = default;

    void setTheme(Theme t);
    [[nodiscard]] Theme theme() const noexcept { return theme_; }

    // Per-theme structural colours
    [[nodiscard]] juce::Colour background()   const noexcept;
    [[nodiscard]] juce::Colour surface()      const noexcept;
    [[nodiscard]] juce::Colour text()         const noexcept;
    [[nodiscard]] juce::Colour accent()       const noexcept { return juce::Colour(k_ember); }

    // Static role colour accessors
    [[nodiscard]] static juce::Colour ember()  noexcept { return juce::Colour(k_ember); }
    [[nodiscard]] static juce::Colour ocre()   noexcept { return juce::Colour(k_ocre); }
    [[nodiscard]] static juce::Colour clay()   noexcept { return juce::Colour(k_clay); }
    [[nodiscard]] static juce::Colour bone()   noexcept { return juce::Colour(k_bone); }
    [[nodiscard]] static juce::Colour indigo() noexcept { return juce::Colour(k_indigo); }

    // Grain overlay intensity: 0=off, 1=max. Adjusted from Tweaks panel.
    void  setGrainIntensity(float g) noexcept { grain_intensity_ = juce::jlimit(0.0f, 1.0f, g); }
    [[nodiscard]] float grainIntensity() const noexcept { return grain_intensity_; }

    // Typeface accessors
    [[nodiscard]] juce::Typeface::Ptr uiTypeface()   const noexcept { return ui_typeface_; }
    [[nodiscard]] juce::Typeface::Ptr monoTypeface() const noexcept { return mono_typeface_; }

    // ---- LookAndFeel_V4 overrides ----

    // Rotary: dark body + coloured value arc + white indicator dot
    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    // Linear vertical: track + metal-cap gradient thumb
    void drawLinearSlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    // Buttons: surface fill + ember outline when toggled (NAV active state)
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override;

    void drawButtonText(juce::Graphics& g,
                        juce::TextButton& button,
                        bool isMouseOverButton,
                        bool isButtonDown) override;

    // Call from any component's paint() to add grain texture.
    // Pre-baked static image — zero per-paint allocation.
    void paintGrainOverlay(juce::Graphics& g,
                           juce::Rectangle<int> bounds,
                           float alpha = 1.0f) const;

  private:
    Theme theme_;
    float grain_intensity_ = 0.3f;
    juce::Image grain_texture_; // 256x256 ARGB, generated once at construction
    juce::Typeface::Ptr ui_typeface_;   // Space Grotesk Medium
    juce::Typeface::Ptr mono_typeface_; // JetBrains Mono Regular

    void loadTypefaces();
    void buildGrainTexture(); // fixed seed → static film-grain aesthetic

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaqueLookAndFeel)
};
