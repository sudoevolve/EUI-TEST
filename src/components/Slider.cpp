#include "Slider.h"
#include <algorithm>
#include <cmath>

namespace EUINEO {

Slider::Slider(float x, float y, float w, float h) {
    this->x = x;
    this->y = y;
    this->width = w;
    this->height = h;
}

void Slider::Update() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);

    float handleW = 10.0f;
    float handleX = absX + value * width - handleW / 2.0f;
    float handleY = absY + (height - 16.0f) / 2.0f;

    bool isHovered = (State.mouseX >= handleX && State.mouseX <= handleX + handleW &&
                      State.mouseY >= handleY && State.mouseY <= handleY + 16.0f);

    float targetHover = (isHovered || isDragging) ? 1.0f : 0.0f;
    if (std::abs(hoverAnim - targetHover) > 0.001f) {
        hoverAnim = Lerp(hoverAnim, targetHover, State.deltaTime * ((isHovered || isDragging) ? 15.0f : 10.0f));
        if (std::abs(hoverAnim - targetHover) < 0.001f) hoverAnim = targetHover;
        MarkDirty(12.0f);
    }

    if (isHovered && State.mouseDown && !State.mouseClicked) {
        isDragging = true;
    }
    if (!State.mouseDown) {
        isDragging = false;
    }

    if (isDragging) {
        float relX = State.mouseX - absX;
        float newVal = std::clamp(relX / width, 0.0f, 1.0f);
        if (std::abs(value - newVal) > 0.0001f) {
            value = newVal;
            if (onValueChanged) onValueChanged(value);
            MarkDirty(12.0f);
        }
    }
}

void Slider::Draw() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);

    float trackH = 4.0f;
    float trackY = absY + (height - trackH) / 2.0f;
    Renderer::DrawRect(absX, trackY, width, trackH, CurrentTheme->surface, trackH / 2.0f);

    if (value > 0.01f) {
        Renderer::DrawRect(absX, trackY, width * value, trackH, CurrentTheme->primary, trackH / 2.0f);
    }

    float handleRadius = 8.0f;
    float handleX = absX + value * width - handleRadius;
    float handleY = absY + (height - handleRadius * 2.0f) / 2.0f;
    Color handleColor = Lerp(CurrentTheme->text, CurrentTheme->primary, hoverAnim);
    Renderer::DrawRect(handleX, handleY, handleRadius * 2.0f, handleRadius * 2.0f, handleColor, handleRadius);
}

} // namespace EUINEO
