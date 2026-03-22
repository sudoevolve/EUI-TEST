#pragma once

#include "../EUINEO.h"
#include "../ui/UIBuilder.h"
#include <algorithm>

namespace EUINEO {

class PanelNode : public UINode {
public:
    using Builder = DefaultNodeBuilder<PanelNode>;

    explicit PanelNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "PanelNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    void update() override {}

    void draw() override {
        if (primitive_.blur > 0.0f) {
            Renderer::CaptureBackdrop();
        }

        PrimitiveClipScope clip(primitive_);
        const RectFrame frame = PrimitiveFrame(primitive_);
        const RectStyle style = MakeStyle(primitive_);
        drawPanel(frame, style, primitive_.borderWidth, ApplyOpacity(primitive_.borderColor, primitive_.opacity));
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.background = CurrentTheme->surface;
    }

private:
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
};

class GlassPanelNode : public PanelNode {
public:
    using Builder = DefaultNodeBuilder<GlassPanelNode>;
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

} // namespace EUINEO
