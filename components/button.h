#pragma once

#include "components/theme.h"
#include "core/dsl.h"

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

namespace components {

struct ButtonStyle {
    ButtonStyle() : ButtonStyle(theme::DarkThemeColors()) {}

    explicit ButtonStyle(const theme::ThemeColorTokens& tokens, bool primary = true) {
        const core::Color base = primary ? tokens.primary : tokens.surface;
        normal = base;
        hover = theme::buttonHover(tokens, base);
        pressed = theme::buttonPressed(tokens, base);
        text = primary || tokens.dark ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : tokens.text;
        icon = text;
        border = theme::buttonBorder(tokens, primary);
        shadow = theme::buttonShadow(tokens);
    }

    core::Color normal;
    core::Color hover;
    core::Color pressed;
    core::Color text;
    core::Color icon;
    core::Border border;
    core::Shadow shadow;
    float radius = 16.0f;
    float opacity = 1.0f;
    float pressScale = 0.965f;
};

class ButtonBuilder {
public:
    ButtonBuilder(core::dsl::Ui& ui, std::string id)
        : ui_(ui), id_(std::move(id)) {}

    ButtonBuilder& size(float width, float height) { width_ = width; height_ = height; return *this; }
    ButtonBuilder& scale(float value) { scale_ = value; return *this; }
    ButtonBuilder& text(const std::string& value) { text_ = value; return *this; }
    ButtonBuilder& icon(unsigned int codepoint) { icon_ = core::dsl::utf8(codepoint); return *this; }
    ButtonBuilder& icon(const std::string& value) { icon_ = value; return *this; }
    ButtonBuilder& fontSize(float value) { fontSize_ = value; return *this; }
    ButtonBuilder& iconSize(float value) { iconSize_ = value; return *this; }
    ButtonBuilder& textColor(const core::Color& value) { style_.text = value; return *this; }
    ButtonBuilder& iconColor(const core::Color& value) { style_.icon = value; return *this; }
    ButtonBuilder& style(const ButtonStyle& value) { style_ = value; return *this; }
    ButtonBuilder& theme(const theme::ThemeColorTokens& tokens, bool primary = true) {
        style_ = ButtonStyle(tokens, primary);
        return *this;
    }
    ButtonBuilder& primaryTheme(const theme::ThemeColorTokens& tokens) { return theme(tokens, true); }
    ButtonBuilder& secondaryTheme(const theme::ThemeColorTokens& tokens) { return theme(tokens, false); }
    ButtonBuilder& radius(float value) { style_.radius = value; return *this; }
    ButtonBuilder& rounding(float value) { return radius(value); }
    ButtonBuilder& opacity(float value) { style_.opacity = std::clamp(value, 0.0f, 1.0f); return *this; }
    ButtonBuilder& pressScale(float value) { style_.pressScale = std::clamp(value, 0.80f, 1.0f); return *this; }
    ButtonBuilder& border(float width, const core::Color& color) { style_.border = {width, color}; return *this; }
    ButtonBuilder& shadow(float blur, float offsetX, float offsetY, const core::Color& color) {
        style_.shadow = {true, {offsetX, offsetY}, blur, 0.0f, color};
        return *this;
    }
    ButtonBuilder& colors(const core::Color& normal, const core::Color& hover, const core::Color& pressed) {
        style_.normal = normal;
        style_.hover = hover;
        style_.pressed = pressed;
        return *this;
    }
    ButtonBuilder& transition(const core::Transition& value) { transition_ = value; return *this; }
    ButtonBuilder& transition(float duration, core::Ease ease = core::Ease::OutCubic) {
        transition_ = core::Transition::make(duration, ease);
        return *this;
    }
    ButtonBuilder& onClick(std::function<void()> callback) { onClick_ = std::move(callback); return *this; }

    void build() {
        const float w = width_ * scale_;
        const float h = height_ * scale_;
        const float font = fontSize_ > 0.0f ? fontSize_ * scale_ : h * 0.46f;
        const float iconFont = iconSize_ > 0.0f ? iconSize_ * scale_ : font * 0.92f;
        const bool hasIcon = !icon_.empty();
        const float iconWidth = hasIcon ? iconFont * 1.15f : 0.0f;
        const float gap = hasIcon ? std::max(6.0f * scale_, h * 0.12f) : 0.0f;
        const float labelWidth = hasIcon ? std::max(0.0f, w - iconWidth - gap - 32.0f * scale_) : w;

        core::Border border = style_.border;
        border.width *= scale_;

        core::Shadow shadow = style_.shadow;
        shadow.offset.x *= scale_;
        shadow.offset.y *= scale_;
        shadow.blur *= scale_;
        shadow.spread *= scale_;

        ui_.stack(id_)
            .size(w, h)
            .visualStateFrom(id_ + ".bg", style_.pressScale)
            .content([&] {
                ui_.rect(id_ + ".bg")
                    .size(w, h)
                    .states(style_.normal, style_.hover, style_.pressed)
                    .radius(style_.radius * scale_)
                    .opacity(style_.opacity)
                    .border(border)
                    .shadow(shadow)
                    .transition(transition_)
                    .onClick(onClick_)
                    .build();

                ui_.row(id_ + ".content")
                    .size(w, h)
                    .gap(gap)
                    .justifyContent(core::Align::CENTER)
                    .alignItems(core::Align::CENTER)
                    .content([&] {
                        if (hasIcon) {
                            ui_.text(id_ + ".icon")
                                .size(iconWidth, h)
                                .icon(icon_)
                                .fontSize(iconFont)
                                .lineHeight(iconFont * 1.18f)
                                .color(style_.icon)
                                .horizontalAlign(core::HorizontalAlign::Center)
                                .verticalAlign(core::VerticalAlign::Center)
                                .build();
                        }

                        ui_.text(id_ + ".text")
                            .size(labelWidth, h)
                            .text(text_)
                            .fontSize(font)
                            .lineHeight(font * 1.18f)
                            .color(style_.text)
                            .horizontalAlign(hasIcon ? core::HorizontalAlign::Left : core::HorizontalAlign::Center)
                            .verticalAlign(core::VerticalAlign::Center)
                            .build();
                    })
                    .build();
            })
            .build();
    }

private:
    core::dsl::Ui& ui_;
    std::string id_;
    std::string text_ = "Button";
    std::string icon_;
    ButtonStyle style_;
    core::Transition transition_;
    std::function<void()> onClick_;
    float width_ = 240.0f;
    float height_ = 70.0f;
    float scale_ = 1.0f;
    float fontSize_ = 0.0f;
    float iconSize_ = 0.0f;
};

inline ButtonBuilder button(core::dsl::Ui& ui, const std::string& id) {
    return ButtonBuilder(ui, id);
}

} // namespace components
