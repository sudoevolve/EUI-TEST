#pragma once

#include "../EUINEO.h"
#include "../ui/UIBuilder.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace EUINEO {

class TabsNode : public UINode {
public:
    class Builder : public UIBuilderBase<TabsNode, Builder> {
    public:
        Builder(UIContext& context, TabsNode& node) : UIBuilderBase<TabsNode, Builder>(context, node) {}

        Builder& items(std::vector<std::string> value) {
            this->node_.trackComposeValue("items", value);
            this->node_.items_ = std::move(value);
            return *this;
        }

        Builder& selected(int index) {
            this->node_.trackComposeValue("selected", index);
            this->node_.selectedIndex_ = index;
            return *this;
        }

        Builder& fontSize(float value) {
            this->node_.trackComposeValue("fontSize", value);
            this->node_.fontSize_ = std::max(12.0f, value);
            return *this;
        }

        Builder& onChange(std::function<void(int)> handler) {
            this->node_.onChange_ = std::move(handler);
            return *this;
        }
    };

    explicit TabsNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "TabsNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    bool wantsContinuousUpdate() const override {
        if (!indicatorReady_ || std::abs(indicatorAnim_ - indicatorTarget_) > 0.001f) {
            return true;
        }
        for (float hover : itemHover_) {
            if (hover > 0.001f && hover < 0.999f) {
                return true;
            }
        }
        return false;
    }

    void update() override {
        ensureRuntimeState();
        if (items_.empty()) {
            return;
        }
        const bool inputAllowed = !(State.inputBlockedByPopup && primitive_.renderLayer != RenderLayer::Popup);

        const int clampedSelected = std::clamp(selectedIndex_, 0, static_cast<int>(items_.size()) - 1);
        if (!indicatorReady_ || lastItemCount_ != items_.size()) {
            indicatorAnim_ = static_cast<float>(clampedSelected);
            indicatorTarget_ = indicatorAnim_;
            indicatorReady_ = true;
            lastItemCount_ = items_.size();
        }

        const RectFrame frame = PrimitiveFrame(primitive_);
        const float tabWidth = frame.width / static_cast<float>(items_.size());
        for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
            RectFrame tabFrame{
                frame.x + tabWidth * static_cast<float>(index),
                frame.y,
                tabWidth,
                frame.height
            };
            const bool hovered = primitive_.enabled && inputAllowed && contains(tabFrame, State.mouseX, State.mouseY);
            const float targetHover = hovered ? 1.0f : 0.0f;
            if (animateTowards(itemHover_[index], targetHover, State.deltaTime * 15.0f)) {
                requestVisualRepaint(0.14f);
            }
            if (hovered && State.mouseClicked && index != clampedSelected) {
                selectedIndex_ = index;
                if (onChange_) {
                    onChange_(selectedIndex_);
                }
                State.mouseClicked = false;
                requestVisualRepaint(0.16f);
                break;
            }
        }

        indicatorTarget_ = static_cast<float>(std::clamp(selectedIndex_, 0, static_cast<int>(items_.size()) - 1));
        if (animateTowards(indicatorAnim_, indicatorTarget_, State.deltaTime * 17.0f, 0.004f, 0.0004f)) {
            requestVisualRepaint(0.14f);
        }
    }

    void draw() override {
        ensureRuntimeState();
        PrimitiveClipScope clip(primitive_);
        const RectFrame frame = PrimitiveFrame(primitive_);
        if (items_.empty()) {
            Renderer::DrawRect(frame.x, frame.y + frame.height - 2.0f, frame.width, 2.0f,
                               ApplyOpacity(CurrentTheme->border, primitive_.opacity), 0.0f);
            return;
        }

        const float tabWidth = frame.width / static_cast<float>(items_.size());
        const float textScale = fontSize_ / 24.0f;
        const int clampedSelected = std::clamp(selectedIndex_, 0, static_cast<int>(items_.size()) - 1);

        Renderer::DrawRect(frame.x, frame.y + frame.height - 1.0f, frame.width, 1.0f,
                           ApplyOpacity(CurrentTheme->border, primitive_.opacity), 0.0f);

        for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
            const RectFrame tabFrame{
                frame.x + tabWidth * static_cast<float>(index),
                frame.y,
                tabWidth,
                frame.height
            };
            if (itemHover_[index] > 0.01f && index != clampedSelected) {
                const Color hoverColor = Color(CurrentTheme->surfaceHover.r, CurrentTheme->surfaceHover.g, CurrentTheme->surfaceHover.b, 0.64f * itemHover_[index]);
                Renderer::DrawRect(tabFrame.x + 2.0f, tabFrame.y + 3.0f, std::max(0.0f, tabFrame.width - 4.0f),
                                   std::max(0.0f, tabFrame.height - 6.0f), ApplyOpacity(hoverColor, primitive_.opacity), 8.0f);
            }

            const float textWidth = Renderer::MeasureTextWidth(items_[index], textScale);
            const float textX = tabFrame.x + (tabFrame.width - textWidth) * 0.5f;
            const float textY = tabFrame.y + tabFrame.height * 0.5f + fontSize_ * 0.22f - 2.0f;
            const bool selected = index == clampedSelected;
            const Color textColor = selected
                ? CurrentTheme->primary
                : Lerp(CurrentTheme->text, CurrentTheme->primary, itemHover_[index] * 0.24f);
            Renderer::DrawTextStr(items_[index], textX, textY, ApplyOpacity(textColor, primitive_.opacity), textScale);
        }

        const float indicatorX = frame.x + indicatorAnim_ * tabWidth;
        Renderer::DrawRect(
            indicatorX + 10.0f,
            frame.y + frame.height - 3.0f,
            std::max(0.0f, tabWidth - 20.0f),
            3.0f,
            ApplyOpacity(CurrentTheme->primary, primitive_.opacity),
            1.5f
        );
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.width = 360.0f;
        primitive_.height = 42.0f;
        items_.clear();
        selectedIndex_ = 0;
        fontSize_ = 18.0f;
        onChange_ = {};
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

    std::vector<std::string> items_;
    int selectedIndex_ = 0;
    float fontSize_ = 18.0f;
    std::function<void(int)> onChange_;
    std::vector<float> itemHover_;
    float indicatorAnim_ = 0.0f;
    float indicatorTarget_ = 0.0f;
    bool indicatorReady_ = false;
    std::size_t lastItemCount_ = 0;
};

} // namespace EUINEO
