#pragma once

#include "../EUINEO.h"
#include "../ui/UIBuilder.h"
#include "../ui/ThemeTokens.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace EUINEO {

class ComboBoxNode : public UINode {
public:
    class Builder : public UIBuilderBase<ComboBoxNode, Builder> {
    public:
        Builder(UIContext& context, ComboBoxNode& node) : UIBuilderBase<ComboBoxNode, Builder>(context, node) {}

        Builder& items(std::vector<std::string> value) {
            this->node_.trackComposeValue("items", value);
            this->node_.items_ = std::move(value);
            return *this;
        }

        Builder& placeholder(std::string value) {
            this->node_.trackComposeValue("placeholder", value);
            this->node_.placeholder_ = std::move(value);
            return *this;
        }

        Builder& selected(int index) {
            this->node_.trackComposeValue("selected", index);
            this->node_.selectedIndex_ = index;
            return *this;
        }

        Builder& fontSize(float value) {
            this->node_.trackComposeValue("fontSize", value);
            this->node_.fontSize_ = value;
            return *this;
        }

        Builder& startOpen(bool value) {
            this->node_.trackComposeValue("startOpen", value);
            if (!this->node_.openStateInitialized_) {
                this->node_.isOpen_ = value;
                this->node_.openAnim_ = value ? 1.0f : 0.0f;
                this->node_.popupPresentation_ = value;
                this->node_.openStateInitialized_ = true;
            }
            return *this;
        }

        Builder& onChange(std::function<void(int)> handler) {
            this->node_.onChange_ = std::move(handler);
            return *this;
        }
    };

