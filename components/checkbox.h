#pragma once

#include "components/theme.h"
#include "core/dsl.h"

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

namespace components {

struct CheckboxStyle {
    CheckboxStyle() : CheckboxStyle(theme::DarkThemeColors()) {}

    explicit CheckboxStyle(const theme::ThemeColorTokens& tokens) {
        box = tokens.surface;
        boxHover = tokens.surfaceHover;
        checked = tokens.primary;
        checkedHover = theme::buttonHover(tokens, checked);
        checkedPressed = theme::buttonPressed(tokens, checked);
        boxPressed = theme::buttonPressed(tokens, boxHover);
        border = tokens.border;
        mark = theme::color(1.0f, 1.0f, 1.0f, 1.0f);
        text = tokens.text;
        rowHover = theme::withAlpha(tokens.text, tokens.dark ? 0.06f : 0.05f);
        rowPressed = theme::withAlpha(tokens.text, tokens.dark ? 0.10f : 0.08f);
    }

    core::Color box;
    core::Color boxHover;
    core::Color boxPressed;
    core::Color checked;
    core::Color checkedHover;
    core::Color checkedPressed;
    core::Color border;
    core::Color mark;
    core::Color text;
    core::Color rowHover;
    core::Color rowPressed;
    float radius = 6.0f;
};

class CheckboxBuilder {
public:
    CheckboxBuilder(core::dsl::Ui& ui, std::string id)
        : ui_(ui), id_(std::move(id)) {}

    CheckboxBuilder& size(float width, float height) { width_ = width; height_ = height; return *this; }
    CheckboxBuilder& checked(bool value) { checked_ = value; return *this; }
    CheckboxBuilder& text(std::string value) { text_ = std::move(value); return *this; }
    CheckboxBuilder& fontSize(float value) { fontSize_ = std::max(1.0f, value); return *this; }
    CheckboxBuilder& boxSize(float value) { boxSize_ = std::max(10.0f, value); return *this; }
    CheckboxBuilder& style(const CheckboxStyle& value) { style_ = value; return *this; }
    CheckboxBuilder& theme(const theme::ThemeColorTokens& tokens) { style_ = CheckboxStyle(tokens); return *this; }
    CheckboxBuilder& transition(const core::Transition& value) { transition_ = value; return *this; }
    CheckboxBuilder& transition(float duration, core::Ease ease = core::Ease::OutCubic) {
        transition_ = core::Transition::make(duration, ease);
        return *this;
    }
    CheckboxBuilder& onChange(std::function<void(bool)> callback) { onChange_ = std::move(callback); return *this; }

