#include "Panel.h"

namespace EUINEO {

Panel::Panel(float x, float y, float w, float h) {
    this->x = x;
    this->y = y;
    this->width = w;
    this->height = h;
    this->color = CurrentTheme ? CurrentTheme->surface : Color(1, 1, 1, 1);
}

void Panel::Update() {
    // 静态面板无需特别的更新逻辑
}

void Panel::Draw() {
    float absX, absY;
    GetAbsoluteBounds(absX, absY);
    
    Renderer::DrawRect(
        absX, absY, width, height, 
        color, 
        rounding, 
        blurAmount, 
        shadowBlur, 
        shadowOffsetX, 
        shadowOffsetY, 
        shadowColor
    );
}

} // namespace EUINEO