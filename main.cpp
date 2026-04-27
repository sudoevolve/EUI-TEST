#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "app/app.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

struct WindowState {
    bool needsRender = true;
    int renderedFrames = 0;
    double lastTitleUpdate = 0.0;
    double nextFrameTime = 0.0;
    double frameInterval = 1.0 / 60.0;
    double lastFrameRateLimit = 0.0;
    double lastRefreshRateUpdate = 0.0;
};

struct TimerResolutionGuard {
    TimerResolutionGuard() {
#ifdef _WIN32
        timeBeginPeriod(1);
#endif
    }

    ~TimerResolutionGuard() {
#ifdef _WIN32
        timeEndPeriod(1);
#endif
    }
};

float getDpiScale(GLFWwindow* window) {
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    glfwGetWindowContentScale(window, &scaleX, &scaleY);
    return (scaleX + scaleY) * 0.5f;
}

float getPointerScale(GLFWwindow* window) {
    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    if (windowWidth <= 0 || windowHeight <= 0) {
        return 1.0f;
    }

    const float scaleX = static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth);
    const float scaleY = static_cast<float>(framebufferHeight) / static_cast<float>(windowHeight);
    return (scaleX + scaleY) * 0.5f;
}

GLFWmonitor* getWindowMonitor(GLFWwindow* window) {
    if (GLFWmonitor* monitor = glfwGetWindowMonitor(window)) {
        return monitor;
    }

    int windowX = 0;
    int windowY = 0;
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowPos(window, &windowX, &windowY);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    GLFWmonitor* bestMonitor = glfwGetPrimaryMonitor();
    int bestArea = 0;

    for (int i = 0; i < monitorCount; ++i) {
        GLFWmonitor* monitor = monitors[i];
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode) {
            continue;
        }

        const int overlapLeft = std::max(windowX, monitorX);
        const int overlapTop = std::max(windowY, monitorY);
        const int overlapRight = std::min(windowX + windowWidth, monitorX + mode->width);
        const int overlapBottom = std::min(windowY + windowHeight, monitorY + mode->height);
        const int overlapWidth = std::max(0, overlapRight - overlapLeft);
        const int overlapHeight = std::max(0, overlapBottom - overlapTop);
        const int overlapArea = overlapWidth * overlapHeight;
        if (overlapArea > bestArea) {
            bestArea = overlapArea;
            bestMonitor = monitor;
        }
    }

    return bestMonitor;
}

double getWindowRefreshRate(GLFWwindow* window) {
    GLFWmonitor* monitor = getWindowMonitor(window);
    const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (mode && mode->refreshRate > 0) {
        return static_cast<double>(mode->refreshRate);
    }
    return 60.0;
}

void updateFrameInterval(GLFWwindow* window, WindowState& windowState, double now, bool force = false) {
    const double limit = app::frameRateLimit();
    if (!force && limit == windowState.lastFrameRateLimit && now - windowState.lastRefreshRateUpdate < 0.5) {
        return;
    }

    double refreshRate = std::clamp(getWindowRefreshRate(window), 30.0, 500.0);
    if (limit > 0.0) {
        refreshRate = std::min(refreshRate, limit);
    }
    windowState.frameInterval = 1.0 / std::max(1.0, refreshRate);
    windowState.lastFrameRateLimit = limit;
    windowState.lastRefreshRateUpdate = now;
}

void waitForNextFrame(GLFWwindow* window, const WindowState& windowState) {
    while (!glfwWindowShouldClose(window)) {
        const double remaining = windowState.nextFrameTime - glfwGetTime();
        if (remaining <= 0.0) {
            break;
        }

        if (remaining > 0.002) {
            glfwWaitEventsTimeout(remaining - 0.001);
        } else {
            std::this_thread::sleep_for(std::chrono::duration<double>(remaining * 0.5));
        }
    }
}

int main() {
    glfwInit();
    TimerResolutionGuard timerResolution;

    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(app::initialWindowWidth(), app::initialWindowHeight(), app::windowTitle(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    WindowState windowState;
    windowState.lastTitleUpdate = glfwGetTime();
    windowState.nextFrameTime = windowState.lastTitleUpdate;
    updateFrameInterval(window, windowState, windowState.lastTitleUpdate, true);
    if (app::showFrameCountInTitle()) {
        char title[128];
        std::snprintf(title, sizeof(title), "%s - 0 FPS", app::windowTitle());
        glfwSetWindowTitle(window, title);
    }
    glfwSetWindowUserPointer(window, &windowState);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* currentWindow, int w, int h) {
        glViewport(0, 0, w, h);
        static_cast<WindowState*>(glfwGetWindowUserPointer(currentWindow))->needsRender = true;
    });
    glfwSetWindowRefreshCallback(window, [](GLFWwindow* currentWindow) {
        static_cast<WindowState*>(glfwGetWindowUserPointer(currentWindow))->needsRender = true;
    });
    glfwSetWindowContentScaleCallback(window, [](GLFWwindow* currentWindow, float, float) {
        static_cast<WindowState*>(glfwGetWindowUserPointer(currentWindow))->needsRender = true;
    });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return -1;
    }

    if (!app::initialize(window)) {
        glfwTerminate();
        return -1;
    }

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        const bool animatingAtFrameStart = app::isAnimating();
        if (animatingAtFrameStart) {
            waitForNextFrame(window, windowState);
        }

        const double currentFrameTime = glfwGetTime();
        updateFrameInterval(window, windowState, currentFrameTime);
        const float deltaSeconds = static_cast<float>(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
            break;
        }

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        const float dpiScale = getDpiScale(window);
        const float pointerScale = getPointerScale(window);

        if (app::update(window, deltaSeconds, framebufferWidth, framebufferHeight, dpiScale, pointerScale)) {
            windowState.needsRender = true;
        }

        if (windowState.needsRender) {
            app::render(framebufferWidth, framebufferHeight, dpiScale);
            glfwSwapBuffers(window);
            windowState.needsRender = false;
            ++windowState.renderedFrames;
        }

        if (app::showFrameCountInTitle()) {
            const double titleElapsed = glfwGetTime() - windowState.lastTitleUpdate;
            if (titleElapsed >= 1.0) {
                const double fps = static_cast<double>(windowState.renderedFrames) / titleElapsed;
                char title[128];
                std::snprintf(title, sizeof(title), "%s - %.0f FPS", app::windowTitle(), fps);
                glfwSetWindowTitle(window, title);
                windowState.renderedFrames = 0;
                windowState.lastTitleUpdate = glfwGetTime();
            }
        }

        if (app::isAnimating()) {
            const double now = glfwGetTime();
            windowState.nextFrameTime += windowState.frameInterval;
            if (windowState.nextFrameTime <= now || windowState.nextFrameTime > now + windowState.frameInterval * 2.0) {
                windowState.nextFrameTime = now + windowState.frameInterval;
            }
            glfwPollEvents();
        } else {
            glfwWaitEvents();
            windowState.nextFrameTime = glfwGetTime();
        }
    }

    app::shutdown();
    glfwTerminate();
    return 0;
}
