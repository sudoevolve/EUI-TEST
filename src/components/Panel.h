#pragma once

#include "../EUINEO.h"
#include "PrimitiveMotion.h"
#include "../ui/UIBuilder.h"
#include <algorithm>

namespace EUINEO {

template <typename NodeT>
class PanelBuilderBase : public UIBuilderBase<NodeT, PanelBuilderBase<NodeT>> {
public:
    PanelBuilderBase(UIContext& context, NodeT& node)
        : UIBuilderBase<NodeT, PanelBuilderBase<NodeT>>(context, node) {}

    PanelBuilderBase& animateScale(float from, float to, float duration,
                                   Easing easing = Easing::EaseInOut,
                                   bool loop = true, bool pingPong = true) {
        this->node_.trackComposeValue("scaleMotionFrom", from);
        this->node_.trackComposeValue("scaleMotionTo", to);
        this->node_.trackComposeValue("scaleMotionDuration", duration);
        this->node_.trackComposeValue("scaleMotionEasing", easing);
        this->node_.trackComposeValue("scaleMotionLoop", loop);
        this->node_.trackComposeValue("scaleMotionPingPong", pingPong);
        this->node_.setScaleMotion(ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong});
        return *this;
    }

    PanelBuilderBase& animateRotation(float from, float to, float duration,
                                      Easing easing = Easing::EaseInOut,
                                      bool loop = true, bool pingPong = true) {
        this->node_.trackComposeValue("rotationMotionFrom", from);
        this->node_.trackComposeValue("rotationMotionTo", to);
        this->node_.trackComposeValue("rotationMotionDuration", duration);
        this->node_.trackComposeValue("rotationMotionEasing", easing);
        this->node_.trackComposeValue("rotationMotionLoop", loop);
        this->node_.trackComposeValue("rotationMotionPingPong", pingPong);
        this->node_.setRotationMotion(ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong});
        return *this;
    }

    PanelBuilderBase& animateOpacity(float from, float to, float duration,
                                     Easing easing = Easing::EaseInOut,
                                     bool loop = true, bool pingPong = true) {
        this->node_.trackComposeValue("opacityMotionFrom", from);
        this->node_.trackComposeValue("opacityMotionTo", to);
        this->node_.trackComposeValue("opacityMotionDuration", duration);
        this->node_.trackComposeValue("opacityMotionEasing", easing);
        this->node_.trackComposeValue("opacityMotionLoop", loop);
        this->node_.trackComposeValue("opacityMotionPingPong", pingPong);
        this->node_.setOpacityMotion(ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong});
        return *this;
    }

    PanelBuilderBase& animateTranslateX(float from, float to, float duration,
                                        Easing easing = Easing::EaseInOut,
                                        bool loop = true, bool pingPong = true) {
        this->node_.trackComposeValue("translateXMotionFrom", from);
        this->node_.trackComposeValue("translateXMotionTo", to);
        this->node_.trackComposeValue("translateXMotionDuration", duration);
        this->node_.trackComposeValue("translateXMotionEasing", easing);
        this->node_.trackComposeValue("translateXMotionLoop", loop);
        this->node_.trackComposeValue("translateXMotionPingPong", pingPong);
        this->node_.setTranslateXMotion(ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong});
        return *this;
    }

    PanelBuilderBase& animateTranslateY(float from, float to, float duration,
                                        Easing easing = Easing::EaseInOut,
                                        bool loop = true, bool pingPong = true) {
        this->node_.trackComposeValue("translateYMotionFrom", from);
        this->node_.trackComposeValue("translateYMotionTo", to);
        this->node_.trackComposeValue("translateYMotionDuration", duration);
        this->node_.trackComposeValue("translateYMotionEasing", easing);
        this->node_.trackComposeValue("translateYMotionLoop", loop);
        this->node_.trackComposeValue("translateYMotionPingPong", pingPong);
        this->node_.setTranslateYMotion(ScalarMotionSpec{true, from, to, duration, easing, loop, pingPong});
        return *this;
    }

    PanelBuilderBase& animateBackground(const Color& from, const Color& to, float duration,
                                        Easing easing = Easing::EaseInOut,
                                        bool loop = true, bool pingPong = true) {
        this->node_.trackComposeValue("backgroundMotionFrom", from);
        this->node_.trackComposeValue("backgroundMotionTo", to);
        this->node_.trackComposeValue("backgroundMotionDuration", duration);
        this->node_.trackComposeValue("backgroundMotionEasing", easing);
        this->node_.trackComposeValue("backgroundMotionLoop", loop);
        this->node_.trackComposeValue("backgroundMotionPingPong", pingPong);
        this->node_.setBackgroundMotion(ColorMotionSpec{true, from, to, duration, easing, loop, pingPong});
        return *this;
    }

    PanelBuilderBase& hoverScale(float idle, float hover, float duration,
                                 Easing easing = Easing::EaseInOut) {
        this->node_.trackComposeValue("hoverScaleIdle", idle);
        this->node_.trackComposeValue("hoverScaleHover", hover);
        this->node_.trackComposeValue("hoverScaleDuration", duration);
        this->node_.trackComposeValue("hoverScaleEasing", easing);
        this->node_.setHoverScaleMotion(HoverScalarMotionSpec{true, idle, hover, duration, easing});
        return *this;
    }

    PanelBuilderBase& hoverRotation(float idle, float hover, float duration,
                                    Easing easing = Easing::EaseInOut) {
        this->node_.trackComposeValue("hoverRotationIdle", idle);
        this->node_.trackComposeValue("hoverRotationHover", hover);
        this->node_.trackComposeValue("hoverRotationDuration", duration);
        this->node_.trackComposeValue("hoverRotationEasing", easing);
        this->node_.setHoverRotationMotion(HoverScalarMotionSpec{true, idle, hover, duration, easing});
        return *this;
    }

    PanelBuilderBase& hoverOpacity(float idle, float hover, float duration,
                                   Easing easing = Easing::EaseInOut) {
        this->node_.trackComposeValue("hoverOpacityIdle", idle);
        this->node_.trackComposeValue("hoverOpacityHover", hover);
        this->node_.trackComposeValue("hoverOpacityDuration", duration);
        this->node_.trackComposeValue("hoverOpacityEasing", easing);
        this->node_.setHoverOpacityMotion(HoverScalarMotionSpec{true, idle, hover, duration, easing});
        return *this;
    }

    PanelBuilderBase& hoverTranslateX(float idle, float hover, float duration,
                                      Easing easing = Easing::EaseInOut) {
        this->node_.trackComposeValue("hoverTranslateXIdle", idle);
        this->node_.trackComposeValue("hoverTranslateXHover", hover);
        this->node_.trackComposeValue("hoverTranslateXDuration", duration);
        this->node_.trackComposeValue("hoverTranslateXEasing", easing);
        this->node_.setHoverTranslateXMotion(HoverScalarMotionSpec{true, idle, hover, duration, easing});
        return *this;
    }

    PanelBuilderBase& hoverTranslateY(float idle, float hover, float duration,
                                      Easing easing = Easing::EaseInOut) {
        this->node_.trackComposeValue("hoverTranslateYIdle", idle);
        this->node_.trackComposeValue("hoverTranslateYHover", hover);
        this->node_.trackComposeValue("hoverTranslateYDuration", duration);
        this->node_.trackComposeValue("hoverTranslateYEasing", easing);
        this->node_.setHoverTranslateYMotion(HoverScalarMotionSpec{true, idle, hover, duration, easing});
        return *this;
    }

    PanelBuilderBase& hoverBackground(const Color& idle, const Color& hover, float duration,
                                      Easing easing = Easing::EaseInOut) {
        this->node_.trackComposeValue("hoverBackgroundIdle", idle);
        this->node_.trackComposeValue("hoverBackgroundHover", hover);
        this->node_.trackComposeValue("hoverBackgroundDuration", duration);
        this->node_.trackComposeValue("hoverBackgroundEasing", easing);
        this->node_.setHoverBackgroundMotion(HoverColorMotionSpec{true, idle, hover, duration, easing});
        return *this;
    }
};

