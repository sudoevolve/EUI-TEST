#pragma once

#include "app/app.h"

#include <glad/glad.h>

#include "core/dsl_runtime.h"

namespace app {

struct DslAppConfig {
    const char* title = "App";
    const char* pageId = "app";
    core::Color clearColor = {0.16f, 0.18f, 0.20f, 1.0f};
    int windowWidth = 800;
    int windowHeight = 600;
    bool showFrameCountInTitle = false;
    double (*frameRateLimit)() = nullptr;
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
    float logicalWidth = 0.0f;
    float logicalHeight = 0.0f;
};

inline DslAppState& dslAppState() {
    static DslAppState state;
    return state;
}

} // namespace detail

const char* windowTitle() {
    return dslAppConfig().title;
}

bool showFrameCountInTitle() {
    return dslAppConfig().showFrameCountInTitle;
}

double frameRateLimit() {
    const DslAppConfig& config = dslAppConfig();
    return config.frameRateLimit ? config.frameRateLimit() : 0.0;
}

int initialWindowWidth() {
    return dslAppConfig().windowWidth;
}

int initialWindowHeight() {
    return dslAppConfig().windowHeight;
}

bool initialize(GLFWwindow*) {
    return detail::dslRuntime().initialize();
}

bool update(GLFWwindow* window, float deltaSeconds, int windowWidth, int windowHeight, float dpiScale, float pointerScale) {
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

    bool changed = detail::dslRuntime().update(window, deltaSeconds, pointerScale, dpiScale);
    if (detail::dslRuntime().needsCompose()) {
        composeFrame();
        changed = detail::dslRuntime().update(window, 0.0f, pointerScale, dpiScale) || changed;
        detail::dslRuntime().markFullRedraw();
        changed = true;
    }

    return changed;
}

bool isAnimating() {
    return detail::dslRuntime().isAnimating();
}

void render(int windowWidth, int windowHeight, float dpiScale) {
    const core::Color clearColor = dslAppConfig().clearColor;
    detail::dslRuntime().render(windowWidth, windowHeight, dpiScale, clearColor);
}

void shutdown() {
    detail::dslRuntime().shutdown();
}

} // namespace app
