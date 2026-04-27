#pragma once

#include "core/dsl.h"

#include <string>

namespace components {

inline core::dsl::ImageBuilder image(core::dsl::Ui& ui, const std::string& id) {
    return ui.image(id);
}

} // namespace components