    void build() {
        const float box = std::min(boxSize_, height_);
        const float boxY = (height_ - box) * 0.5f;
        const float labelX = box + gap_;
        const float labelWidth = std::max(0.0f, width_ - labelX);
        const float labelLineHeight = fontSize_;
        const float labelY = std::max(0.0f, (height_ - labelLineHeight) * 0.5f);
        const float markThickness = std::max(2.0f, box * 0.12f);
        const float markAngle = 0.78f;
        const float markAngleCos = 0.7109f;
        const float markAngleSin = 0.7033f;
        const float markShort = checked_ ? box * 0.28f : 0.0f;
        const float markLong = checked_ ? box * 0.46f : 0.0f;
        const float markOverlap = markThickness * 0.70f;
        const float markStartX = box * 0.26f;
        const float markStartY = box * 0.55f;
        const float markCornerX = markStartX + box * 0.28f * markAngleCos;
        const float markCornerY = markStartY + box * 0.28f * markAngleSin;
        const float markLongX = markCornerX - markOverlap * markAngleCos;
        const float markLongY = markCornerY + markOverlap * markAngleSin;
        core::Transition markShortTransition = checked_ ? transition_ : core::Transition::none();
        markShortTransition.durationSeconds = 0.09f;
        markShortTransition.delaySeconds = 0.0f;
        markShortTransition.ease = core::Ease::OutCubic;
        core::Transition markLongTransition = checked_ ? transition_ : core::Transition::none();
        markLongTransition.durationSeconds = 0.12f;
        markLongTransition.delaySeconds = 0.08f;
        markLongTransition.ease = core::Ease::OutCubic;
        const float hitWidth = text_.empty()
            ? box
            : std::min(width_, labelX + textWidth(text_, fontSize_));
        const core::Color idle = checked_ ? style_.checked : style_.box;
        const core::Color hover = checked_ ? style_.checkedHover : style_.boxHover;
        const core::Color pressed = checked_ ? style_.checkedPressed : style_.boxPressed;
        const bool nextChecked = !checked_;
        const std::function<void(bool)> onChange = onChange_;

        ui_.stack(id_)
            .size(width_, height_)
            .visualStateFrom(id_ + ".hit", 0.985f)
            .content([&] {
                ui_.rect(id_ + ".hit")
                    .size(hitWidth, height_)
                    .states(theme::color(0.0f, 0.0f, 0.0f, 0.0f), style_.rowHover, style_.rowPressed)
                    .radius(std::max(6.0f, height_ * 0.20f))
                    .transition(transition_)
                    .onClick([onChange, nextChecked] {
                        if (onChange) {
                            onChange(nextChecked);
                        }
                    })
                    .build();

                ui_.rect(id_ + ".box")
                    .y(boxY)
                    .size(box, box)
                    .color(idle)
                    .radius(style_.radius)
                    .border(1.5f, checked_ ? style_.checked : style_.border)
                    .transition(transition_)
                    .animate(core::AnimProperty::Color | core::AnimProperty::Border)
                    .build();

                ui_.stack(id_ + ".mark.clip")
                    .y(boxY)
                    .size(box, box)
                    .clip()
                    .content([&] {
                        ui_.rect(id_ + ".mark.short")
                            .x(markStartX)
                            .y(markStartY - markThickness * 0.5f)
                            .size(markShort, markThickness)
                            .color(style_.mark)
                            .radius(markThickness * 0.5f)
                            .opacity(checked_ ? 1.0f : 0.0f)
                            .rotate(markAngle)
                            .transformOrigin(0.0f, 0.5f)
                            .transition(markShortTransition)
                            .animate(core::AnimProperty::Frame | core::AnimProperty::Opacity)
                            .build();

                        ui_.rect(id_ + ".mark.long")
                            .x(markLongX)
                            .y(markLongY - markThickness * 0.5f)
                            .size(markLong + markOverlap, markThickness)
                            .color(style_.mark)
                            .radius(markThickness * 0.5f)
                            .opacity(checked_ ? 1.0f : 0.0f)
                            .rotate(-markAngle)
                            .transformOrigin(0.0f, 0.5f)
                            .transition(markLongTransition)
                            .animate(core::AnimProperty::Frame | core::AnimProperty::Opacity)
                            .build();
                    })
                    .build();

                if (!text_.empty()) {
                    ui_.text(id_ + ".label")
                        .x(labelX)
                        .y(labelY)
                        .size(labelWidth, labelLineHeight)
                        .text(text_)
                        .fontSize(fontSize_)
                        .lineHeight(labelLineHeight)
                        .color(style_.text)
                        .verticalAlign(core::VerticalAlign::Top)
                        .build();
                }
            })
            .build();
    }

private:
    static float textWidth(const std::string& value, float fontSize) {
        return core::TextPrimitive::measureTextWidth(value, {}, fontSize, 400);
    }

    core::dsl::Ui& ui_;
    std::string id_;
    CheckboxStyle style_;
    core::Transition transition_ = core::Transition::make(0.16f, core::Ease::OutCubic);
    std::function<void(bool)> onChange_;
    std::string text_;
    bool checked_ = false;
    float width_ = 180.0f;
    float height_ = 28.0f;
    float boxSize_ = 22.0f;
    float gap_ = 10.0f;
    float fontSize_ = 18.0f;
};

inline CheckboxBuilder checkbox(core::dsl::Ui& ui, const std::string& id) {
    return CheckboxBuilder(ui, id);
}

} // namespace components
