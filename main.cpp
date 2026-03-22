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
    GLint screenSizeLoc = -1;
    GLint posLoc = -1;
    GLint sizeLoc = -1;
    GLint uvPosLoc = -1;
    GLint uvSizeLoc = -1;
    bool hasCache = false;
    int width = 0;
    int height = 0;
};

static FrameCache gFrameCache;
static constexpr bool kReuseSwapBackBufferForPartialRedraw = false;
static constexpr bool kEnablePartialRedraw = false;

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
        uniform vec2 uScreenSize;
        uniform vec2 uPos;
        uniform vec2 uSize;
        uniform vec2 uUVPos;
        uniform vec2 uUVSize;
        void main() {
            vec2 pixelPos = uPos + aPos * uSize;
            vec2 ndc = vec2(
                (pixelPos.x / uScreenSize.x) * 2.0 - 1.0,
                (pixelPos.y / uScreenSize.y) * 2.0 - 1.0
            );
            vUV = uUVPos + aUV * uUVSize;
            gl_Position = vec4(ndc, 0.0, 1.0);
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
    gFrameCache.screenSizeLoc = glGetUniformLocation(gFrameCache.program, "uScreenSize");
    gFrameCache.posLoc = glGetUniformLocation(gFrameCache.program, "uPos");
    gFrameCache.sizeLoc = glGetUniformLocation(gFrameCache.program, "uSize");
    gFrameCache.uvPosLoc = glGetUniformLocation(gFrameCache.program, "uUVPos");
    gFrameCache.uvSizeLoc = glGetUniformLocation(gFrameCache.program, "uUVSize");

    const float quad[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
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

struct GLDirtyRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    float redrawX1 = 0.0f;
    float redrawY1 = 0.0f;
    float redrawX2 = 0.0f;
    float redrawY2 = 0.0f;
};

static bool BeginFrameCacheDraw(int width, int height) {
    if (!gFrameCache.hasCache || !gFrameCache.texture) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return false;
    }

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_SCISSOR_TEST);
    glUseProgram(gFrameCache.program);
    glUniform1i(gFrameCache.textureLoc, 0);
    glUniform2f(gFrameCache.screenSizeLoc, (float)width, (float)height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gFrameCache.texture);
    glBindVertexArray(gFrameCache.vao);
    return true;
}

