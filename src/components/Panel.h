#pragma once
#include "../EUINEO.h"

namespace EUINEO {

class Panel : public Widget {
public:
    Color color;
    float rounding = 0.0f;
    float blurAmount = 0.0f;
    float shadowBlur = 0.0f;
    float shadowOffsetX = 0.0f;
    float shadowOffsetY = 0.0f;
    Color shadowColor = Color(0, 0, 0, 0);

    Panel() = default;
    Panel(float x, float y, float w, float h);

    void Update() override;
    void Draw() override;
};

} // namespace EUINEO