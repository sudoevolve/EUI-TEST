#pragma once

#include "../EUINEO.h"
#include "PrimitiveMotion.h"
#include "../ui/UIBuilder.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace EUINEO {

class PolygonNode : public UINode {
public:
    class Builder : public UIBuilderBase<PolygonNode, Builder> {
    public:
        Builder(UIContext& context, PolygonNode& node) : UIBuilderBase<PolygonNode, Builder>(context, node) {}

        Builder& points(std::initializer_list<Point2> values) {
            return points(std::vector<Point2>(values));
        }

        Builder& points(std::vector<Point2> values) {
            this->node_.trackComposeValue("polygonPointCount", static_cast<int>(values.size()));
            for (const Point2& point : values) {
                this->node_.trackComposeValue("polygonPointX", point.x);
                this->node_.trackComposeValue("polygonPointY", point.y);
            }
            this->node_.points_ = std::move(values);
            return *this;
        }

        Builder& animateScale(float from, float to, float duration,
                              Easing easing = Easing::EaseInOut,
                              bool loop = true, bool pingPong = true) {
            this->node_.trackComposeValue("scaleMotionFrom", from);
            this->node_.trackComposeValue("scaleMotionTo", to);
            this->node_.trackComposeValue("scaleMotionDuration", duration);
            this->node_.trackComposeValue("scaleMotionEasing", easing);
            this->node_.trackComposeValue("scaleMotionLoop", loop);
            this->node_.trackComposeValue("scaleMotionPingPong", pingPong);
            this->node_.scaleMotion_ = ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong};
            return *this;
        }

