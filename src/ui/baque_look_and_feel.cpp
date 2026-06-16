#include "baque_look_and_feel.h"

#include <BinaryData.h>
#include <cmath>

BaqueLookAndFeel::BaqueLookAndFeel(Theme t)
    : theme_(t) {
    loadTypefaces();
    buildGrainTexture();
    setTheme(t);
}

void BaqueLookAndFeel::loadTypefaces() {
    ui_typeface_ = juce::Typeface::createSystemTypefaceFor(BinaryData::SpaceGroteskMedium_ttf,
                                                           static_cast<size_t>(BinaryData::SpaceGroteskMedium_ttfSize));
    jassert(ui_typeface_ != nullptr);

    mono_typeface_ = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoRegular_ttf, static_cast<size_t>(BinaryData::JetBrainsMonoRegular_ttfSize));
    jassert(mono_typeface_ != nullptr);
}

void BaqueLookAndFeel::buildGrainTexture() {
    // Pre-bake a 256x256 noise image with a fixed seed — static film-grain aesthetic.
    // Called once in constructor; zero per-paint allocation in paintGrainOverlay().
    grain_texture_ = juce::Image(juce::Image::ARGB, 256, 256, true);
    juce::Random rng(static_cast<juce::int64>(0x6261717565LL)); // "baque"
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            const auto v = static_cast<juce::uint8>(200 + rng.nextInt(56)); // 200-255
            const auto a = static_cast<juce::uint8>(rng.nextInt(40));       // 0-39
            grain_texture_.setPixelAt(x, y, juce::Colour(v, v, v, a));
        }
    }
}

void BaqueLookAndFeel::setTheme(Theme t) {
    theme_ = t;
    const auto bg = background();
    const auto surf = surface();
    const auto txt = text();

    setColour(juce::ResizableWindow::backgroundColourId, bg);
    setColour(juce::DocumentWindow::backgroundColourId, bg);
    setColour(juce::Label::textColourId, txt); // required for ScreenPlaceholder findColour()
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonColourId, surf);
    setColour(juce::TextButton::buttonOnColourId, surf);
    setColour(juce::TextButton::textColourOffId, txt);
    setColour(juce::TextButton::textColourOnId, bone());
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffaaaaaa));
    setColour(juce::Slider::trackColourId, surf);
    setColour(juce::Slider::backgroundColourId, bg);

    if (ui_typeface_ != nullptr)
        setDefaultSansSerifTypeface(ui_typeface_);
}

juce::Colour BaqueLookAndFeel::background() const noexcept {
    switch (theme_) {
    case Theme::barro:
        return juce::Colour(0xff1a1410u);
    case Theme::cinza:
        return juce::Colour(0xff1a1a1au);
    case Theme::maracatu:
        return juce::Colour(0xff0f0a18u);
    case Theme::papel:
        return juce::Colour(0xfff5f0e8u);
    default:
        return juce::Colour(0xff1a1410u);
    }
}

juce::Colour BaqueLookAndFeel::surface() const noexcept {
    switch (theme_) {
    case Theme::barro:
        return juce::Colour(0xff241c16u);
    case Theme::cinza:
        return juce::Colour(0xff252525u);
    case Theme::maracatu:
        return juce::Colour(0xff1a1025u);
    case Theme::papel:
        return juce::Colour(0xffede5d8u);
    default:
        return juce::Colour(0xff241c16u);
    }
}

juce::Colour BaqueLookAndFeel::text() const noexcept {
    switch (theme_) {
    case Theme::barro:
        return juce::Colour(0xfff2e9ddu);
    case Theme::cinza:
        return juce::Colour(0xffe8e8e0u);
    case Theme::maracatu:
        return juce::Colour(0xfff0e0ffu);
    case Theme::papel:
        return juce::Colour(0xff1a1410u);
    default:
        return juce::Colour(0xfff2e9ddu);
    }
}

void BaqueLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                        int x,
                                        int y,
                                        int width,
                                        int height,
                                        float sliderPosProportional,
                                        float rotaryStartAngle,
                                        float rotaryEndAngle,
                                        juce::Slider& /*slider*/) {
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float r = std::min(static_cast<float>(width), static_cast<float>(height)) * 0.5f - 4.0f;
    if (r <= 0.0f)
        return;

    // Body
    g.setColour(surface().darker(0.3f));
    g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    g.setColour(surface());
    g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f);

    const float trackW = std::max(2.0f, r * 0.12f);
    const float arcR = r - trackW;

    // Track (full range)
    {
        juce::Path track;
        track.addArc(cx - arcR, cy - arcR, arcR * 2.0f, arcR * 2.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(surface().brighter(0.4f));
        g.strokePath(track, juce::PathStrokeType(trackW));
    }

    // Value arc
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    if (angle > rotaryStartAngle) {
        juce::Path valueArc;
        valueArc.addArc(cx - arcR, cy - arcR, arcR * 2.0f, arcR * 2.0f, rotaryStartAngle, angle, true);
        g.setColour(ember());
        g.strokePath(valueArc, juce::PathStrokeType(trackW));
    }

    // Indicator dot at current position
    const float dotR = std::max(2.0f, trackW * 0.8f);
    const float dotX = cx + arcR * std::sin(angle);
    const float dotY = cy - arcR * std::cos(angle);
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
}

void BaqueLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                        int x,
                                        int y,
                                        int width,
                                        int height,
                                        float sliderPos,
                                        float /*minSliderPos*/,
                                        float /*maxSliderPos*/,
                                        juce::Slider::SliderStyle style,
                                        juce::Slider& slider) {
    if (style == juce::Slider::LinearVertical) {
        const float trackW = 4.0f;
        const float trackX = static_cast<float>(x + width / 2) - trackW * 0.5f;
        const float trackTop = static_cast<float>(y + 4);
        const float trackBottom = static_cast<float>(y + height - 4);

        g.setColour(surface().brighter(0.3f));
        g.fillRoundedRectangle(trackX, trackTop, trackW, trackBottom - trackTop, 2.0f);

        const float thumbH = 18.0f;
        const float thumbW = static_cast<float>(width - 8);
        const float thumbX = static_cast<float>(x + 4);
        const float thumbY = sliderPos - thumbH * 0.5f;

        juce::ColourGradient grad(
            juce::Colour(0xffcccccc), thumbX, thumbY, juce::Colour(0xff666666), thumbX, thumbY + thumbH, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 3.0f);
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 3.0f, 1.0f);
    } else {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0.0f, 1.0f, style, slider);
    }
}

void BaqueLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                            juce::Button& button,
                                            const juce::Colour& /*backgroundColour*/,
                                            bool isMouseOverButton,
                                            bool isButtonDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto col = surface();
    if (isMouseOverButton || isButtonDown)
        col = col.brighter(0.15f);
    g.setColour(col);
    g.fillRoundedRectangle(bounds, 3.0f);
    if (button.getToggleState()) {
        g.setColour(ember().withAlpha(0.9f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    }
}

void BaqueLookAndFeel::drawButtonText(juce::Graphics& g,
                                      juce::TextButton& button,
                                      bool /*isMouseOverButton*/,
                                      bool /*isButtonDown*/) {
    juce::Font f = (ui_typeface_ != nullptr) ? juce::Font(juce::FontOptions(ui_typeface_).withHeight(10.0f))
                                             : juce::Font(juce::FontOptions{}.withHeight(10.0f));
    g.setFont(f);
    g.setColour(button.getToggleState() ? bone() : text());
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(2), juce::Justification::centred, 1);
}

void BaqueLookAndFeel::paintGrainOverlay(juce::Graphics& g, juce::Rectangle<int> bounds, float alpha) const {
    if (!grain_texture_.isValid() || grain_intensity_ <= 0.0f)
        return;
    g.setOpacity(grain_intensity_ * alpha * 0.08f);
    g.drawImage(grain_texture_, bounds.toFloat(), juce::RectanglePlacement::fillDestination);
    g.setOpacity(1.0f);
}
