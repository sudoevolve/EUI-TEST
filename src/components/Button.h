#pragma once

#include "../EUINEO.h"
#include "../ui/UIBuilder.h"
#include <cmath>
#include <functional>
#include <string>
#include <utility>

namespace EUINEO {

enum class ButtonStyle { Default, Primary, Outline };

class ButtonNode : public UINode {
public:
    class Builder : public UIBuilderBase<ButtonNode, Builder> {
    public:
        Builder(UIContext& context, ButtonNode& node) : UIBuilderBase<ButtonNode, Builder>(context, node) {}

        Builder& text(std::string value) {
            this->node_.text_ = std::move(value);
            return *this;
        }

        Builder& fontSize(float value) {
            this->node_.fontSize_ = value;
            return *this;
        }

        Builder& style(ButtonStyle value) {
            this->node_.style_ = value;
            return *this;
        }

        Builder& onClick(std::function<void()> handler) {
            this->node_.onClick_ = std::move(handler);
            return *this;
        }
    };

    explicit ButtonNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "ButtonNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    void update() override {
        const bool hovered = primitive_.enabled && PrimitiveContains(primitive_, State.mouseX, State.mouseY);
        const float speed = 15.0f * State.deltaTime;

        const float targetHover = hovered ? 1.0f : 0.0f;
        if (std::abs(hoverAnim_ - targetHover) > 0.001f) {
            hoverAnim_ = Lerp(hoverAnim_, targetHover, speed);
            if (std::abs(hoverAnim_ - targetHover) < 0.001f) {
                hoverAnim_ = targetHover;
            }
            markDirty(6.0f);
        }

        const float targetClick = (hovered && State.mouseDown) ? 1.0f : 0.0f;
        if (std::abs(clickAnim_ - targetClick) > 0.001f) {
            clickAnim_ = Lerp(clickAnim_, targetClick, speed * 2.0f);
            if (std::abs(clickAnim_ - targetClick) < 0.001f) {
                clickAnim_ = targetClick;
            }
            markDirty(6.0f);
        }

        if (hovered && State.mouseClicked && onClick_) {
            onClick_();
            markDirty(6.0f, 0.2f);
        }
    }

    void draw() override {
        PrimitiveClipScope clip(primitive_);
        const RectFrame frame = PrimitiveFrame(primitive_);
        const float cornerRadius = primitive_.rounding > 0.0f ? primitive_.rounding : 6.0f;

        const Color baseColor = style_ == ButtonStyle::Primary ? CurrentTheme->primary : CurrentTheme->surface;
        const Color hoverColor = style_ == ButtonStyle::Primary
            ? Lerp(CurrentTheme->primary, Color(1.0f, 1.0f, 1.0f, 1.0f), 0.2f)
            : CurrentTheme->surfaceHover;
        const Color activeColor = style_ == ButtonStyle::Primary
            ? Lerp(CurrentTheme->primary, Color(0.0f, 0.0f, 0.0f, 1.0f), 0.2f)
            : CurrentTheme->surfaceActive;

        Color fillColor = Lerp(baseColor, hoverColor, hoverAnim_);
        fillColor = Lerp(fillColor, activeColor, clickAnim_);
        fillColor = ApplyOpacity(fillColor, primitive_.opacity);

        if (style_ == ButtonStyle::Outline) {
            RectStyle borderStyle = MakeStyle(primitive_);
            borderStyle.color = ApplyOpacity(
                primitive_.borderColor.a > 0.0f ? primitive_.borderColor : CurrentTheme->border,
                primitive_.opacity
            );
            borderStyle.gradient = RectGradient{};
            borderStyle.rounding = cornerRadius;
            Renderer::DrawRect(frame.x, frame.y, frame.width, frame.height, borderStyle);

            RectStyle innerStyle = MakeStyle(primitive_);
            innerStyle.color = ApplyOpacity(
                primitive_.background.a > 0.0f ? primitive_.background : CurrentTheme->background,
                primitive_.opacity
            );
            innerStyle.rounding = std::max(0.0f, cornerRadius - 1.0f);
            Renderer::DrawRect(frame.x + 1.0f, frame.y + 1.0f,
                               frame.width - 2.0f, frame.height - 2.0f, innerStyle);
        } else {
            RectStyle style = MakeStyle(primitive_);
            style.color = fillColor;
            style.rounding = cornerRadius;
            Renderer::DrawRect(frame.x, frame.y, frame.width, frame.height, style);
        }

        const float textScale = fontSize_ / 24.0f;
        const float textWidth = Renderer::MeasureTextWidth(text_, textScale);
        const float textX = frame.x + (frame.width - textWidth) * 0.5f;
        const float textY = frame.y + frame.height * 0.5f + (fontSize_ / 4.0f);
        const Color textColor = ApplyOpacity(
            style_ == ButtonStyle::Primary ? Color(1.0f, 1.0f, 1.0f, 1.0f) : CurrentTheme->text,
            primitive_.opacity
        );
        Renderer::DrawTextStr(text_, textX, textY, textColor, textScale);
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.width = 120.0f;
        primitive_.height = 40.0f;
        text_.clear();
        fontSize_ = 20.0f;
        style_ = ButtonStyle::Default;
        onClick_ = {};
    }

private:
    void markDirty(float expand = 6.0f, float duration = 0.0f) {
        MarkPrimitiveDirty(primitive_, MakeStyle(primitive_), expand, duration);
    }

    std::string text_;
    float fontSize_ = 20.0f;
    ButtonStyle style_ = ButtonStyle::Default;
    std::function<void()> onClick_;
    float hoverAnim_ = 0.0f;
    float clickAnim_ = 0.0f;
};

} // namespace EUINEO
