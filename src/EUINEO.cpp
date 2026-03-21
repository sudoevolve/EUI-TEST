#include "EUINEO.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace EUINEO {

Color Lerp(const Color& a, const Color& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Color(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    );
}

float Lerp(float a, float b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

Theme LightTheme = {
    Color(0.95f, 0.95f, 0.97f),
    Color(0.2f, 0.5f, 0.9f),
    Color(1.0f, 1.0f, 1.0f),
    Color(0.9f, 0.9f, 0.9f),
    Color(0.8f, 0.8f, 0.8f),
    Color(0.0f, 0.0f, 0.0f),
    Color(0.8f, 0.8f, 0.8f)
};

Theme DarkTheme = {
    Color(0.1f, 0.1f, 0.12f),
    Color(0.3f, 0.6f, 1.0f),
    Color(0.15f, 0.15f, 0.18f),
    Color(0.25f, 0.25f, 0.28f),
    Color(0.35f, 0.35f, 0.38f),
    Color(1.0f, 1.0f, 1.0f),
    Color(0.3f, 0.3f, 0.3f)
};

Theme* CurrentTheme = &DarkTheme;
UIState State;

static GLuint VAO = 0;
static GLuint VBO = 0;
static GLuint ShaderProgram = 0;
static GLint ProjLoc = -1;
static GLint ColorLoc = -1;
static GLint PosLoc = -1;
static GLint SizeLoc = -1;
static GLint RoundingLoc = -1;
static GLint BoxPosLoc = -1;
static GLint BoxSizeLoc = -1;
static GLint BlurAmountLoc = -1;
static GLint ShadowBlurLoc = -1;
static GLint ShadowOffsetLoc = -1;
static GLint ShadowColorLoc = -1;
static GLint TimeLoc = -1;
static GLint ResolutionLoc = -1;
static GLint Channel0Loc = -1;
static GLuint BgTexture = 0;
static GLuint CachedBlurProgram = 0;
static GLint CachedBlurProjLoc = -1;
static GLint CachedBlurPosLoc = -1;
static GLint CachedBlurSizeLoc = -1;
static GLint CachedBlurTextureLoc = -1;
static GLuint CachedBlurTexture = 0;
static int CachedBlurTextureW = 0;
static int CachedBlurTextureH = 0;
static bool BlurCacheValid = false;
static float CachedBlurX = 0.0f;
static float CachedBlurY = 0.0f;
static float CachedBlurW = 0.0f;
static float CachedBlurH = 0.0f;
static float CachedBlurColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static float CachedBlurShadowColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static float CachedBlurRounding = 0.0f;
static float CachedBlurAmount = 0.0f;
static float CachedBlurShadowBlur = 0.0f;
static float CachedBlurShadowOffsetX = 0.0f;
static float CachedBlurShadowOffsetY = 0.0f;
static unsigned int BackdropVersion = 1;
static unsigned int CachedBackdropVersion = 0;
static bool ActiveRedrawClipEnabled = false;
static float ActiveRedrawX1 = 0.0f;
static float ActiveRedrawY1 = 0.0f;
static float ActiveRedrawX2 = 0.0f;
static float ActiveRedrawY2 = 0.0f;

static const char* vShaderStr = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vPos;
uniform mat4 projection;
uniform vec2 uPos;
uniform vec2 uSize;
void main() {
    vec2 pos = (aPos * uSize) + uPos;
    vPos = pos;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
)";

static const char* fShaderStr = R"(
#version 330 core
in vec2 vPos;
uniform vec4 uColor;
uniform vec2 uPos;
uniform vec2 uSize;
uniform vec2 uBoxPos;
uniform vec2 uBoxSize;
uniform float uRounding;
uniform float uBlurAmount;
uniform float uShadowBlur;
uniform vec2 uShadowOffset;
uniform vec4 uShadowColor;
uniform float iTime;
uniform vec2 iResolution;
uniform sampler2D iChannel0;
out vec4 FragColor;

float roundedBoxSDF(vec2 centerPosition, vec2 size, float radius) {
    return length(max(abs(centerPosition) - size + radius, 0.0)) - radius;
}

vec3 draw(vec2 uv) {
    return texture(iChannel0, uv).rgb;
}

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 center = uBoxPos + uBoxSize * 0.5;
    vec2 p = vPos - center;
    float d = roundedBoxSDF(p, uBoxSize * 0.5, uRounding);

    float shadowAlpha = 0.0;
    if (uShadowBlur > 0.0) {
        vec2 shadowCenter = center + uShadowOffset;
        vec2 sp = vPos - shadowCenter;
        float sd = roundedBoxSDF(sp, uBoxSize * 0.5, uRounding);
        shadowAlpha = 1.0 - smoothstep(-uShadowBlur, uShadowBlur, sd);
        shadowAlpha *= uShadowColor.a;
    }

    float alpha = 1.0 - smoothstep(-1.0, 1.0, d);
    vec4 finalColor = vec4(0.0);

    if (uBlurAmount > 0.0 && alpha > 0.0) {
        vec2 uv = gl_FragCoord.xy / iResolution.xy;
        float bluramount = uBlurAmount;
        vec3 blurredImage = vec3(0.0);
        const float repeats = 60.0;
        for (float i = 0.0; i < repeats; i += 1.0) {
            vec2 q = vec2(cos(degrees((i / repeats) * 360.0)), sin(degrees((i / repeats) * 360.0))) *
                     (rand(vec2(i, uv.x + uv.y)) + bluramount);
            vec2 uv2 = uv + (q * bluramount);
            blurredImage += draw(uv2) / 2.0;
            q = vec2(cos(degrees((i / repeats) * 360.0)), sin(degrees((i / repeats) * 360.0))) *
                (rand(vec2(i + 2.0, uv.x + uv.y + 24.0)) + bluramount);
            uv2 = uv + (q * bluramount);
            blurredImage += draw(uv2) / 2.0;
        }
        blurredImage /= repeats;
        vec3 mixColor = mix(blurredImage, uColor.rgb, uColor.a);
        finalColor = vec4(mixColor, alpha);
    } else {
        finalColor = vec4(uColor.rgb, uColor.a * alpha);
    }

    if (shadowAlpha > 0.0 && alpha < 1.0) {
        vec3 outRgb = (finalColor.rgb * finalColor.a + uShadowColor.rgb * shadowAlpha * (1.0 - finalColor.a)) /
                      max(0.001, (finalColor.a + shadowAlpha * (1.0 - finalColor.a)));
        float outA = finalColor.a + shadowAlpha * (1.0 - finalColor.a);
        FragColor = vec4(outRgb, outA);
    } else {
        FragColor = finalColor;
    }
}
)";

