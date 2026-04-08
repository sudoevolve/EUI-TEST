#pragma once

#include "../EUINEO.h"
#include "../ui/ThemeTokens.h"
#include "../ui/UIBuilder.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>

namespace EUINEO {

class ToastNode : public UINode {
public:
    class Builder : public UIBuilderBase<ToastNode, Builder> {
    public:
        Builder(UIContext& context, ToastNode& node) : UIBuilderBase<ToastNode, Builder>(context, node) {}

        Builder& x(float value) {
            UIBuilderBase<ToastNode, Builder>::x(value);
            this->node_.trackComposeValue("toastX", value);
            this->node_.hasExplicitPosition_ = true;
            return *this;
        }

        Builder& y(float value) {
            UIBuilderBase<ToastNode, Builder>::y(value);
            this->node_.trackComposeValue("toastY", value);
            this->node_.hasExplicitPosition_ = true;
            return *this;
        }

        Builder& position(float xValue, float yValue) {
            UIBuilderBase<ToastNode, Builder>::position(xValue, yValue);
            this->node_.trackComposeValue("toastPositionX", xValue);
            this->node_.trackComposeValue("toastPositionY", yValue);
            this->node_.hasExplicitPosition_ = true;
            return *this;
        }

        Builder& message(std::string value) {
            this->node_.trackComposeValue("message", value);
            this->node_.message_ = std::move(value);
            return *this;
        }

        Builder& show(bool value) {
            this->node_.trackComposeValue("show", value);
            this->node_.showRequested_ = value;
            return *this;
        }

        Builder& duration(float seconds) {
            this->node_.trackComposeValue("duration", seconds);
            this->node_.durationSeconds_ = std::max(0.1f, seconds);
            return *this;
        }

        Builder& autoHide(bool value) {
            this->node_.trackComposeValue("autoHide", value);
            this->node_.autoHide_ = value;
            return *this;
        }

        Builder& fixedToWindowBottomCenter(bool value) {
            this->node_.trackComposeValue("fixedToWindowBottomCenter", value);
            this->node_.fixedToWindowBottomCenter_ = value;
            return *this;
        }

        Builder& onClose(std::function<void()> handler) {
            this->node_.onClose_ = std::move(handler);
            return *this;
        }
    };

    explicit ToastNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "ToastNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    bool usesCachedSurface() const override {
        return false;
    }

    bool wantsContinuousUpdate() const override {
        if (visibilityAnim_ > 0.001f && visibilityAnim_ < 0.999f) {
            return true;
        }
        if (visible_ && autoHide_ && remainingSeconds_ > 0.0f) {
            return true;
        }
        return false;
    }

    RectFrame paintBounds() const override {
        if (visibilityAnim_ <= 0.001f && !visible_) {
            return RectFrame{};
        }
        return toastFrame();
    }

    void update() override {
        if (showRequested_ && !lastShowRequested_) {
            visible_ = true;
            remainingSeconds_ = durationSeconds_;
            dismissedByTimer_ = false;
            requestVisualRepaint(0.14f);
        } else if (!showRequested_ && lastShowRequested_ && !autoHide_) {
            visible_ = false;
            requestVisualRepaint(0.14f);
        }

        if (visible_ && autoHide_) {
            remainingSeconds_ = std::max(0.0f, remainingSeconds_ - State.deltaTime);
            if (remainingSeconds_ <= 0.0f) {
                visible_ = false;
                if (!dismissedByTimer_) {
                    dismissedByTimer_ = true;
                    if (onClose_) {
                        onClose_();
                    }
                }
                requestVisualRepaint(0.14f);
            }
        }

        if (primitive_.enabled && visible_ && State.mouseClicked && contains(toastFrame(), State.mouseX, State.mouseY)) {
            visible_ = false;
            State.mouseClicked = false;
            if (onClose_) {
                onClose_();
            }
            requestVisualRepaint(0.14f);
        }

        if (animateTowards(visibilityAnim_, visible_ ? 1.0f : 0.0f, State.deltaTime * 13.0f)) {
            requestVisualRepaint(0.14f);
        }

        lastShowRequested_ = showRequested_;
    }

