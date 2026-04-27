#pragma once

#include "core/dsl.h"
#include "core/text.h"

#include <string>

namespace components {

using TextStyle = core::TextStyle;

inline core::dsl::TextBuilder text(core::dsl::Ui& ui, const std::string& id) {
    return ui.text(id);
}

inline core::dsl::TextBuilder label(core::dsl::Ui& ui, const std::string& id) {
    return ui.label(id);
}

inline core::dsl::TextBuilder text(core::dsl::Ui& ui, const std::string& id, const TextStyle& style) {
    auto builder = ui.text(id);
    builder.text(style.text)
        .fontFamily(style.fontFamily)
        .fontSize(style.fontSize)
        .fontWeight(style.fontWeight)
        .color(style.color)
        .maxWidth(style.maxWidth)
        .wrap(style.wrap)
        .horizontalAlign(style.horizontalAlign)
        .verticalAlign(style.verticalAlign)
        .lineHeight(style.lineHeight);
    return builder;
}

} // namespace components