static const char* cachedBlurVShaderStr = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vUV;
uniform mat4 projection;
uniform vec2 uPos;
uniform vec2 uSize;
void main() {
    vec2 pos = (aPos * uSize) + uPos;
    vUV = vec2(aPos.x, 1.0 - aPos.y);
    gl_Position = projection * vec4(pos, 0.0, 1.0);
}
)";

static const char* cachedBlurFShaderStr = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTexture;
out vec4 FragColor;
void main() {
    FragColor = texture(uTexture, vUV);
}
)";

static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

static GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

static bool FloatEq(float a, float b, float epsilon = 0.0001f) {
    return std::abs(a - b) <= epsilon;
}

static bool RectIntersectsActiveRedraw(float x, float y, float w, float h) {
    if (!ActiveRedrawClipEnabled) {
        return true;
    }
    return w > 0.0f && h > 0.0f &&
        x < ActiveRedrawX2 &&
        x + w > ActiveRedrawX1 &&
        y < ActiveRedrawY2 &&
        y + h > ActiveRedrawY1;
}

static bool RectFullyCoveredByActiveRedraw(float x, float y, float w, float h) {
    if (!ActiveRedrawClipEnabled) {
        return true;
    }
    return w > 0.0f && h > 0.0f &&
        x >= ActiveRedrawX1 &&
        y >= ActiveRedrawY1 &&
        x + w <= ActiveRedrawX2 &&
        y + h <= ActiveRedrawY2;
}

