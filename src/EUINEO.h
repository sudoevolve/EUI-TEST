#pragma once

#include <functional>
#include <glad/glad.h>
#include <string>
#include <vector>

namespace EUINEO {

struct Color {
    float r, g, b, a;
    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
};

Color Lerp(const Color& a, const Color& b, float t);
float Lerp(float a, float b, float t);

struct Theme {
    Color background;
    Color primary;
    Color surface;
    Color surfaceHover;
    Color surfaceActive;
    Color text;
    Color border;
};

extern Theme LightTheme;
extern Theme DarkTheme;
extern Theme* CurrentTheme;

struct UIState {
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool mouseDown = false;
    bool mouseClicked = false;
    bool mouseRightDown = false;
    bool mouseRightClicked = false;
    float deltaTime = 0.0f;
    float screenW = 800.0f;
    float screenH = 600.0f;

    std::string textInput;
    bool keys[512] = {false};
    bool keysPressed[512] = {false};

    bool needsRepaint = true;
    float animationTimeLeft = 0.0f;
    int frameCount = 0;

    bool fullScreenDirty = true;
    float dirtyX1 = 0.0f;
    float dirtyY1 = 0.0f;
    float dirtyX2 = 0.0f;
    float dirtyY2 = 0.0f;
};

extern UIState State;

class Renderer {
public:
    static void Init();
    static void Shutdown();
    static void BeginFrame();
    static void SetFullRedraw();
    static void SetPartialRedraw(float x1, float y1, float x2, float y2);

    static void ApplyScissor();
    static void ClearDirtyRect();
    static void AddDirtyRect(float x, float y, float w, float h);
    static void InvalidateAll();
    static void InvalidateBackdrop();

    static void DrawRect(float x, float y, float w, float h, const Color& color, float rounding = 0.0f,
                         float blurAmount = 0.0f, float shadowBlur = 0.0f,
                         float shadowOffsetX = 0.0f, float shadowOffsetY = 0.0f,
                         const Color& shadowColor = Color(0, 0, 0, 0));

    static bool LoadFont(const std::string& fontPath, float fontSize = 24.0f,
                         unsigned int startChar = 32, unsigned int endChar = 128);
    static void DrawTextStr(const std::string& text, float x, float y, const Color& color, float scale = 1.0f);
    static float MeasureTextWidth(const std::string& text, float scale = 1.0f);
    static void RequestRepaint(float duration = 0.0f);
    static bool ShouldRepaint();
};

enum class Anchor {
    TopLeft, TopCenter, TopRight,
    CenterLeft, Center, CenterRight,
    BottomLeft, BottomCenter, BottomRight
};

class Widget {
public:
    float x = 0.0f;
    float y = 0.0f;
    float width = 100.0f;
    float height = 30.0f;
    Anchor anchor = Anchor::TopLeft;
    float fontSize = 24.0f;

    virtual ~Widget() = default;
    virtual void Update() {}
    virtual void Draw() {}

    void GetAbsoluteBounds(float& outX, float& outY);
    bool IsHovered();
    void MarkDirty(float expand = 20.0f, float duration = 0.0f);
};

} // namespace EUINEO