    void draw() override {
        if (visibilityAnim_ <= 0.001f || message_.empty()) {
            return;
        }

        const RectFrame frame = toastFrame();
        const float reveal = std::clamp(visibilityAnim_, 0.0f, 1.0f);
        const float lift = (1.0f - reveal) * 12.0f;
        UIPrimitive popupPrimitive = primitive_;
        popupPrimitive.opacity = primitive_.opacity * reveal;
        RectStyle style = MakePopupChromeStyle(popupPrimitive, primitive_.rounding > 0.0f ? primitive_.rounding : 10.0f);
        style.color = ApplyOpacity(Lerp(CurrentTheme->surface, CurrentTheme->surfaceHover, 0.35f), popupPrimitive.opacity);
        style.shadowColor = ApplyOpacity(style.shadowColor, popupPrimitive.opacity);
        Renderer::DrawRect(frame.x, frame.y + lift, frame.width, frame.height, style);

        const float textScale = fontSize_ / 24.0f;
        const float textWidth = Renderer::MeasureTextWidth(message_, textScale);
        const float textX = frame.x + (frame.width - textWidth) * 0.5f;
        const float textY = frame.y + lift + frame.height * 0.5f + fontSize_ * 0.24f;
        Renderer::DrawTextStr(message_, textX, textY, ApplyOpacity(CurrentTheme->text, popupPrimitive.opacity), textScale);
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.renderLayer = RenderLayer::Popup;
        primitive_.clipToParent = false;
        primitive_.zIndex = 300;
        primitive_.hasExplicitZIndex = true;
        primitive_.width = 260.0f;
        primitive_.height = 44.0f;
        primitive_.rounding = 10.0f;
        message_ = "Saved";
        showRequested_ = false;
        durationSeconds_ = 2.0f;
        autoHide_ = true;
        fixedToWindowBottomCenter_ = true;
        hasExplicitPosition_ = false;
        fontSize_ = 16.0f;
        onClose_ = {};
    }

private:
    static bool contains(const RectFrame& frame, float x, float y) {
        return x >= frame.x && x <= frame.x + frame.width &&
               y >= frame.y && y <= frame.y + frame.height;
    }

    RectFrame toastFrame() const {
        const RectFrame anchor = PrimitiveFrame(primitive_);
        float width = anchor.width > 0.0f ? anchor.width : primitive_.width;
        float height = anchor.height > 0.0f ? anchor.height : primitive_.height;
        width = std::max(120.0f, width);
        height = std::max(32.0f, height);

        float x = anchor.x;
        float y = anchor.y;
        if (fixedToWindowBottomCenter_ && !hasExplicitPosition_) {
            x = (State.screenW - width) * 0.5f;
            y = State.screenH - height - 24.0f;
        } else if (anchor.width <= 0.0f || anchor.height <= 0.0f) {
            x = (State.screenW - width) * 0.5f;
            y = State.screenH - height - 24.0f;
        }

        x = std::clamp(x, 6.0f, std::max(6.0f, State.screenW - width - 6.0f));
        y = std::clamp(y, 6.0f, std::max(6.0f, State.screenH - height - 6.0f));
        return RectFrame{x, y, width, height};
    }

    std::string message_ = "Saved";
    bool showRequested_ = false;
    float durationSeconds_ = 2.0f;
    bool autoHide_ = true;
    bool fixedToWindowBottomCenter_ = true;
    bool hasExplicitPosition_ = false;
    float fontSize_ = 16.0f;
    std::function<void()> onClose_;

    bool visible_ = false;
    float visibilityAnim_ = 0.0f;
    float remainingSeconds_ = 0.0f;
    bool lastShowRequested_ = false;
    bool dismissedByTimer_ = false;
};

} // namespace EUINEO