static void EnsureCachedBlurTexture(int width, int height) {
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (CachedBlurTexture == 0) {
        glGenTextures(1, &CachedBlurTexture);
    }
    glBindTexture(GL_TEXTURE_2D, CachedBlurTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (CachedBlurTextureW != width || CachedBlurTextureH != height) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        CachedBlurTextureW = width;
        CachedBlurTextureH = height;
        BlurCacheValid = false;
    }
}

static bool CachedBlurMatches(float quadX, float quadY, float quadW, float quadH, const Color& color, float rounding,
                              float blurAmount, float shadowBlur, float shadowOffsetX, float shadowOffsetY,
                              const Color& shadowColor) {
    return BlurCacheValid &&
        CachedBackdropVersion == BackdropVersion &&
        FloatEq(CachedBlurX, quadX) &&
        FloatEq(CachedBlurY, quadY) &&
        FloatEq(CachedBlurW, quadW) &&
        FloatEq(CachedBlurH, quadH) &&
        FloatEq(CachedBlurColor[0], color.r) &&
        FloatEq(CachedBlurColor[1], color.g) &&
        FloatEq(CachedBlurColor[2], color.b) &&
        FloatEq(CachedBlurColor[3], color.a) &&
        FloatEq(CachedBlurRounding, rounding) &&
        FloatEq(CachedBlurAmount, blurAmount) &&
        FloatEq(CachedBlurShadowBlur, shadowBlur) &&
        FloatEq(CachedBlurShadowOffsetX, shadowOffsetX) &&
        FloatEq(CachedBlurShadowOffsetY, shadowOffsetY) &&
        FloatEq(CachedBlurShadowColor[0], shadowColor.r) &&
        FloatEq(CachedBlurShadowColor[1], shadowColor.g) &&
        FloatEq(CachedBlurShadowColor[2], shadowColor.b) &&
        FloatEq(CachedBlurShadowColor[3], shadowColor.a);
}

struct Character {
    GLuint TextureID;
    int Size[2];
    int Bearing[2];
    unsigned int Advance;
};

static std::unordered_map<unsigned int, Character> Characters;
static GLuint TextVAO = 0;
static GLuint TextVBO = 0;
static GLuint TextShaderProgram = 0;
static GLint TextProjLoc = -1;
static GLint TextColorLoc = -1;

static const char* textVShaderStr = R"(
#version 330 core
layout(location = 0) in vec4 vertex;
out vec2 TexCoords;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

static const char* textFShaderStr = R"(
#version 330 core
in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;
uniform vec4 textColor;
void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = textColor * sampled;
}
)";

