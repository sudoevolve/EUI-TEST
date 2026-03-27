#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "src/EUINEO.h"
#include "src/pages/MainPage.h"

namespace {

int gFramebufferW = 0;
int gFramebufferH = 0;
int gWindowW = 0;
int gWindowH = 0;

void UpdateMousePosition(double x, double y) {
    const float mouseScaleX = gWindowW > 0 ? static_cast<float>(gFramebufferW) / static_cast<float>(gWindowW) : 1.0f;
    const float mouseScaleY = gWindowH > 0 ? static_cast<float>(gFramebufferH) / static_cast<float>(gWindowH) : 1.0f;
    const float nextX = static_cast<float>(x) * mouseScaleX;
    const float nextY = static_cast<float>(y) * mouseScaleY;
    if (std::abs(EUINEO::State.mouseX - nextX) > 0.01f ||
        std::abs(EUINEO::State.mouseY - nextY) > 0.01f) {
        EUINEO::State.pointerMoved = true;
    }
    EUINEO::State.mouseX = nextX;
    EUINEO::State.mouseY = nextY;
}

}

int main() {
    glfwInit();
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

    GLFWwindow* window = glfwCreateWindow(800, 600, "EUI-NEO", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
        glViewport(0, 0, w, h);
        gFramebufferW = w;
        gFramebufferH = h;
        EUINEO::State.screenW = (float)w;
        EUINEO::State.screenH = (float)h;
        EUINEO::Renderer::InvalidateBackdrop();
        EUINEO::Renderer::InvalidateAll();
    });

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    int initialFbW = 0;
    int initialFbH = 0;
    glfwGetFramebufferSize(window, &initialFbW, &initialFbH);
    gFramebufferW = initialFbW;
    gFramebufferH = initialFbH;
    glViewport(0, 0, initialFbW, initialFbH);
    EUINEO::State.screenW = (float)initialFbW;
    EUINEO::State.screenH = (float)initialFbH;
    glfwGetWindowSize(window, &gWindowW, &gWindowH);

    glfwSetWindowSizeCallback(window, [](GLFWwindow*, int w, int h) {
        gWindowW = w;
        gWindowH = h;
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow*, double x, double y) {
        UpdateMousePosition(x, y);
    });
    double initialMouseX = 0.0;
    double initialMouseY = 0.0;
    glfwGetCursorPos(window, &initialMouseX, &initialMouseY);
    UpdateMousePosition(initialMouseX, initialMouseY);

    glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                EUINEO::State.mouseDown = true;
                EUINEO::State.mouseClicked = true;
            } else if (action == GLFW_RELEASE) {
                EUINEO::State.mouseDown = false;
            }
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                EUINEO::State.mouseRightDown = true;
                EUINEO::State.mouseRightClicked = true;
            } else if (action == GLFW_RELEASE) {
                EUINEO::State.mouseRightDown = false;
            }
        }
        EUINEO::Renderer::RequestRepaint();
    });

    glfwSetCharCallback(window, [](GLFWwindow*, unsigned int codepoint) {
        if (codepoint <= 0x7f) {
            EUINEO::State.textInput += (char)codepoint;
        } else if (codepoint <= 0x7ff) {
            EUINEO::State.textInput += (char)(0xc0 | ((codepoint >> 6) & 0x1f));
            EUINEO::State.textInput += (char)(0x80 | (codepoint & 0x3f));
        } else if (codepoint <= 0xffff) {
            EUINEO::State.textInput += (char)(0xe0 | ((codepoint >> 12) & 0x0f));
            EUINEO::State.textInput += (char)(0x80 | ((codepoint >> 6) & 0x3f));
            EUINEO::State.textInput += (char)(0x80 | (codepoint & 0x3f));
        } else if (codepoint <= 0x10ffff) {
            EUINEO::State.textInput += (char)(0xf0 | ((codepoint >> 18) & 0x07));
            EUINEO::State.textInput += (char)(0x80 | ((codepoint >> 12) & 0x3f));
            EUINEO::State.textInput += (char)(0x80 | ((codepoint >> 6) & 0x3f));
            EUINEO::State.textInput += (char)(0x80 | (codepoint & 0x3f));
        }
        EUINEO::Renderer::RequestRepaint();
    });

    glfwSetScrollCallback(window, [](GLFWwindow*, double xoffset, double yoffset) {
        EUINEO::State.scrollDeltaX += static_cast<float>(xoffset);
        EUINEO::State.scrollDeltaY += static_cast<float>(yoffset);
        EUINEO::Renderer::RequestRepaint();
    });

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key >= 0 && key < 512) {
            if (action == GLFW_PRESS) {
                EUINEO::State.keys[key] = true;
                EUINEO::State.keysPressed[key] = true;
                EUINEO::Renderer::RequestRepaint();
            } else if (action == GLFW_RELEASE) {
                EUINEO::State.keys[key] = false;
                EUINEO::Renderer::RequestRepaint();
            } else if (action == GLFW_REPEAT) {
                EUINEO::State.keysPressed[key] = true;
                EUINEO::Renderer::RequestRepaint();
            }
        }
    });

    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_MULTISAMPLE);

    EUINEO::Renderer::Init();

    constexpr const char* kUIFontFile = "YouSheBiaoTiHei-2.ttf";
    constexpr const char* kIconFontFile = "Font Awesome 7 Free-Solid-900.otf";
    constexpr float kUiSdfLoadSize = 72.0f;
    constexpr float kIconSdfLoadSize = 96.0f;
    constexpr float kCjkSdfLoadSize = 72.0f;

    const auto loadProjectFont = [](const char* fileName,
                                    float fontSize,
                                    unsigned int startChar,
                                    unsigned int endChar,
                                    bool useSdf = true) {
        static const char* kFontDirs[] = {
            "font/",
            "src/font/"
        };
        for (const char* dir : kFontDirs) {
            const std::string path = std::string(dir) + fileName;
            if (EUINEO::Renderer::LoadFont(path, fontSize, startChar, endChar, useSdf)) {
                return true;
            }
        }
        return false;
    };

    const auto loadProjectIcon = [&](unsigned int codepoint) {
        return loadProjectFont(kIconFontFile, kIconSdfLoadSize, codepoint, codepoint + 1, false);
    };

    bool fontLoaded = false;
    if (loadProjectFont(kUIFontFile, kUiSdfLoadSize, 32, 128)) {
        fontLoaded = true;
    }

    loadProjectIcon(0xF009); // grid
    loadProjectIcon(0xF013); // gear
    loadProjectIcon(0xF015); // home
    loadProjectIcon(0xF031); // font
    loadProjectIcon(0xF04B); // play
    loadProjectIcon(0xF106); // chevron-up
    loadProjectIcon(0xF107); // chevron-down
    loadProjectIcon(0xF185); // sun
    loadProjectIcon(0xF186); // moon

    if (!fontLoaded) {
        if (EUINEO::Renderer::LoadFont("C:/Windows/Fonts/msyh.ttc", kUiSdfLoadSize, 32, 128)) {
            fontLoaded = true;
        } else if (EUINEO::Renderer::LoadFont("C:/Windows/Fonts/arial.ttf", kUiSdfLoadSize)) {
            fontLoaded = true;
        } else {
            printf("Failed to load fallback font!\n");
        }
    }

    EUINEO::Renderer::RegisterFontSource("C:/Windows/Fonts/msyh.ttc", kCjkSdfLoadSize); // Deferred fallback for missing glyphs.

    EUINEO::MainPage mainPage{}; // Force recompilation when header-only pages change.
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }

        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - lastTime);
        EUINEO::State.deltaTime = dt > 0.05f ? 0.05f : dt;
        lastTime = currentTime;

        const bool frameRequestedBeforeUpdate =
            EUINEO::State.needsRepaint ||
            EUINEO::State.animationTimeLeft > 0.0f ||
            EUINEO::State.pointerMoved ||
            mainPage.WantsContinuousUpdate();
        if (frameRequestedBeforeUpdate) {
            mainPage.Update();
        }

        bool shouldDraw = EUINEO::Renderer::ShouldRepaint();
        if (shouldDraw) {
            EUINEO::State.frameCount++;
            EUINEO::Renderer::SetLayerBounds(EUINEO::RenderLayer::Backdrop, mainPage.LayerBounds(EUINEO::RenderLayer::Backdrop));
            if (EUINEO::Renderer::NeedsLayerRedraw(EUINEO::RenderLayer::Backdrop)) {
                EUINEO::Renderer::BeginLayer(EUINEO::RenderLayer::Backdrop);
                EUINEO::Renderer::BeginFrame();
                mainPage.DrawLayer(EUINEO::RenderLayer::Backdrop);
                EUINEO::Renderer::EndLayer();
            }
            EUINEO::Renderer::CompositeLayers(EUINEO::CurrentTheme->background);
            EUINEO::Renderer::BeginFrame();
            mainPage.Draw();
            glfwSwapBuffers(window);
        }

        EUINEO::State.textInput.clear();
        EUINEO::State.scrollDeltaX = 0.0f;
        EUINEO::State.scrollDeltaY = 0.0f;
        EUINEO::State.scrollConsumed = false;
        EUINEO::State.mouseClicked = false;
        EUINEO::State.mouseRightClicked = false;
        EUINEO::State.pointerMoved = false;
        memset(EUINEO::State.keysPressed, 0, sizeof(EUINEO::State.keysPressed));

        if (shouldDraw || EUINEO::State.animationTimeLeft > 0) {
            glfwPollEvents();
        } else if (mainPage.WantsContinuousUpdate()) {
            glfwWaitEventsTimeout(1.0 / 30.0);
        } else {
            glfwWaitEvents();
        }
    }

    EUINEO::Renderer::Shutdown();
    glfwTerminate();
    return 0;
}
