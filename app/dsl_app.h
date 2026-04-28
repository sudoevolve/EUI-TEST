#pragma once

#include "app/app.h"

#include <glad/glad.h>

#include "3rd/stb_image.h"
#include "core/dsl_runtime.h"
#include "core/network.h"

#include <filesystem>
#include <string>
#include <vector>

namespace app {

struct DslAppConfig {
    const char* title = "App";
    const char* pageId = "app";
    core::Color clearColor = {0.16f, 0.18f, 0.20f, 1.0f};
    int windowWidth = 800;
    int windowHeight = 600;
    bool showFrameCountInTitle = false;
    double fps = 0.0;
    const char* iconPath = "assets/icon.png";
};

const DslAppConfig& dslAppConfig();
void compose(core::dsl::Ui& ui, const core::dsl::Screen& screen);

namespace detail {

inline core::dsl::Runtime& dslRuntime() {
    static core::dsl::Runtime runtime;
    return runtime;
}

struct DslAppState {
    bool composed = false;
    bool iconApplied = false;
    float logicalWidth = 0.0f;
    float logicalHeight = 0.0f;
};

inline DslAppState& dslAppState() {
    static DslAppState state;
    return state;
}

inline std::string resolveIconPath(const char* iconPath) {
    if (iconPath == nullptr || iconPath[0] == '\0') {
        return {};
    }

    namespace fs = std::filesystem;
    std::error_code error;
    const fs::path requested(iconPath);
    const fs::path current = fs::current_path(error);
    std::vector<fs::path> candidates;
    candidates.push_back(requested);
    if (!error) {
        candidates.push_back(current / requested);
        candidates.push_back(current / "assets" / requested.filename());
    }

    for (const fs::path& candidate : candidates) {
        error.clear();
        if (fs::exists(candidate, error) && !error) {
            return fs::absolute(candidate, error).string();
        }
    }
    return {};
}

inline void applyWindowIcon(GLFWwindow* window) {
    if (window == nullptr) {
        return;
    }

    const std::string iconPath = resolveIconPath(dslAppConfig().iconPath);
    if (iconPath.empty()) {
        return;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* pixels = stbi_load(iconPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }
        return;
    }

    GLFWimage image{};
    image.width = width;
    image.height = height;
    image.pixels = pixels;
    glfwSetWindowIcon(window, 1, &image);
    stbi_image_free(pixels);
}

} // namespace detail

const char* windowTitle() {
    return dslAppConfig().title;
}

bool showFrameCountInTitle() {
    return dslAppConfig().showFrameCountInTitle;
}

double frameRateLimit() {
    return dslAppConfig().fps;
}

int initialWindowWidth() {
    return dslAppConfig().windowWidth;
}

int initialWindowHeight() {
    return dslAppConfig().windowHeight;
}

bool initialize(GLFWwindow* window) {
    detail::DslAppState& state = detail::dslAppState();
    if (!state.iconApplied) {
        detail::applyWindowIcon(window);
        state.iconApplied = true;
    }
    return detail::dslRuntime().initialize(window);
}

bool update(GLFWwindow* window, float deltaSeconds, int windowWidth, int windowHeight, float dpiScale, float pointerScale) {
    if (windowWidth <= 0 || windowHeight <= 0 || dpiScale <= 0.0f) {
        return false;
    }

    const DslAppConfig& config = dslAppConfig();
    const float logicalWidth = static_cast<float>(windowWidth) / dpiScale;
    const float logicalHeight = static_cast<float>(windowHeight) / dpiScale;
    detail::DslAppState& state = detail::dslAppState();

    const auto composeFrame = [&] {
        detail::dslRuntime().compose(config.pageId, logicalWidth, logicalHeight, [](core::dsl::Ui& ui, const core::dsl::Screen& screen) {
            compose(ui, screen);
        });
        state.composed = true;
        state.logicalWidth = logicalWidth;
        state.logicalHeight = logicalHeight;
    };

    if (!state.composed || state.logicalWidth != logicalWidth || state.logicalHeight != logicalHeight) {
        composeFrame();
    }

    bool changed = false;
    if (core::network::consumeAnyTextReady()) {
        composeFrame();
        detail::dslRuntime().markFullRedraw();
        changed = true;
    }

    changed = detail::dslRuntime().update(window, deltaSeconds, pointerScale, dpiScale) || changed;
    if (detail::dslRuntime().needsCompose()) {
        composeFrame();
        changed = detail::dslRuntime().update(window, 0.0f, pointerScale, dpiScale) || changed;
        changed = true;
    }

    return changed;
}

bool isAnimating() {
    return detail::dslRuntime().isAnimating();
}

void render(int windowWidth, int windowHeight, float dpiScale) {
    if (windowWidth <= 0 || windowHeight <= 0 || dpiScale <= 0.0f) {
        return;
    }

    const core::Color clearColor = dslAppConfig().clearColor;
    detail::dslRuntime().render(windowWidth, windowHeight, dpiScale, clearColor);
}

void shutdown() {
    detail::dslRuntime().shutdown();
    core::network::shutdown();
}

} // namespace app
