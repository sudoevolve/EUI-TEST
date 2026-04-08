#pragma once

#include "../EUINEO.h"
#include "../ui/ThemeTokens.h"
#include "../ui/UIBuilder.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace EUINEO {

class ContextMenuNode : public UINode {
public:
    class Builder : public UIBuilderBase<ContextMenuNode, Builder> {
    public:
        Builder(UIContext& context, ContextMenuNode& node) : UIBuilderBase<ContextMenuNode, Builder>(context, node) {}

        Builder& items(std::vector<std::string> value) {
            this->node_.trackComposeValue("items", value);
            this->node_.items_ = std::move(value);
            return *this;
        }

        Builder& label(std::string value) {
            this->node_.trackComposeValue("label", value);
            this->node_.label_ = std::move(value);
            return *this;
        }

        Builder& fontSize(float value) {
            this->node_.trackComposeValue("fontSize", value);
            this->node_.fontSize_ = std::max(12.0f, value);
            return *this;
        }

        Builder& itemHeight(float value) {
            this->node_.trackComposeValue("itemHeight", value);
            this->node_.itemHeight_ = std::max(24.0f, value);
            return *this;
        }

        Builder& onSelect(std::function<void(int)> handler) {
            this->node_.onSelect_ = std::move(handler);
            return *this;
        }
    };

    explicit ContextMenuNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "ContextMenuNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    bool usesCachedSurface() const override {
        return false;
    }

    bool blocksUnderlyingInput() const override {
        return open_ || visibilityAnim_ > 0.001f;
    }

    bool wantsContinuousUpdate() const override {
        if (visibilityAnim_ > 0.001f && visibilityAnim_ < 0.999f) {
            return true;
        }
        if (hintHover_ > 0.001f && hintHover_ < 0.999f) {
            return true;
        }
        for (float hover : itemHover_) {
            if (hover > 0.001f && hover < 0.999f) {
                return true;
            }
        }
        return false;
    }

    RectFrame paintBounds() const override {
        const RectFrame frame = PrimitiveFrame(primitive_);
        if (!(open_ || visibilityAnim_ > 0.001f)) {
            return clipPaintBounds(frame);
        }
        return unionPaintBounds(clipPaintBounds(frame), popupFrame(frame));
    }

    void update() override {
        ensureRuntimeState();
        const RectFrame frame = PrimitiveFrame(primitive_);
        const bool inputAllowed = !(State.inputBlockedByPopup && primitive_.renderLayer != RenderLayer::Popup);
        const bool hoveredHint = primitive_.enabled && inputAllowed && contains(frame, State.mouseX, State.mouseY);

        if (animateTowards(hintHover_, hoveredHint ? 1.0f : 0.0f, State.deltaTime * 14.0f)) {
            requestVisualRepaint(0.12f);
        }

        if (primitive_.enabled && hoveredHint && State.mouseRightClicked) {
            open_ = !items_.empty();
            if (open_) {
                popupX_ = State.mouseX;
                popupY_ = State.mouseY;
                State.inputBlockedByPopup = true;
            }
            State.mouseRightClicked = false;
            requestVisualRepaint(0.14f);
        }

        if (!open_ && visibilityAnim_ <= 0.001f) {
            updatePopupPresentation(false);
            return;
        }

        const float previousVisibility = visibilityAnim_;
        if (animateTowards(visibilityAnim_, open_ ? 1.0f : 0.0f, State.deltaTime * 20.0f)) {
            requestVisualRepaint(0.14f);
        }
        updatePopupPresentation(open_ || visibilityAnim_ > 0.001f);

        if (visibilityAnim_ <= 0.001f) {
            return;
        }

        const RectFrame popup = popupFrame(frame);
        if (open_ && State.mouseClicked) {
            if (!contains(popup, State.mouseX, State.mouseY)) {
                open_ = false;
                State.mouseClicked = false;
                requestVisualRepaint(0.14f);
            } else {
                const int index = static_cast<int>((State.mouseY - popup.y) / itemHeight_);
                if (index >= 0 && index < static_cast<int>(items_.size())) {
                    if (onSelect_) {
                        onSelect_(index);
                    }
                    open_ = false;
                    State.mouseClicked = false;
                    requestVisualRepaint(0.14f);
                }
            }
        }

        for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
            const RectFrame item = itemFrame(popup, index);
            const bool hoveredItem = open_ && inputAllowed && contains(item, State.mouseX, State.mouseY);
            if (animateTowards(itemHover_[index], hoveredItem ? 1.0f : 0.0f, State.deltaTime * 16.0f)) {
                requestVisualRepaint(0.12f);
            }
        }

        if (std::abs(previousVisibility - visibilityAnim_) > 0.0001f) {
            requestVisualRepaint(0.12f);
        }
    }

    void draw() override {
        const RectFrame frame = PrimitiveFrame(primitive_);
        {
            PrimitiveClipScope clip(primitive_);
            DrawFieldChrome(primitive_, hintHover_, 0.0f, primitive_.rounding > 0.0f ? primitive_.rounding : 8.0f);
            const float textScale = fontSize_ / 24.0f;
            const Color hintColor = Lerp(
                Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.70f),
                CurrentTheme->text,
                hintHover_ * 0.8f
            );
            Renderer::DrawTextStr(label_, frame.x + 12.0f, frame.y + frame.height * 0.5f + fontSize_ * 0.24f,
                                  ApplyOpacity(hintColor, primitive_.opacity), textScale);
        }

        if (visibilityAnim_ <= 0.001f || items_.empty()) {
            return;
        }

        const RectFrame popup = popupFrame(frame);
        UIPrimitive popupPrimitive = primitive_;
        popupPrimitive.opacity = primitive_.opacity * std::clamp(visibilityAnim_, 0.0f, 1.0f);
        DrawPopupChrome(popupPrimitive, popup, 10.0f);

        const float textScale = fontSize_ / 24.0f;
        for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
            const RectFrame item = itemFrame(popup, index);
            if (itemHover_[index] > 0.01f) {
                const Color bg = Color(CurrentTheme->surfaceHover.r, CurrentTheme->surfaceHover.g, CurrentTheme->surfaceHover.b, 0.85f * itemHover_[index]);
                Renderer::DrawRect(item.x + 2.0f, item.y + 1.0f, std::max(0.0f, item.width - 4.0f),
                                   std::max(0.0f, item.height - 2.0f), ApplyOpacity(bg, popupPrimitive.opacity), 7.0f);
            }
            Renderer::DrawTextStr(
                items_[index],
                item.x + 12.0f,
                item.y + item.height * 0.5f + fontSize_ * 0.24f,
                ApplyOpacity(CurrentTheme->text, popupPrimitive.opacity),
                textScale
            );
        }
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        if (popupPresentation_) {
            applyPopupPresentationDefaults(240);
        }
        primitive_.width = 220.0f;
        primitive_.height = 34.0f;
        primitive_.rounding = 8.0f;
        items_.clear();
        label_ = "Right click here";
        fontSize_ = 16.0f;
        itemHeight_ = 30.0f;
        onSelect_ = {};
    }

