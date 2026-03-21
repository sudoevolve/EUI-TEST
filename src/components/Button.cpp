#include "Button.h"
#include <cmath>

namespace EUINEO {

Button::Button(std::string t, float x, float y, float w, float h) {
    this->text = t;
    this->x = x;
    this->y = y;
    this->width = w;
    this->height = h;
}

void Button::Update() {
    bool hovered = IsHovered();
    float speed = 15.0f * State.deltaTime;

    float targetHover = hovered ? 1.0f : 0.0f;
    if (std::abs(hoverAnim - targetHover) > 0.001f) {
        hoverAnim = Lerp(hoverAnim, targetHover, speed);
        if (std::abs(hoverAnim - targetHover) < 0.001f) hoverAnim = targetHover;
        MarkDirty(6.0f);
    }

    float targetClick = (hovered && State.mouseDown) ? 1.0f : 0.0f;
    if (std::abs(clickAnim - targetClick) > 0.001f) {
        clickAnim = Lerp(clickAnim, targetClick, speed * 2.0f);
        if (std::abs(clickAnim - targetClick) < 0.001f) clickAnim = targetClick;
        MarkDirty(6.0f);
    }

    if (hovered && State.mouseClicked && onClick) {
        onClick();
        MarkDirty(6.0f, 0.2f);
    }
}

void Button::Draw() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);

    Color baseColor = (style == ButtonStyle::Primary) ? CurrentTheme->primary : CurrentTheme->surface;
    Color hoverColor = (style == ButtonStyle::Primary)
        ? Lerp(CurrentTheme->primary, Color(1, 1, 1, 1), 0.2f)
        : CurrentTheme->surfaceHover;
    Color activeColor = (style == ButtonStyle::Primary)
        ? Lerp(CurrentTheme->primary, Color(0, 0, 0, 1), 0.2f)
        : CurrentTheme->surfaceActive;

    Color drawColor = Lerp(baseColor, hoverColor, hoverAnim);
    drawColor = Lerp(drawColor, activeColor, clickAnim);

    float cornerRadius = 6.0f;
    if (style == ButtonStyle::Outline) {
        Renderer::DrawRect(absX, absY, width, height, CurrentTheme->border, cornerRadius);
        Renderer::DrawRect(absX + 1, absY + 1, width - 2, height - 2, CurrentTheme->background, cornerRadius - 1.0f);
    } else {
        Renderer::DrawRect(absX, absY, width, height, drawColor, cornerRadius);
    }

    float textScale = fontSize / 24.0f;
    float textWidth = Renderer::MeasureTextWidth(text, textScale);
    float textX = absX + (width - textWidth) / 2.0f;
    float textY = absY + (height / 2.0f) + (fontSize / 4.0f);
    Color textColor = (style == ButtonStyle::Primary) ? Color(1, 1, 1, 1) : CurrentTheme->text;
    Renderer::DrawTextStr(text, textX, textY, textColor, textScale);
}

} // namespace EUINEO
