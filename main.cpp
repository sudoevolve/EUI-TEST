#include <algorithm>
#include <cmath>
#include <cstring>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "src/EUINEO.h"
#include "src/pages/MainPage.h"

bool lastLeftDown = false;
bool lastRightDown = false;

struct FrameCache {
    GLuint texture = 0;
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint textureLoc = -1;
    bool hasCache = false;
    int width = 0;
    int height = 0;
};

static FrameCache gFrameCache;

static GLuint CompileFrameCacheShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

static void InitFrameCacheRenderer() {
    static const char* vs = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aUV;
        out vec2 vUV;
        void main() {
            vUV = aUV;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    static const char* fs = R"(
        #version 330 core
        in vec2 vUV;
        uniform sampler2D uTexture;
        out vec4 FragColor;
        void main() {
            FragColor = texture(uTexture, vUV);
        }
    )";

    GLuint vertex = CompileFrameCacheShader(GL_VERTEX_SHADER, vs);
    GLuint fragment = CompileFrameCacheShader(GL_FRAGMENT_SHADER, fs);
    gFrameCache.program = glCreateProgram();
    glAttachShader(gFrameCache.program, vertex);
    glAttachShader(gFrameCache.program, fragment);
    glLinkProgram(gFrameCache.program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    gFrameCache.textureLoc = glGetUniformLocation(gFrameCache.program, "uTexture");

    const float quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &gFrameCache.vao);
    glGenBuffers(1, &gFrameCache.vbo);
    glBindVertexArray(gFrameCache.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gFrameCache.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static void ShutdownFrameCacheRenderer() {
    if (gFrameCache.texture) glDeleteTextures(1, &gFrameCache.texture);
    if (gFrameCache.program) glDeleteProgram(gFrameCache.program);
    if (gFrameCache.vao) glDeleteVertexArrays(1, &gFrameCache.vao);
    if (gFrameCache.vbo) glDeleteBuffers(1, &gFrameCache.vbo);
}

static void EnsureFrameCacheTexture(int width, int height) {
    if (gFrameCache.texture == 0) {
        glGenTextures(1, &gFrameCache.texture);
    }

    if (gFrameCache.width == width && gFrameCache.height == height) {
        return;
    }

    gFrameCache.width = width;
    gFrameCache.height = height;
    gFrameCache.hasCache = false;

    glBindTexture(GL_TEXTURE_2D, gFrameCache.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

static void CopyFullToFrameCache(int width, int height) {
    EnsureFrameCacheTexture(width, height);
    glBindTexture(GL_TEXTURE_2D, gFrameCache.texture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
    gFrameCache.hasCache = true;
}

static void CopyRegionToFrameCache(int x, int y, int width, int height) {
    if (!gFrameCache.texture || width <= 0 || height <= 0) {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, gFrameCache.texture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x, y, x, y, width, height);
    gFrameCache.hasCache = true;
}

static void DrawFrameCache(int width, int height) {
    if (!gFrameCache.hasCache || !gFrameCache.texture) {
        return;
    }
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_SCISSOR_TEST);
    glUseProgram(gFrameCache.program);
    glUniform1i(gFrameCache.textureLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gFrameCache.texture);
    glBindVertexArray(gFrameCache.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

struct GLDirtyRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

static GLDirtyRect ToGLDirtyRect(float x1, float y1, float x2, float y2, int framebufferW, int framebufferH) {
    int x = std::clamp((int)std::floor(x1), 0, framebufferW);
    int yTop = std::clamp((int)std::floor(y1), 0, framebufferH);
    int w = std::clamp((int)std::ceil(x2 - x1), 0, framebufferW - x);
    int h = std::clamp((int)std::ceil(y2 - y1), 0, framebufferH - yTop);
    int y = std::clamp(framebufferH - (yTop + h), 0, framebufferH);
    return GLDirtyRect{x, y, w, h};
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
        EUINEO::State.screenW = (float)w;
        EUINEO::State.screenH = (float)h;
        gFrameCache.hasCache = false;
        EUINEO::Renderer::InvalidateBackdrop();
        EUINEO::Renderer::InvalidateAll();
    });

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    InitFrameCacheRenderer();

    glfwSetCursorPosCallback(window, [](GLFWwindow*, double x, double y) {
        EUINEO::State.mouseX = (float)x;
        EUINEO::State.mouseY = (float)y;
    });

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

    bool fontLoaded = false;
    if (EUINEO::Renderer::LoadFont("font/Medicinal Compound.otf", 24.0f, 32, 128) ||
        EUINEO::Renderer::LoadFont("../src/font/Medicinal Compound.otf", 24.0f, 32, 128)) {
        fontLoaded = true;
    }

    if (!EUINEO::Renderer::LoadFont("font/Font Awesome 7 Free-Solid-900.otf", 24.0f, 0xF000, 0xF2FF)) {
        EUINEO::Renderer::LoadFont("../src/font/Font Awesome 7 Free-Solid-900.otf", 24.0f, 0xF000, 0xF2FF);
    }

    if (!fontLoaded) {
        if (EUINEO::Renderer::LoadFont("C:/Windows/Fonts/msyh.ttc", 24.0f, 32, 128)) {
            EUINEO::Renderer::LoadFont("C:/Windows/Fonts/msyh.ttc", 24.0f, 0x4E00, 0x9FA5);
        } else if (!EUINEO::Renderer::LoadFont("C:/Windows/Fonts/arial.ttf", 24.0f)) {
            printf("Failed to load fallback font!\n");
        }
    }

    EUINEO::MainPage mainPage;
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }

        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - lastTime);
        EUINEO::State.deltaTime = dt > 0.05f ? 0.05f : dt;
        lastTime = currentTime;

        int w = 0;
        int h = 0;
        glfwGetWindowSize(window, &w, &h);
        EUINEO::State.screenW = (float)w;
        EUINEO::State.screenH = (float)h;

        double mx = 0.0;
        double my = 0.0;
        glfwGetCursorPos(window, &mx, &my);
        EUINEO::State.mouseX = (float)mx;
        EUINEO::State.mouseY = (float)my;

        bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        EUINEO::State.mouseDown = leftDown;
        EUINEO::State.mouseRightDown = rightDown;
        EUINEO::State.mouseClicked = (leftDown && !lastLeftDown);
        EUINEO::State.mouseRightClicked = (rightDown && !lastRightDown);

        lastLeftDown = leftDown;
        lastRightDown = rightDown;

        mainPage.Update();

        bool shouldDraw = EUINEO::Renderer::ShouldRepaint();
        if (shouldDraw) {
            EUINEO::Color bg = EUINEO::CurrentTheme->background;
            bool hasDirtyRect = EUINEO::State.dirtyX2 > EUINEO::State.dirtyX1 && EUINEO::State.dirtyY2 > EUINEO::State.dirtyY1;
            bool canSkipRender = !EUINEO::State.fullScreenDirty && gFrameCache.hasCache && !hasDirtyRect;

            if (canSkipRender) {
                EUINEO::Renderer::SetFullRedraw();
                EUINEO::Renderer::ClearDirtyRect();
            } else {
                EUINEO::State.frameCount++;
                bool useFullRedraw = EUINEO::State.fullScreenDirty || !gFrameCache.hasCache || !hasDirtyRect;

                if (useFullRedraw) {
                    EUINEO::Renderer::SetFullRedraw();
                    glDisable(GL_SCISSOR_TEST);
                    glClearColor(bg.r, bg.g, bg.b, bg.a);
                    glClear(GL_COLOR_BUFFER_BIT);
                    EUINEO::Renderer::BeginFrame();
                    mainPage.Draw();
                    CopyFullToFrameCache(w, h);
                } else {
                    GLDirtyRect dirty = ToGLDirtyRect(EUINEO::State.dirtyX1, EUINEO::State.dirtyY1, EUINEO::State.dirtyX2, EUINEO::State.dirtyY2, w, h);
                    if (dirty.w <= 0 || dirty.h <= 0) {
                        EUINEO::Renderer::SetFullRedraw();
                        glDisable(GL_SCISSOR_TEST);
                        glClearColor(bg.r, bg.g, bg.b, bg.a);
                        glClear(GL_COLOR_BUFFER_BIT);
                        EUINEO::Renderer::BeginFrame();
                        mainPage.Draw();
                        CopyFullToFrameCache(w, h);
                    } else {
                        EUINEO::Renderer::SetPartialRedraw(EUINEO::State.dirtyX1, EUINEO::State.dirtyY1,
                                                           EUINEO::State.dirtyX2, EUINEO::State.dirtyY2);
                        DrawFrameCache(w, h);
                        glEnable(GL_SCISSOR_TEST);
                        glScissor(dirty.x, dirty.y, dirty.w, dirty.h);
                        glClearColor(bg.r, bg.g, bg.b, bg.a);
                        glClear(GL_COLOR_BUFFER_BIT);
                        EUINEO::Renderer::BeginFrame();
                        mainPage.Draw();
                        glDisable(GL_SCISSOR_TEST);
                        CopyRegionToFrameCache(dirty.x, dirty.y, dirty.w, dirty.h);
                    }
                }

                glfwSwapBuffers(window);
                EUINEO::Renderer::ClearDirtyRect();
            }
        }

        EUINEO::State.textInput.clear();
        memset(EUINEO::State.keysPressed, 0, sizeof(EUINEO::State.keysPressed));

        if (shouldDraw || EUINEO::State.animationTimeLeft > 0) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }
    }

    ShutdownFrameCacheRenderer();
    EUINEO::Renderer::Shutdown();
    glfwTerminate();
    return 0;
}