    explicit ComboBoxNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "ComboBoxNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    bool wantsContinuousUpdate() const override {
        if (hoverAnim_ > 0.001f && hoverAnim_ < 0.999f) {
            return true;
        }
        if (openAnim_ > 0.001f && openAnim_ < 0.999f) {
            return true;
        }
        for (float hover : itemHoverAnims_) {
            if (hover > 0.001f && hover < 0.999f) {
                return true;
            }
        }
        return false;
    }

    RectFrame paintBounds() const override {
        const RectFrame frame = PrimitiveFrame(primitive_);
        RectFrame bounds = popupPresentation_ ? frame : clipPaintBounds(frame);
        const float visibleOpen = std::clamp(openAnim_, 0.0f, 1.0f);
        const float visibleListHeight = listVisibleHeight(frame.height, items_.size(), visibleOpen);
        if (visibleListHeight <= 0.0f) {
            return bounds;
        }

        const UIFieldVisualTokens visuals = CurrentFieldVisuals();
        const RectFrame popupFrame = PopupListFrame(frame, visibleListHeight, listOverlap(visibleListHeight));
        const RectFrame popupBounds = measurePaintBounds(
            popupFrame,
            MakePopupChromeStyle(primitive_, visuals.popupRounding),
            !popupPresentation_
        );
        return unionPaintBounds(bounds, popupBounds);
    }

    void update() override {
        ensureRuntimeState();
        const RectFrame frame = PrimitiveFrame(primitive_);
        const bool hoveredMain = hovered();

        if (animateTowards(hoverAnim_, hoveredMain ? 1.0f : 0.0f, State.deltaTime * 15.0f)) {
            requestRepaint(openAnim_, openAnim_);
        }

        const float targetOpen = isOpen_ ? 1.0f : 0.0f;
        const float fromOpen = openAnim_;
        if (animateTowards(openAnim_, targetOpen, State.deltaTime * 20.0f)) {
            requestRepaint(fromOpen, openAnim_);
        }

        const bool wantsPopupPresentation = isOpen_ || openAnim_ > 0.001f;
        if (popupPresentation_ != wantsPopupPresentation) {
            popupPresentation_ = wantsPopupPresentation;
            if (popupPresentation_) {
                applyPopupPresentationDefaults(180);
            } else {
                primitive_.renderLayer = RenderLayer::Content;
                primitive_.clipToParent = true;
            }
            requestRepaint(openAnim_, openAnim_);
        }

        if (isOpen_ || openAnim_ > 0.0f) {
            const float listY = frame.y + frame.height;
            for (std::size_t index = 0; index < items_.size(); ++index) {
                const float itemY = listY + static_cast<float>(index) * frame.height;
                const bool itemHovered = primitive_.enabled &&
                    State.mouseX >= frame.x && State.mouseX <= frame.x + frame.width &&
                    State.mouseY >= itemY && State.mouseY <= itemY + frame.height;

                const float targetItemHover = (itemHovered && isOpen_) ? 1.0f : 0.0f;
                if (animateTowards(itemHoverAnims_[index], targetItemHover, State.deltaTime * 15.0f)) {
                    requestRepaint(openAnim_, openAnim_);
                }
            }
        }

        if (State.mouseClicked && primitive_.enabled) {
            if (isOpen_) {
                const float visibleOpen = std::clamp(openAnim_, 0.0f, 1.0f);
                const float visibleListHeight = listVisibleHeight(frame.height, items_.size(), visibleOpen);
                const float overlap = listOverlap(visibleListHeight);
                const RectFrame popupFrame = PopupListFrame(frame, visibleListHeight, overlap);
                const float listY = popupFrame.y;
                const float listHeight = popupFrame.height;
                const bool hoveredList =
                    State.mouseX >= frame.x && State.mouseX <= frame.x + frame.width &&
                    State.mouseY >= listY && State.mouseY <= listY + listHeight;

                if (hoveredList && frame.height > 0.0f) {
                    const int index = static_cast<int>((State.mouseY - listY) / frame.height);
                    if (index >= 0 && index < static_cast<int>(items_.size())) {
                        selectedIndex_ = index;
                        if (onChange_) {
                            onChange_(selectedIndex_);
                        }
                    }
                }

                const float fromOpen = openAnim_;
                isOpen_ = false;
                requestRepaint(fromOpen, 0.0f);
            } else if (hoveredMain) {
                const float fromOpen = openAnim_;
                isOpen_ = true;
                requestRepaint(fromOpen, 1.0f);
            }
        }
    }

    void draw() override {
        const RectFrame frame = PrimitiveFrame(primitive_);
        const UIFieldVisualTokens visuals = CurrentFieldVisuals();
        const float textScale = fontSize_ / 24.0f;
        const float textX = frame.x + visuals.horizontalInset;
        const float textY = frame.y + frame.height * 0.5f + (fontSize_ / 4.0f);
        const float visibleOpen = std::clamp(openAnim_, 0.0f, 1.0f);
        const float visibleListHeight = listVisibleHeight(frame.height, items_.size(), visibleOpen);

        if (visibleListHeight > 1.0f) {
            const float overlap = listOverlap(visibleListHeight);
            const RectFrame popupFrame = PopupListFrame(frame, visibleListHeight, overlap);
            const float listY = popupFrame.y;
            const float listHeight = popupFrame.height;
            DrawPopupChrome(primitive_, popupFrame, visuals.popupRounding);

            for (std::size_t index = 0; index < items_.size(); ++index) {
                const float itemY = frame.y + frame.height + static_cast<float>(index) * frame.height;
                const float currentListBottom = listY + listHeight;
                const float visibleHeight = std::clamp(currentListBottom - itemY, 0.0f, frame.height);

                if (itemY > currentListBottom || visibleHeight <= 0.0f) {
                    break;
                }

                const float itemAlpha = visibleHeight / frame.height;
                Color itemBg = Lerp(CurrentTheme->surface, CurrentTheme->surfaceHover, itemHoverAnims_[index]);
                if (itemHoverAnims_[index] > 0.01f) {
                    itemBg.a *= visibleOpen;
                    Renderer::DrawRect(
                        frame.x + 1.0f,
                        itemY,
                        frame.width - 2.0f,
                        visibleHeight,
                        ApplyOpacity(itemBg, primitive_.opacity),
                        visibleHeight < frame.height ? 0.0f : 4.0f
                    );
                }

                Color itemColor = index == static_cast<std::size_t>(selectedIndex_) ? CurrentTheme->primary : CurrentTheme->text;
                itemColor.a *= visibleOpen * itemAlpha;
                if (visibleHeight >= fontSize_ * 0.90f) {
                    Renderer::DrawTextStr(items_[index], textX, itemY + frame.height * 0.5f + (fontSize_ / 4.0f),
                                          ApplyOpacity(itemColor, primitive_.opacity), textScale);
                }
            }
        }

        {
            PrimitiveClipScope clip(primitive_);
            DrawFieldChrome(primitive_, hoverAnim_, openAnim_, primitive_.rounding);

            const std::string displayText =
                (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) ? items_[selectedIndex_] : placeholder_;
            Color textColor = selectedIndex_ >= 0
                ? CurrentTheme->text
                : Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.5f);
            Renderer::DrawTextStr(displayText, textX, textY, ApplyOpacity(textColor, primitive_.opacity), textScale);
            Renderer::DrawTextStr(isOpen_ ? "\xEF\x84\x86" : "\xEF\x84\x87",
                                  frame.x + frame.width - 25.0f, textY,
                                  ApplyOpacity(CurrentTheme->text, primitive_.opacity), textScale);
        }
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        if (popupPresentation_) {
            applyPopupPresentationDefaults(180);
        }
        primitive_.width = 220.0f;
        primitive_.height = 36.0f;
        placeholder_.clear();
        items_.clear();
        selectedIndex_ = -1;
        fontSize_ = 20.0f;
        onChange_ = {};
    }

private:
    static float listVisibleHeight(float itemHeight, std::size_t itemCount, float openFactor) {
        return static_cast<float>(itemCount) * itemHeight * std::clamp(openFactor, 0.0f, 1.0f);
    }

    static float listOverlap(float visibleListHeight) {
        return visibleListHeight > 6.0f ? 1.0f : 0.0f;
    }

    void ensureRuntimeState() {
        if (itemHoverAnims_.size() != items_.size()) {
            itemHoverAnims_.assign(items_.size(), 0.0f);
        }
    }

    void requestRepaint(float fromOpenFactor, float toOpenFactor, float duration = 0.0f) {
        const float delta = std::abs(toOpenFactor - fromOpenFactor);
        const float autoDuration = std::max(0.10f, delta * 0.20f);
        requestVisualRepaint(duration > 0.0f ? duration : autoDuration);
    }

    std::vector<std::string> items_;
    std::string placeholder_;
    int selectedIndex_ = -1;
    float fontSize_ = 20.0f;
    std::function<void(int)> onChange_;
    bool isOpen_ = false;
    bool openStateInitialized_ = false;
    bool popupPresentation_ = false;
    float hoverAnim_ = 0.0f;
    float openAnim_ = 0.0f;
    std::vector<float> itemHoverAnims_;
};

} // namespace EUINEO
