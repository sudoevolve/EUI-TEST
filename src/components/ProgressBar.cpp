#include "ProgressBar.h"
#include <cmath>

namespace EUINEO {

ProgressBar::ProgressBar(float x, float y, float w, float h) {
    this->x = x;
    this->y = y;
    this->width = w;
    this->height = h;
}

void ProgressBar::Update() {
    if (animatedValue < 0.0f) {
        animatedValue = value;
    }
    if (std::abs(animatedValue - value) > 0.001f) {
        animatedValue = Lerp(animatedValue, value, 10.0f * State.deltaTime);
        MarkDirty(2.0f);
    }
}

void ProgressBar::Draw() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);

    float cornerRadius = height / 2.0f;
    Renderer::DrawRect(absX, absY, width, height, CurrentTheme->surfaceHover, cornerRadius);
    if (animatedValue > 0.01f) {
        Renderer::DrawRect(absX, absY, width * animatedValue, height, CurrentTheme->primary, cornerRadius);
    }
}

} // namespace EUINEO