void Renderer::Init() {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vShaderStr);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fShaderStr);
    ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, vs);
    glAttachShader(ShaderProgram, fs);
    glLinkProgram(ShaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    CachedBlurProgram = CreateProgram(cachedBlurVShaderStr, cachedBlurFShaderStr);

    ProjLoc = glGetUniformLocation(ShaderProgram, "projection");
    ColorLoc = glGetUniformLocation(ShaderProgram, "uColor");
    PosLoc = glGetUniformLocation(ShaderProgram, "uPos");
    SizeLoc = glGetUniformLocation(ShaderProgram, "uSize");
    BoxPosLoc = glGetUniformLocation(ShaderProgram, "uBoxPos");
    BoxSizeLoc = glGetUniformLocation(ShaderProgram, "uBoxSize");
    RoundingLoc = glGetUniformLocation(ShaderProgram, "uRounding");
    BlurAmountLoc = glGetUniformLocation(ShaderProgram, "uBlurAmount");
    ShadowBlurLoc = glGetUniformLocation(ShaderProgram, "uShadowBlur");
    ShadowOffsetLoc = glGetUniformLocation(ShaderProgram, "uShadowOffset");
    ShadowColorLoc = glGetUniformLocation(ShaderProgram, "uShadowColor");
    TimeLoc = glGetUniformLocation(ShaderProgram, "iTime");
    ResolutionLoc = glGetUniformLocation(ShaderProgram, "iResolution");
    Channel0Loc = glGetUniformLocation(ShaderProgram, "iChannel0");
    CachedBlurProjLoc = glGetUniformLocation(CachedBlurProgram, "projection");
    CachedBlurPosLoc = glGetUniformLocation(CachedBlurProgram, "uPos");
    CachedBlurSizeLoc = glGetUniformLocation(CachedBlurProgram, "uSize");
    CachedBlurTextureLoc = glGetUniformLocation(CachedBlurProgram, "uTexture");

    glGenTextures(1, &BgTexture);
    glBindTexture(GL_TEXTURE_2D, BgTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint tvs = CompileShader(GL_VERTEX_SHADER, textVShaderStr);
    GLuint tfs = CompileShader(GL_FRAGMENT_SHADER, textFShaderStr);
    TextShaderProgram = glCreateProgram();
    glAttachShader(TextShaderProgram, tvs);
    glAttachShader(TextShaderProgram, tfs);
    glLinkProgram(TextShaderProgram);
    glDeleteShader(tvs);
    glDeleteShader(tfs);

    TextProjLoc = glGetUniformLocation(TextShaderProgram, "projection");
    TextColorLoc = glGetUniformLocation(TextShaderProgram, "textColor");

    glGenVertexArrays(1, &TextVAO);
    glGenBuffers(1, &TextVBO);
    glBindVertexArray(TextVAO);
    glBindBuffer(GL_ARRAY_BUFFER, TextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::Shutdown() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(ShaderProgram);
    glDeleteProgram(CachedBlurProgram);
    glDeleteTextures(1, &BgTexture);
    if (CachedBlurTexture) glDeleteTextures(1, &CachedBlurTexture);

    glDeleteVertexArrays(1, &TextVAO);
    glDeleteBuffers(1, &TextVBO);
    glDeleteProgram(TextShaderProgram);
}

static GLuint CurrentActiveProgram = 0;

void Renderer::BeginFrame() {
    float L = 0.0f;
    float R = State.screenW;
    float B = State.screenH;
    float T = 0.0f;
    float proj[16] = {
        2.0f / (R - L), 0, 0, 0,
        0, 2.0f / (T - B), 0, 0,
        0, 0, -1, 0,
        -(R + L) / (R - L), -(T + B) / (T - B), 0, 1
    };

    glUseProgram(ShaderProgram);
    glUniformMatrix4fv(ProjLoc, 1, GL_FALSE, proj);

    glUseProgram(CachedBlurProgram);
    glUniformMatrix4fv(CachedBlurProjLoc, 1, GL_FALSE, proj);

    glUseProgram(TextShaderProgram);
    glUniformMatrix4fv(TextProjLoc, 1, GL_FALSE, proj);

    CurrentActiveProgram = TextShaderProgram;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void Renderer::SetFullRedraw() {
    ActiveRedrawClipEnabled = false;
    ActiveRedrawX1 = ActiveRedrawY1 = ActiveRedrawX2 = ActiveRedrawY2 = 0.0f;
}

void Renderer::SetPartialRedraw(float x1, float y1, float x2, float y2) {
    if (x2 <= x1 || y2 <= y1) {
        SetFullRedraw();
        return;
    }
    ActiveRedrawClipEnabled = true;
    ActiveRedrawX1 = x1;
    ActiveRedrawY1 = y1;
    ActiveRedrawX2 = x2;
    ActiveRedrawY2 = y2;
}

void Renderer::DrawRect(float x, float y, float w, float h, const Color& color, float rounding,
                        float blurAmount, float shadowBlur, float shadowOffsetX, float shadowOffsetY,
                        const Color& shadowColor) {
    float expand = shadowBlur * 2.0f;
    float quadX = x - expand + std::min(0.0f, shadowOffsetX);
    float quadY = y - expand + std::min(0.0f, shadowOffsetY);
    float quadW = w + expand * 2.0f + std::abs(shadowOffsetX);
    float quadH = h + expand * 2.0f + std::abs(shadowOffsetY);

    if (!RectIntersectsActiveRedraw(quadX, quadY, quadW, quadH)) {
        return;
    }

    if (blurAmount > 0.0f && CachedBlurMatches(quadX, quadY, quadW, quadH, color, rounding, blurAmount,
                                               shadowBlur, shadowOffsetX, shadowOffsetY, shadowColor)) {
        if (CurrentActiveProgram != CachedBlurProgram) {
            glUseProgram(CachedBlurProgram);
            CurrentActiveProgram = CachedBlurProgram;
        }
        glUniform2f(CachedBlurPosLoc, CachedBlurX, CachedBlurY);
        glUniform2f(CachedBlurSizeLoc, CachedBlurW, CachedBlurH);
        glUniform1i(CachedBlurTextureLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, CachedBlurTexture);
        glDisable(GL_BLEND);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return;
    }

    if (CurrentActiveProgram != ShaderProgram) {
        glUseProgram(ShaderProgram);
        CurrentActiveProgram = ShaderProgram;
    }

    glUniform2f(PosLoc, quadX, quadY);
    glUniform2f(SizeLoc, quadW, quadH);
    glUniform2f(BoxPosLoc, x, y);
    glUniform2f(BoxSizeLoc, w, h);
    glUniform4f(ColorLoc, color.r, color.g, color.b, color.a);
    glUniform1f(RoundingLoc, rounding);
    glUniform1f(BlurAmountLoc, blurAmount);
    glUniform1f(ShadowBlurLoc, shadowBlur);
    glUniform2f(ShadowOffsetLoc, shadowOffsetX, shadowOffsetY);
    glUniform4f(ShadowColorLoc, shadowColor.r, shadowColor.g, shadowColor.b, shadowColor.a);
    glUniform1f(TimeLoc, (float)glfwGetTime());
    glUniform2f(ResolutionLoc, State.screenW, State.screenH);

    static int lastBlurFrame = -1;
    if (blurAmount > 0.0f) {
        glBindTexture(GL_TEXTURE_2D, BgTexture);
        static int texW = 0;
        static int texH = 0;
        if (texW != (int)State.screenW || texH != (int)State.screenH) {
            texW = (int)State.screenW;
            texH = (int)State.screenH;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            lastBlurFrame = -1;
        }
        if (lastBlurFrame != State.frameCount) {
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texW, texH);
            lastBlurFrame = State.frameCount;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, BgTexture);
        glUniform1i(Channel0Loc, 0);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (blurAmount > 0.0f) {
        int copyX = std::clamp((int)std::floor(quadX), 0, (int)State.screenW);
        int copyYTop = std::clamp((int)std::floor(quadY), 0, (int)State.screenH);
        int copyRight = std::clamp((int)std::ceil(quadX + quadW), 0, (int)State.screenW);
        int copyBottom = std::clamp((int)std::ceil(quadY + quadH), 0, (int)State.screenH);
        int copyW = copyRight - copyX;
        int copyH = copyBottom - copyYTop;

        if (copyW > 0 && copyH > 0 &&
            RectFullyCoveredByActiveRedraw((float)copyX, (float)copyYTop, (float)copyW, (float)copyH)) {
            int copyY = std::max(0, (int)std::floor(State.screenH - (copyYTop + copyH)));

            EnsureCachedBlurTexture(copyW, copyH);
            glBindTexture(GL_TEXTURE_2D, CachedBlurTexture);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, copyX, copyY, copyW, copyH);

            CachedBackdropVersion = BackdropVersion;
            CachedBlurX = (float)copyX;
            CachedBlurY = (float)copyYTop;
            CachedBlurW = (float)copyW;
            CachedBlurH = (float)copyH;
            CachedBlurColor[0] = color.r;
            CachedBlurColor[1] = color.g;
            CachedBlurColor[2] = color.b;
            CachedBlurColor[3] = color.a;
            CachedBlurShadowColor[0] = shadowColor.r;
            CachedBlurShadowColor[1] = shadowColor.g;
            CachedBlurShadowColor[2] = shadowColor.b;
            CachedBlurShadowColor[3] = shadowColor.a;
            CachedBlurRounding = rounding;
            CachedBlurAmount = blurAmount;
            CachedBlurShadowBlur = shadowBlur;
            CachedBlurShadowOffsetX = shadowOffsetX;
            CachedBlurShadowOffsetY = shadowOffsetY;
            BlurCacheValid = true;
        }
    }
}

bool Renderer::LoadFont(const std::string& fontPath, float fontSize, unsigned int startChar, unsigned int endChar) {
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size)) return false;

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, buffer.data(), 0)) return false;

    float scale = stbtt_ScaleForPixelHeight(&font, fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned int c = startChar; c < endChar; ++c) {
        if (Characters.find(c) != Characters.end()) continue;
        if (stbtt_FindGlyphIndex(&font, c) == 0) continue;

        int width = 0;
        int height = 0;
        int xoff = 0;
        int yoff = 0;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font, 0, scale, c, &width, &height, &xoff, &yoff);
        if (!bitmap) continue;

        int advance = 0;
        int lsb = 0;
        stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Characters[c] = {
            texture,
            {width, height},
            {xoff, yoff},
            (unsigned int)(advance * scale)
        };
        stbtt_FreeBitmap(bitmap, nullptr);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Renderer::DrawTextStr(const std::string& text, float x, float y, const Color& color, float scale) {
    if (text.empty()) {
        return;
    }

    float textWidth = std::max(MeasureTextWidth(text, scale), 24.0f * scale);
    float textHeight = 32.0f * scale;
    if (!RectIntersectsActiveRedraw(x, y - textHeight, textWidth, textHeight * 1.35f)) {
        return;
    }

    if (CurrentActiveProgram != TextShaderProgram) {
        glUseProgram(TextShaderProgram);
        CurrentActiveProgram = TextShaderProgram;
    }

    glUniform4f(TextColorLoc, color.r, color.g, color.b, color.a);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(TextVAO);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    size_t i = 0;
    while (i < text.length()) {
        unsigned int c = 0;
        unsigned char ch = text[i];
        if (ch <= 0x7F) { c = ch; i++; }
        else if ((ch & 0xE0) == 0xC0) { if (i + 1 < text.length()) { c = ((ch & 0x1F) << 6) | (text[i + 1] & 0x3F); i += 2; } else break; }
        else if ((ch & 0xF0) == 0xE0) { if (i + 2 < text.length()) { c = ((ch & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F); i += 3; } else break; }
        else if ((ch & 0xF8) == 0xF0) { if (i + 3 < text.length()) { c = ((ch & 0x07) << 18) | ((text[i + 1] & 0x3F) << 12) | ((text[i + 2] & 0x3F) << 6) | (text[i + 3] & 0x3F); i += 4; } else break; }
        else { i++; continue; }

        if (c == ' ') {
            x += 24.0f * 0.3f * scale;
            continue;
        }

        if (Characters.find(c) == Characters.end()) continue;
        Character charData = Characters[c];

        float xpos = x + charData.Bearing[0] * scale;
        float ypos = y + charData.Bearing[1] * scale;
        float w = charData.Size[0] * scale;
        float h = charData.Size[1] * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos,     ypos,       0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f }
        };

        glBindTexture(GL_TEXTURE_2D, charData.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, TextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += charData.Advance * scale;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

float Renderer::MeasureTextWidth(const std::string& text, float scale) {
    float width = 0.0f;
    size_t i = 0;
    while (i < text.length()) {
        unsigned int c = 0;
        unsigned char ch = text[i];
        if (ch <= 0x7F) { c = ch; i++; }
        else if ((ch & 0xE0) == 0xC0) { if (i + 1 < text.length()) { c = ((ch & 0x1F) << 6) | (text[i + 1] & 0x3F); i += 2; } else break; }
        else if ((ch & 0xF0) == 0xE0) { if (i + 2 < text.length()) { c = ((ch & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F); i += 3; } else break; }
        else if ((ch & 0xF8) == 0xF0) { if (i + 3 < text.length()) { c = ((ch & 0x07) << 18) | ((text[i + 1] & 0x3F) << 12) | ((text[i + 2] & 0x3F) << 6) | (text[i + 3] & 0x3F); i += 4; } else break; }
        else { i++; continue; }

        if (Characters.find(c) != Characters.end()) {
            width += Characters[c].Advance * scale;
        } else if (c == ' ') {
            width += 24.0f * 0.3f * scale;
        }
    }
    return width;
}

void Renderer::RequestRepaint(float duration) {
    State.needsRepaint = true;
    if (duration > State.animationTimeLeft) {
        State.animationTimeLeft = duration;
    }
}

void Renderer::InvalidateAll() {
    State.needsRepaint = true;
    State.fullScreenDirty = true;
}

void Renderer::InvalidateBackdrop() {
    ++BackdropVersion;
    BlurCacheValid = false;
    State.needsRepaint = true;
    State.fullScreenDirty = true;
}

void Renderer::AddDirtyRect(float x, float y, float w, float h) {
    if (State.fullScreenDirty) return;

    if (State.dirtyX1 == State.dirtyX2) {
        State.dirtyX1 = x;
        State.dirtyY1 = y;
        State.dirtyX2 = x + w;
        State.dirtyY2 = y + h;
    } else {
        State.dirtyX1 = std::min(State.dirtyX1, x);
        State.dirtyY1 = std::min(State.dirtyY1, y);
        State.dirtyX2 = std::max(State.dirtyX2, x + w);
        State.dirtyY2 = std::max(State.dirtyY2, y + h);
    }
}

void Renderer::ApplyScissor() {
    glDisable(GL_SCISSOR_TEST);
}

void Renderer::ClearDirtyRect() {
    State.fullScreenDirty = false;
    State.dirtyX1 = State.dirtyY1 = State.dirtyX2 = State.dirtyY2 = 0;
}

bool Renderer::ShouldRepaint() {
    if (State.needsRepaint) {
        State.needsRepaint = false;
        return true;
    }
    if (State.animationTimeLeft > 0) {
        State.animationTimeLeft -= State.deltaTime;
        return true;
    }
    return false;
}

void Widget::GetAbsoluteBounds(float& outX, float& outY) {
    outX = x;
    outY = y;
    switch (anchor) {
        case Anchor::TopCenter: outX += State.screenW / 2 - width / 2; break;
        case Anchor::TopRight: outX += State.screenW - width; break;
        case Anchor::CenterLeft: outY += State.screenH / 2 - height / 2; break;
        case Anchor::Center: outX += State.screenW / 2 - width / 2; outY += State.screenH / 2 - height / 2; break;
        case Anchor::CenterRight: outX += State.screenW - width; outY += State.screenH / 2 - height / 2; break;
        case Anchor::BottomLeft: outY += State.screenH - height; break;
        case Anchor::BottomCenter: outX += State.screenW / 2 - width / 2; outY += State.screenH - height; break;
        case Anchor::BottomRight: outX += State.screenW - width; outY += State.screenH - height; break;
        case Anchor::TopLeft: default: break;
    }
}

bool Widget::IsHovered() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);
    return State.mouseX >= absX && State.mouseX <= absX + width &&
           State.mouseY >= absY && State.mouseY <= absY + height;
}

void Widget::MarkDirty(float expand, float duration) {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);
    Renderer::AddDirtyRect(absX - expand, absY - expand, width + expand * 2, height + expand * 2);
    Renderer::RequestRepaint(duration);
}

} // namespace EUINEO