static void DrawFrameCacheRegion(int width, int height, int x, int y, int regionW, int regionH) {
    if (regionW <= 0 || regionH <= 0 || width <= 0 || height <= 0) {
        return;
    }

    float uvX = (float)x / (float)width;
    float uvY = (float)y / (float)height;
    float uvW = (float)regionW / (float)width;
    float uvH = (float)regionH / (float)height;

    glUniform2f(gFrameCache.posLoc, (float)x, (float)y);
    glUniform2f(gFrameCache.sizeLoc, (float)regionW, (float)regionH);
    glUniform2f(gFrameCache.uvPosLoc, uvX, uvY);
    glUniform2f(gFrameCache.uvSizeLoc, uvW, uvH);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void DrawFrameCacheOutsideDirty(int width, int height, const GLDirtyRect& dirty) {
    if (!BeginFrameCacheDraw(width, height)) {
        return;
    }

    DrawFrameCacheRegion(width, height, 0, 0, width, dirty.y);

    int topY = dirty.y + dirty.h;
    DrawFrameCacheRegion(width, height, 0, topY, width, height - topY);

    DrawFrameCacheRegion(width, height, 0, dirty.y, dirty.x, dirty.h);

    int rightX = dirty.x + dirty.w;
    DrawFrameCacheRegion(width, height, rightX, dirty.y, width - rightX, dirty.h);
}

static GLDirtyRect ToGLDirtyRect(float x1, float y1, float x2, float y2, int framebufferW, int framebufferH) {
    int xLeft = std::clamp((int)std::floor(x1) - 1, 0, framebufferW);
    int yTop = std::clamp((int)std::floor(y1) - 1, 0, framebufferH);
    int xRight = std::clamp((int)std::ceil(x2) + 1, xLeft, framebufferW);
    int yBottom = std::clamp((int)std::ceil(y2) + 1, yTop, framebufferH);
    int w = xRight - xLeft;
    int h = yBottom - yTop;
    int y = std::clamp(framebufferH - yBottom, 0, framebufferH);
    return GLDirtyRect{
        xLeft,
        y,
        w,
        h,
        (float)xLeft,
        (float)yTop,
        (float)xRight,
        (float)yBottom
    };
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
    int initialFbW = 0;
    int initialFbH = 0;
    glfwGetFramebufferSize(window, &initialFbW, &initialFbH);
    glViewport(0, 0, initialFbW, initialFbH);
    EUINEO::State.screenW = (float)initialFbW;
    EUINEO::State.screenH = (float)initialFbH;

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
    if (EUINEO::Renderer::LoadFont("src/font/Mountain and Nature.ttf", 24.0f, 32, 128) ||
        EUINEO::Renderer::LoadFont("../src/font/Mountain and Nature.ttf", 24.0f, 32, 128) ||
        EUINEO::Renderer::LoadFont("../../src/font/Mountain and Nature.ttf", 24.0f, 32, 128)) {
        fontLoaded = true;
    }

    if (!EUINEO::Renderer::LoadFont("font/Font Awesome 7 Free-Solid-900.otf", 24.0f, 0xF000, 0xF2FF)) {
        EUINEO::Renderer::LoadFont("../src/font/Font Awesome 7 Free-Solid-900.otf", 24.0f, 0xF000, 0xF2FF);
    }

    if (!fontLoaded) {
        if (EUINEO::Renderer::LoadFont("C:/Windows/Fonts/msyh.ttc", 24.0f, 32, 128)) {
            fontLoaded = true;
        } else if (EUINEO::Renderer::LoadFont("C:/Windows/Fonts/arial.ttf", 24.0f)) {
            fontLoaded = true;
        } else {
            printf("Failed to load fallback font!\n");
        }
    }

    EUINEO::Renderer::LoadFont("C:/Windows/Fonts/msyh.ttc", 24.0f, 0x4E00, 0xA000);

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

        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        EUINEO::State.screenW = (float)w;
        EUINEO::State.screenH = (float)h;

        double mx = 0.0;
        double my = 0.0;
        glfwGetCursorPos(window, &mx, &my);
        int winW = 0;
        int winH = 0;
        glfwGetWindowSize(window, &winW, &winH);
        float mouseScaleX = (winW > 0) ? ((float)w / (float)winW) : 1.0f;
        float mouseScaleY = (winH > 0) ? ((float)h / (float)winH) : 1.0f;
        EUINEO::State.mouseX = (float)mx * mouseScaleX;
        EUINEO::State.mouseY = (float)my * mouseScaleY;

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
            bool animationFrame = EUINEO::State.animationTimeLeft > 0.0f;
            bool canSkipRender = !EUINEO::State.fullScreenDirty && gFrameCache.hasCache && !hasDirtyRect && !animationFrame;

            if (canSkipRender) {
                EUINEO::Renderer::SetFullRedraw();
                EUINEO::Renderer::ClearDirtyRect();
            } else {
                EUINEO::State.frameCount++;
                bool useFullRedraw = !kEnablePartialRedraw ||
                                     EUINEO::State.fullScreenDirty ||
                                     !gFrameCache.hasCache ||
                                     !hasDirtyRect;

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
                        EUINEO::Renderer::SetPartialRedraw(dirty.redrawX1, dirty.redrawY1,
                                                           dirty.redrawX2, dirty.redrawY2);
                        if (!kReuseSwapBackBufferForPartialRedraw) {
                            DrawFrameCacheOutsideDirty(w, h, dirty);
                        }
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