        Builder& animateRotation(float from, float to, float duration,
                                 Easing easing = Easing::EaseInOut,
                                 bool loop = true, bool pingPong = true) {
            this->node_.trackComposeValue("rotationMotionFrom", from);
            this->node_.trackComposeValue("rotationMotionTo", to);
            this->node_.trackComposeValue("rotationMotionDuration", duration);
            this->node_.trackComposeValue("rotationMotionEasing", easing);
            this->node_.trackComposeValue("rotationMotionLoop", loop);
            this->node_.trackComposeValue("rotationMotionPingPong", pingPong);
            this->node_.rotationMotion_ = ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong};
            return *this;
        }

        Builder& animateOpacity(float from, float to, float duration,
                                Easing easing = Easing::EaseInOut,
                                bool loop = true, bool pingPong = true) {
            this->node_.trackComposeValue("opacityMotionFrom", from);
            this->node_.trackComposeValue("opacityMotionTo", to);
            this->node_.trackComposeValue("opacityMotionDuration", duration);
            this->node_.trackComposeValue("opacityMotionEasing", easing);
            this->node_.trackComposeValue("opacityMotionLoop", loop);
            this->node_.trackComposeValue("opacityMotionPingPong", pingPong);
            this->node_.opacityMotion_ = ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong};
            return *this;
        }

        Builder& animateTranslateX(float from, float to, float duration,
                                   Easing easing = Easing::EaseInOut,
                                   bool loop = true, bool pingPong = true) {
            this->node_.trackComposeValue("translateXMotionFrom", from);
            this->node_.trackComposeValue("translateXMotionTo", to);
            this->node_.trackComposeValue("translateXMotionDuration", duration);
            this->node_.trackComposeValue("translateXMotionEasing", easing);
            this->node_.trackComposeValue("translateXMotionLoop", loop);
            this->node_.trackComposeValue("translateXMotionPingPong", pingPong);
            this->node_.translateXMotion_ = ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong};
            return *this;
        }

        Builder& animateTranslateY(float from, float to, float duration,
                                   Easing easing = Easing::EaseInOut,
                                   bool loop = true, bool pingPong = true) {
            this->node_.trackComposeValue("translateYMotionFrom", from);
            this->node_.trackComposeValue("translateYMotionTo", to);
            this->node_.trackComposeValue("translateYMotionDuration", duration);
            this->node_.trackComposeValue("translateYMotionEasing", easing);
            this->node_.trackComposeValue("translateYMotionLoop", loop);
            this->node_.trackComposeValue("translateYMotionPingPong", pingPong);
            this->node_.translateYMotion_ = ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong};
            return *this;
        }

        Builder& animateBackground(const Color& from, const Color& to, float duration,
                                   Easing easing = Easing::EaseInOut,
                                   bool loop = true, bool pingPong = true) {
            this->node_.trackComposeValue("backgroundMotionFrom", from);
            this->node_.trackComposeValue("backgroundMotionTo", to);
            this->node_.trackComposeValue("backgroundMotionDuration", duration);
            this->node_.trackComposeValue("backgroundMotionEasing", easing);
            this->node_.trackComposeValue("backgroundMotionLoop", loop);
            this->node_.trackComposeValue("backgroundMotionPingPong", pingPong);
            this->node_.backgroundMotion_ = ColorMotionSpec{true, from, to, duration, easing, loop, pingPong};
            return *this;
        }

        Builder& hoverScale(float idle, float hover, float duration,
                            Easing easing = Easing::EaseInOut) {
            this->node_.trackComposeValue("hoverScaleIdle", idle);
            this->node_.trackComposeValue("hoverScaleHover", hover);
            this->node_.trackComposeValue("hoverScaleDuration", duration);
            this->node_.trackComposeValue("hoverScaleEasing", easing);
            this->node_.hoverScaleMotion_ = HoverScalarMotionSpec{true, idle, hover, duration, easing};
            return *this;
        }

        Builder& hoverRotation(float idle, float hover, float duration,
                               Easing easing = Easing::EaseInOut) {
            this->node_.trackComposeValue("hoverRotationIdle", idle);
            this->node_.trackComposeValue("hoverRotationHover", hover);
            this->node_.trackComposeValue("hoverRotationDuration", duration);
            this->node_.trackComposeValue("hoverRotationEasing", easing);
            this->node_.hoverRotationMotion_ = HoverScalarMotionSpec{true, idle, hover, duration, easing};
            return *this;
        }

        Builder& hoverOpacity(float idle, float hover, float duration,
                              Easing easing = Easing::EaseInOut) {
            this->node_.trackComposeValue("hoverOpacityIdle", idle);
            this->node_.trackComposeValue("hoverOpacityHover", hover);
            this->node_.trackComposeValue("hoverOpacityDuration", duration);
            this->node_.trackComposeValue("hoverOpacityEasing", easing);
            this->node_.hoverOpacityMotion_ = HoverScalarMotionSpec{true, idle, hover, duration, easing};
            return *this;
        }

        Builder& hoverTranslateX(float idle, float hover, float duration,
                                 Easing easing = Easing::EaseInOut) {
            this->node_.trackComposeValue("hoverTranslateXIdle", idle);
            this->node_.trackComposeValue("hoverTranslateXHover", hover);
            this->node_.trackComposeValue("hoverTranslateXDuration", duration);
            this->node_.trackComposeValue("hoverTranslateXEasing", easing);
            this->node_.hoverTranslateXMotion_ = HoverScalarMotionSpec{true, idle, hover, duration, easing};
            return *this;
        }

        Builder& hoverTranslateY(float idle, float hover, float duration,
                                 Easing easing = Easing::EaseInOut) {
            this->node_.trackComposeValue("hoverTranslateYIdle", idle);
            this->node_.trackComposeValue("hoverTranslateYHover", hover);
            this->node_.trackComposeValue("hoverTranslateYDuration", duration);
            this->node_.trackComposeValue("hoverTranslateYEasing", easing);
            this->node_.hoverTranslateYMotion_ = HoverScalarMotionSpec{true, idle, hover, duration, easing};
            return *this;
        }

        Builder& hoverBackground(const Color& idle, const Color& hover, float duration,
                                 Easing easing = Easing::EaseInOut) {
            this->node_.trackComposeValue("hoverBackgroundIdle", idle);
            this->node_.trackComposeValue("hoverBackgroundHover", hover);
            this->node_.trackComposeValue("hoverBackgroundDuration", duration);
            this->node_.trackComposeValue("hoverBackgroundEasing", easing);
            this->node_.hoverBackgroundMotion_ = HoverColorMotionSpec{true, idle, hover, duration, easing};
            return *this;
        }
    };

    explicit PolygonNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "PolygonNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    bool wantsContinuousUpdate() const override {
        return motion_.wantsContinuousUpdate(
            scaleMotion_, hoverScaleMotion_,
            rotationMotion_, hoverRotationMotion_,
            opacityMotion_, hoverOpacityMotion_,
            translateXMotion_, hoverTranslateXMotion_,
            translateYMotion_, hoverTranslateYMotion_,
            backgroundMotion_, hoverBackgroundMotion_
        );
    }

    RectFrame paintBounds() const override {
        const UIPrimitive animatedPrimitive = resolvedPrimitive();
        const std::vector<Point2> screenPoints = makeScreenPoints(animatedPrimitive);
        const RectBounds bounds = Renderer::MeasurePolygonBounds(screenPoints, animatedPrimitive.borderWidth);
        return clipPaintBounds(RectFrame{bounds.x, bounds.y, bounds.w, bounds.h});
    }

    void update() override {
        if (motion_.update(
                State.deltaTime,
                motionHovered(),
                scaleMotion_, hoverScaleMotion_,
                rotationMotion_, hoverRotationMotion_,
                opacityMotion_, hoverOpacityMotion_,
                translateXMotion_, hoverTranslateXMotion_,
                translateYMotion_, hoverTranslateYMotion_,
                backgroundMotion_, hoverBackgroundMotion_)) {
            requestVisualRepaint(0.18f);
        }
    }

    void draw() override {
        const UIPrimitive animatedPrimitive = resolvedPrimitive();
        PrimitiveClipScope clip(animatedPrimitive);
        RectGradient gradient = animatedPrimitive.gradient;
        if (gradient.enabled) {
            gradient.topLeft = ApplyOpacity(gradient.topLeft, animatedPrimitive.opacity);
            gradient.topRight = ApplyOpacity(gradient.topRight, animatedPrimitive.opacity);
            gradient.bottomLeft = ApplyOpacity(gradient.bottomLeft, animatedPrimitive.opacity);
            gradient.bottomRight = ApplyOpacity(gradient.bottomRight, animatedPrimitive.opacity);
        }
        Renderer::DrawPolygon(
            makeScreenPoints(animatedPrimitive),
            ApplyOpacity(animatedPrimitive.background, animatedPrimitive.opacity),
            gradient,
            animatedPrimitive.borderWidth,
            ApplyOpacity(animatedPrimitive.borderColor, animatedPrimitive.opacity)
        );
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.width = 96.0f;
        primitive_.height = 96.0f;
        primitive_.background = CurrentTheme->primary;
        points_.clear();
        scaleMotion_ = ScalarMotionSpec{};
        rotationMotion_ = ScalarMotionSpec{};
        opacityMotion_ = ScalarMotionSpec{};
        translateXMotion_ = ScalarMotionSpec{};
        translateYMotion_ = ScalarMotionSpec{};
        backgroundMotion_ = ColorMotionSpec{};
        hoverScaleMotion_ = HoverScalarMotionSpec{};
        hoverRotationMotion_ = HoverScalarMotionSpec{};
        hoverOpacityMotion_ = HoverScalarMotionSpec{};
        hoverTranslateXMotion_ = HoverScalarMotionSpec{};
        hoverTranslateYMotion_ = HoverScalarMotionSpec{};
        hoverBackgroundMotion_ = HoverColorMotionSpec{};
    }