private:
    static bool contains(const RectFrame& frame, float x, float y) {
        return x >= frame.x && x <= frame.x + frame.width &&
               y >= frame.y && y <= frame.y + frame.height;
    }

    void ensureRuntimeState() {
        if (itemHover_.size() != items_.size()) {
            itemHover_.assign(items_.size(), 0.0f);
        }
    }

    void updatePopupPresentation(bool showPopup) {
        if (popupPresentation_ == showPopup) {
            return;
        }
        popupPresentation_ = showPopup;
        if (popupPresentation_) {
            applyPopupPresentationDefaults(240);
        } else {
            primitive_.renderLayer = RenderLayer::Content;
            primitive_.clipToParent = true;
        }
    }

    RectFrame popupFrame(const RectFrame& frame) const {
        const float menuWidth = std::max(frame.width, 150.0f);
        const float menuHeight = itemHeight_ * static_cast<float>(items_.size());
        const float maxX = std::max(6.0f, State.screenW - menuWidth - 6.0f);
        const float maxY = std::max(6.0f, State.screenH - menuHeight - 6.0f);
        const float x = std::clamp(popupX_, 6.0f, maxX);
        const float y = std::clamp(popupY_, 6.0f, maxY);
        return RectFrame{x, y, menuWidth, menuHeight};
    }

    RectFrame itemFrame(const RectFrame& popup, int index) const {
        return RectFrame{
            popup.x,
            popup.y + itemHeight_ * static_cast<float>(index),
            popup.width,
            itemHeight_
        };
    }

    std::vector<std::string> items_;
    std::string label_ = "Right click here";
    float fontSize_ = 16.0f;
    float itemHeight_ = 30.0f;
    std::function<void(int)> onSelect_;

    bool open_ = false;
    bool popupPresentation_ = false;
    float popupX_ = 0.0f;
    float popupY_ = 0.0f;
    float visibilityAnim_ = 0.0f;
    float hintHover_ = 0.0f;
    std::vector<float> itemHover_;
};

} // namespace EUINEO
