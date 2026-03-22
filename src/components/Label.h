#pragma once

#include "../EUINEO.h"
#include "../ui/UIBuilder.h"
#include <string>
#include <utility>

namespace EUINEO {

class LabelNode : public UINode {
public:
    class Builder : public UIBuilderBase<LabelNode, Builder> {
    public:
        Builder(UIContext& context, LabelNode& node) : UIBuilderBase<LabelNode, Builder>(context, node) {}

        Builder& text(std::string value) {
            this->node_.text_ = std::move(value);
            return *this;
        }

        Builder& fontSize(float value) {
            this->node_.fontSize_ = value;
            return *this;
        }

        Builder& color(const Color& value) {
            this->node_.color_ = value;
            this->node_.useThemeColor_ = false;
            return *this;
        }
    };

    explicit LabelNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "LabelNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    void update() override {}

    void draw() override {
        PrimitiveClipScope clip(primitive_);
        const RectFrame frame = PrimitiveFrame(primitive_);
        const float textScale = fontSize_ / 24.0f;
        const Color drawColor = useThemeColor_
            ? ApplyOpacity(CurrentTheme->text, primitive_.opacity)
            : ApplyOpacity(color_, primitive_.opacity);

        Renderer::DrawTextStr(
            text_,
            frame.x + primitive_.translateX,
            frame.y + primitive_.translateY,
            drawColor,
            textScale,
            primitive_.rotation
        );
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        text_.clear();
        fontSize_ = 24.0f;
        color_ = CurrentTheme->text;
        useThemeColor_ = true;
    }

private:
    std::string text_;
    float fontSize_ = 24.0f;
    Color color_ = Color(1.0f, 1.0f, 1.0f, 1.0f);
    bool useThemeColor_ = true;
};

} // namespace EUINEO
