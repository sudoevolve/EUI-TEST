#pragma once

#include "../EUINEO.h"
#include "../ui/UIBuilder.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace EUINEO {

class SliderNode : public UINode {
public:
    class Builder : public UIBuilderBase<SliderNode, Builder> {
    public:
        Builder(UIContext& context, SliderNode& node) : UIBuilderBase<SliderNode, Builder>(context, node) {}

        Builder& value(float nextValue) {
            this->node_.trackComposeValue("value", nextValue);
            this->node_.value_ = nextValue;
            return *this;
        }

        Builder& onChange(std::function<void(float)> handler) {
            this->node_.onChange_ = std::move(handler);
            return *this;
        }
    };

    explicit SliderNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "SliderNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    bool wantsContinuousUpdate() const override {
        return isDragging_ ||
               (hoverAnim_ > 0.001f && hoverAnim_ < 0.999f) ||
               (activeAnim_ > 0.001f && activeAnim_ < 0.999f);
    }

    RectFrame paintBounds() const override {
        return expandPrimitivePaintBounds(8.0f, 0.0f, 8.0f, 0.0f);
    }

    void update() override {
        const RectFrame frame = PrimitiveFrame(primitive_);
        const float clampedValue = std::clamp(value_, 0.0f, 1.0f);
        const float handleDiameter = 16.0f;
        const float handleRadius = handleDiameter * 0.5f;
        const float handleCenterX = frame.x + clampedValue * frame.width;
        const float handleCenterY = frame.y + frame.height * 0.5f;
        const bool hoveredTrack = primitive_.enabled && PrimitiveContains(primitive_, State.mouseX, State.mouseY);
        const bool hoveredHandle = primitive_.enabled &&
            State.mouseX >= handleCenterX - handleRadius - 4.0f && State.mouseX <= handleCenterX + handleRadius + 4.0f &&
            State.mouseY >= handleCenterY - handleRadius - 4.0f && State.mouseY <= handleCenterY + handleRadius + 4.0f;

        const float targetHover = (hoveredTrack || hoveredHandle || isDragging_) ? 1.0f : 0.0f;
        if (std::abs(hoverAnim_ - targetHover) > 0.001f) {
            hoverAnim_ = Lerp(hoverAnim_, targetHover, State.deltaTime * ((hoveredTrack || isDragging_) ? 15.0f : 10.0f));
            if (std::abs(hoverAnim_ - targetHover) < 0.001f) {
                hoverAnim_ = targetHover;
            }
            requestRepaint(12.0f, 0.16f);
        }

        const float targetActive = isDragging_ ? 1.0f : 0.0f;
        if (std::abs(activeAnim_ - targetActive) > 0.001f) {
            activeAnim_ = Lerp(activeAnim_, targetActive, State.deltaTime * 18.0f);
            if (std::abs(activeAnim_ - targetActive) < 0.001f) {
                activeAnim_ = targetActive;
            }
            requestRepaint(12.0f, 0.18f);
        }

        if (primitive_.enabled && State.mouseClicked && hoveredTrack) {
            isDragging_ = true;
            requestRepaint(12.0f, 0.18f);
        }

        if (!State.mouseDown) {
            if (isDragging_) {
                isDragging_ = false;
                requestRepaint(12.0f, 0.18f);
            }
        }

        if (primitive_.enabled && isDragging_ && frame.width > 0.0f) {
            const float relativeX = State.mouseX - frame.x;
            const float nextValue = std::clamp(relativeX / frame.width, 0.0f, 1.0f);
            if (std::abs(value_ - nextValue) > 0.0001f) {
                value_ = nextValue;
                if (onChange_) {
                    onChange_(value_);
                }
                requestRepaint(12.0f, 0.18f);
            }
        }
    }

    void draw() override {
        PrimitiveClipScope clip(primitive_);
        const RectFrame frame = PrimitiveFrame(primitive_);
        const float clampedValue = std::clamp(value_, 0.0f, 1.0f);
        const float emphasis = std::clamp(std::max(hoverAnim_, activeAnim_), 0.0f, 1.0f);

        const float trackHeight = 4.0f;
        const float trackY = frame.y + (frame.height - trackHeight) * 0.5f;
        const Color trackColor = ApplyOpacity(
            Lerp(
                CurrentTheme->surfaceHover,
                CurrentTheme->surfaceActive,
                CurrentTheme == &DarkTheme ? 0.24f : 0.18f
            ),
            primitive_.opacity
        );
        Renderer::DrawRect(
            frame.x,
            trackY,
            frame.width,
            trackHeight,
            trackColor,
            trackHeight * 0.5f
        );

        if (clampedValue > 0.01f) {
            const Color fillColor = ApplyOpacity(
                Lerp(
                    Color(CurrentTheme->primary.r, CurrentTheme->primary.g, CurrentTheme->primary.b, 0.86f),
                    CurrentTheme->primary,
                    emphasis
                ),
                primitive_.opacity
            );
            Renderer::DrawRect(
                frame.x,
                trackY,
                frame.width * clampedValue,
                trackHeight,
                fillColor,
                trackHeight * 0.5f
            );
        }

        const float handleRadius = Lerp(8.0f, 9.5f, emphasis);
        const float handleX = frame.x + clampedValue * frame.width - handleRadius;
        const float handleY = frame.y + (frame.height - handleRadius * 2.0f) * 0.5f;
        const Color handleColor = ApplyOpacity(
            Lerp(CurrentTheme->text, CurrentTheme->primary, emphasis),
            primitive_.opacity
        );
        Renderer::DrawRect(handleX, handleY, handleRadius * 2.0f, handleRadius * 2.0f, handleColor, handleRadius);
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.width = 300.0f;
        primitive_.height = 20.0f;
        value_ = 0.0f;
        onChange_ = {};
    }

private:
    void requestRepaint(float expand = 12.0f, float duration = 0.0f) {
        (void)expand;
        (void)duration;
        requestVisualRepaint();
    }

    float value_ = 0.0f;
    std::function<void(float)> onChange_;
    bool isDragging_ = false;
    float hoverAnim_ = 0.0f;
    float activeAnim_ = 0.0f;
};

} // namespace EUINEO
