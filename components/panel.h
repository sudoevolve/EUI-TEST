#pragma once

#include "core/dsl.h"

#include <string>

namespace components {

struct PanelStyle {
    core::Color color = {0.08f, 0.10f, 0.13f, 1.0f};
    core::Gradient gradient;
    core::Border border;
    core::Shadow shadow;
    float radius = 12.0f;
    float opacity = 1.0f;
};

inline core::dsl::RectBuilder panel(core::dsl::Ui& ui, const std::string& id) {
    return ui.rect(id);
}

inline core::dsl::RectBuilder panel(core::dsl::Ui& ui, const std::string& id, const PanelStyle& style) {
    auto builder = ui.rect(id);
    builder.color(style.color)
        .gradient(style.gradient)
        .border(style.border)
        .shadow(style.shadow)
        .radius(style.radius)
        .opacity(style.opacity);
    return builder;
}

} // namespace components
