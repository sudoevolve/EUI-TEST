#include "SegmentedControl.h"
#include <algorithm>
#include <cmath>

namespace EUINEO {

SegmentedControl::SegmentedControl(std::vector<std::string> opts, float x, float y, float w, float h) {
    this->options = opts;
    this->x = x;
    this->y = y;
    this->width = w;
    this->height = h;
}

void SegmentedControl::Update() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);

    bool isHovered = (State.mouseX >= absX && State.mouseX <= absX + width &&
                      State.mouseY >= absY && State.mouseY <= absY + height);

    if (isHovered && State.mouseClicked) {
        float relX = State.mouseX - absX;
        float segW = width / options.size();
        selectedIndex = std::clamp((int)(relX / segW), 0, (int)options.size() - 1);
        if (onSelectionChanged) onSelectionChanged(selectedIndex);
        MarkDirty(4.0f);
    }

    float targetAnim = (float)selectedIndex;
    if (std::abs(indicatorAnim - targetAnim) > 0.001f) {
        indicatorAnim = Lerp(indicatorAnim, targetAnim, State.deltaTime * 15.0f);
        if (std::abs(indicatorAnim - targetAnim) < 0.001f) indicatorAnim = targetAnim;
        MarkDirty(4.0f);
    }
}

void SegmentedControl::Draw() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);

    float cornerRadius = 6.0f;
    Renderer::DrawRect(absX, absY, width, height, CurrentTheme->surface, cornerRadius);

    float segW = width / options.size();
    float indX = absX + indicatorAnim * segW;
    Renderer::DrawRect(indX + 2, absY + 2, segW - 4, height - 4, CurrentTheme->primary, cornerRadius - 1.0f);

    float textScale = fontSize / 24.0f;
    for (size_t i = 0; i < options.size(); ++i) {
        float textWidth = Renderer::MeasureTextWidth(options[i], textScale);
        float textX = absX + i * segW + (segW - textWidth) / 2.0f;
        float textY = absY + (height / 2.0f) + (fontSize / 4.0f);
        Color textColor = (i == std::round(indicatorAnim)) ? Color(1, 1, 1, 1) : CurrentTheme->text;
        Renderer::DrawTextStr(options[i], textX, textY, textColor, textScale);
    }
}

} // namespace EUINEO