private:
    static bool containsPoint(const std::vector<Point2>& polygon, float x, float y) {
        if (polygon.size() < 3) {
            return false;
        }

        bool inside = false;
        for (std::size_t index = 0, previous = polygon.size() - 1; index < polygon.size(); previous = index++) {
            const Point2& a = polygon[previous];
            const Point2& b = polygon[index];
            const bool crosses = (a.y > y) != (b.y > y);
            if (!crosses) {
                continue;
            }

            const float edgeY = b.y - a.y;
            if (std::abs(edgeY) <= 0.0001f) {
                continue;
            }

            const float t = (y - a.y) / edgeY;
            const float hitX = a.x + (b.x - a.x) * t;
            if (x < hitX) {
                inside = !inside;
            }
        }
        return inside;
    }

    bool motionHovered() const {
        if (!primitive_.enabled) {
            return false;
        }
        const std::vector<Point2> screenPoints = makeScreenPoints(primitive_);
        return containsPoint(screenPoints, State.mouseX, State.mouseY);
    }

    UIPrimitive resolvedPrimitive() const {
        UIPrimitive animatedPrimitive = primitive_;
        motion_.apply(
            animatedPrimitive,
            scaleMotion_, hoverScaleMotion_,
            rotationMotion_, hoverRotationMotion_,
            opacityMotion_, hoverOpacityMotion_,
            translateXMotion_, hoverTranslateXMotion_,
            translateYMotion_, hoverTranslateYMotion_,
            backgroundMotion_, hoverBackgroundMotion_
        );
        return animatedPrimitive;
    }

    std::vector<Point2> makeScreenPoints(const UIPrimitive& primitive) const {
        std::vector<Point2> screenPoints;
        if (points_.empty()) {
            return screenPoints;
        }

        const RectFrame frame = PrimitiveFrame(primitive);
        const float centerX = frame.x + frame.width * 0.5f;
        const float centerY = frame.y + frame.height * 0.5f;
        const float radians = primitive.rotation * 0.017453292519943295f;
        const float c = std::cos(radians);
        const float s = std::sin(radians);

        screenPoints.reserve(points_.size());
        for (const Point2& point : points_) {
            const float localX = frame.x + point.x * frame.width;
            const float localY = frame.y + point.y * frame.height;
            const float dx = (localX - centerX) * primitive.scaleX;
            const float dy = (localY - centerY) * primitive.scaleY;
            screenPoints.push_back(Point2{
                centerX + dx * c - dy * s + primitive.translateX,
                centerY + dx * s + dy * c + primitive.translateY
            });
        }
        return screenPoints;
    }

    std::vector<Point2> points_;
    ScalarMotionSpec scaleMotion_;
    ScalarMotionSpec rotationMotion_;
    ScalarMotionSpec opacityMotion_;
    ScalarMotionSpec translateXMotion_;
    ScalarMotionSpec translateYMotion_;
    ColorMotionSpec backgroundMotion_;
    HoverScalarMotionSpec hoverScaleMotion_;
    HoverScalarMotionSpec hoverRotationMotion_;
    HoverScalarMotionSpec hoverOpacityMotion_;
    HoverScalarMotionSpec hoverTranslateXMotion_;
    HoverScalarMotionSpec hoverTranslateYMotion_;
    HoverColorMotionSpec hoverBackgroundMotion_;
    PrimitiveMotionState motion_;
};

} // namespace EUINEO