class PanelNode : public UINode {
public:
    using Builder = PanelBuilderBase<PanelNode>;

    explicit PanelNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "PanelNode";
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
        const RectFrame frame = PrimitiveFrame(animatedPrimitive);
        const RectStyle style = MakeStyle(animatedPrimitive);
        const RectBounds bounds = Renderer::MeasureRectBounds(frame.x, frame.y, frame.width, frame.height, style);
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
        if (animatedPrimitive.blur > 0.0f) {
            Renderer::CaptureBackdrop();
        }

        PrimitiveClipScope clip(animatedPrimitive);
        const RectFrame frame = PrimitiveFrame(animatedPrimitive);
        const RectStyle style = MakeStyle(animatedPrimitive);
        drawPanel(frame, style, animatedPrimitive.borderWidth, ApplyOpacity(animatedPrimitive.borderColor, animatedPrimitive.opacity));
    }

    void setScaleMotion(const ScalarMotionSpec& value) {
        scaleMotion_ = value;
    }

    void setRotationMotion(const ScalarMotionSpec& value) {
        rotationMotion_ = value;
    }

    void setOpacityMotion(const ScalarMotionSpec& value) {
        opacityMotion_ = value;
    }

    void setTranslateXMotion(const ScalarMotionSpec& value) {
        translateXMotion_ = value;
    }

    void setTranslateYMotion(const ScalarMotionSpec& value) {
        translateYMotion_ = value;
    }

    void setBackgroundMotion(const ColorMotionSpec& value) {
        backgroundMotion_ = value;
    }

    void setHoverScaleMotion(const HoverScalarMotionSpec& value) {
        hoverScaleMotion_ = value;
    }

    void setHoverRotationMotion(const HoverScalarMotionSpec& value) {
        hoverRotationMotion_ = value;
    }

    void setHoverOpacityMotion(const HoverScalarMotionSpec& value) {
        hoverOpacityMotion_ = value;
    }

    void setHoverTranslateXMotion(const HoverScalarMotionSpec& value) {
        hoverTranslateXMotion_ = value;
    }

    void setHoverTranslateYMotion(const HoverScalarMotionSpec& value) {
        hoverTranslateYMotion_ = value;
    }

    void setHoverBackgroundMotion(const HoverColorMotionSpec& value) {
        hoverBackgroundMotion_ = value;
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.background = CurrentTheme->surface;
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
    bool motionHovered() const {
        if (!primitive_.enabled) {
            return false;
        }
        const RectFrame frame = PrimitiveFrame(primitive_);
        const RectStyle style = MakeStyle(primitive_);
        const RectBounds bounds = Renderer::MeasureRectBounds(frame.x, frame.y, frame.width, frame.height, style);
        return State.mouseX >= bounds.x && State.mouseX <= bounds.x + bounds.w &&
               State.mouseY >= bounds.y && State.mouseY <= bounds.y + bounds.h;
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

    static void drawPanel(const RectFrame& frame, const RectStyle& fillStyle,
                          float borderWidth, const Color& borderColor) {
        if (borderWidth > 0.0f && borderColor.a > 0.0f &&
            frame.width > borderWidth * 2.0f && frame.height > borderWidth * 2.0f) {
            RectStyle borderStyle;
            borderStyle.color = borderColor;
            borderStyle.rounding = fillStyle.rounding;
            borderStyle.transform = fillStyle.transform;
            Renderer::DrawRect(frame.x, frame.y, frame.width, frame.height, borderStyle);

            RectStyle innerStyle = fillStyle;
            innerStyle.rounding = std::max(0.0f, fillStyle.rounding - borderWidth);
            Renderer::DrawRect(frame.x + borderWidth, frame.y + borderWidth,
                               frame.width - borderWidth * 2.0f, frame.height - borderWidth * 2.0f, innerStyle);
            return;
        }

        Renderer::DrawRect(frame.x, frame.y, frame.width, frame.height, fillStyle);
    }

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

class GlassPanelNode : public PanelNode {
public:
    using Builder = PanelBuilderBase<GlassPanelNode>;
    using PanelNode::PanelNode;

    static constexpr const char* StaticTypeName() {
        return "GlassPanelNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

protected:
    void resetDefaults() override {
        PanelNode::resetDefaults();
        primitive_.background = Color(CurrentTheme->surface.r, CurrentTheme->surface.g, CurrentTheme->surface.b, 0.60f);
        primitive_.rounding = 18.0f;
        primitive_.shadow.blur = CurrentTheme == &DarkTheme ? 20.0f : 18.0f;
        primitive_.shadow.offsetY = CurrentTheme == &DarkTheme ? 10.0f : 8.0f;
        primitive_.shadow.color = CurrentTheme == &DarkTheme
            ? Color(0.0f, 0.0f, 0.0f, 0.30f)
            : Color(0.10f, 0.14f, 0.22f, 0.18f);
    }
};

class PopupPanelNode : public PanelNode {
public:
    using Builder = PanelBuilderBase<PopupPanelNode>;
    using PanelNode::PanelNode;

    static constexpr const char* StaticTypeName() {
        return "PopupPanelNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

protected:
    void resetDefaults() override {
        PanelNode::resetDefaults();
        applyPopupPresentationDefaults();
        primitive_.rounding = 14.0f;
        primitive_.shadow.blur = CurrentTheme == &DarkTheme ? 24.0f : 18.0f;
        primitive_.shadow.offsetY = CurrentTheme == &DarkTheme ? 12.0f : 8.0f;
        primitive_.shadow.color = CurrentTheme == &DarkTheme
            ? Color(0.0f, 0.0f, 0.0f, 0.34f)
            : Color(0.10f, 0.14f, 0.22f, 0.16f);
    }
};

} // namespace EUINEO
