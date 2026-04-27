#pragma once

#include <glad/glad.h>

#include <algorithm>
#include <cmath>

namespace core {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool contains(double pointX, double pointY) const {
        return pointX >= x && pointX <= x + width &&
               pointY >= y && pointY <= y + height;
    }
};

enum class GradientDirection {
    Horizontal = 0,
    Vertical = 1
};

struct Gradient {
    bool enabled = false;
    Color start = {1.0f, 1.0f, 1.0f, 1.0f};
    Color end = {1.0f, 1.0f, 1.0f, 1.0f};
    GradientDirection direction = GradientDirection::Vertical;
};

struct Border {
    float width = 0.0f;
    Color color = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct Shadow {
    bool enabled = false;
    Vec2 offset = {0.0f, 4.0f};
    float blur = 8.0f;
    float spread = 0.0f;
    Color color = {0.0f, 0.0f, 0.0f, 0.28f};
};

struct Transform {
    Vec2 translate = {0.0f, 0.0f};
    Vec2 scale = {1.0f, 1.0f};
    float rotate = 0.0f;
    Vec2 origin = {0.5f, 0.5f};
};

class RoundedRectPrimitive {
public:
    RoundedRectPrimitive() = default;

    RoundedRectPrimitive(float x, float y, float width, float height)
        : bounds_{x, y, width, height} {}

    bool initialize() {
        const char* vertexSource =
            "#version 330 core\n"
            "layout(location = 0) in vec2 aScreenPos;\n"
            "layout(location = 1) in vec2 aLocalPos;\n"
            "uniform vec2 uWindowSize;\n"
            "out vec2 vLocalPos;\n"
            "void main() {\n"
            "    vLocalPos = aLocalPos;\n"
            "    vec2 ndc = vec2((aScreenPos.x / uWindowSize.x) * 2.0 - 1.0,\n"
            "                    1.0 - (aScreenPos.y / uWindowSize.y) * 2.0);\n"
            "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
            "}\n";

        const char* fragmentSource =
            "#version 330 core\n"
            "in vec2 vLocalPos;\n"
            "out vec4 FragColor;\n"
            "uniform vec4 uFillColor;\n"
            "uniform vec4 uGradientStart;\n"
            "uniform vec4 uGradientEnd;\n"
            "uniform vec4 uBorderColor;\n"
            "uniform vec4 uShadowColor;\n"
            "uniform vec2 uWindowSize;\n"
            "uniform vec4 uRect;\n"
            "uniform float uRadius;\n"
            "uniform float uBorderWidth;\n"
            "uniform float uOpacity;\n"
            "uniform float uShadowBlur;\n"
            "uniform float uBlurAmount;\n"
            "uniform int uUseGradient;\n"
            "uniform int uGradientDirection;\n"
            "uniform int uShadowPass;\n"
            "uniform sampler2D uBackdrop;\n"
            "float rand(vec2 co) {\n"
            "    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);\n"
            "}\n"
            "float roundedBoxDistance(vec2 point, vec2 halfSize, float radius) {\n"
            "    vec2 cornerVector = abs(point) - halfSize + vec2(radius);\n"
            "    return length(max(cornerVector, 0.0)) + min(max(cornerVector.x, cornerVector.y), 0.0) - radius;\n"
            "}\n"
            "vec3 backdropBlur(vec2 uv) {\n"
            "    vec2 pixelStep = 1.0 / max(uWindowSize, vec2(1.0));\n"
            "    float blurRadiusPx = uBlurAmount;\n"
            "    vec3 blurred = texture(uBackdrop, uv).rgb;\n"
            "    float repeats = mix(8.0, 24.0, clamp(blurRadiusPx / 36.0, 0.0, 1.0));\n"
            "    const float tau = 6.28318530718;\n"
            "    for (float i = 0.0; i < 24.0; i += 1.0) {\n"
            "        if (i >= repeats) break;\n"
            "        float angle = (i / repeats) * tau;\n"
            "        vec2 dir = vec2(cos(angle), sin(angle));\n"
            "        float radiusA = blurRadiusPx * (0.35 + 0.65 * rand(vec2(i, uv.x + uv.y)));\n"
            "        vec2 uvA = clamp(uv + dir * radiusA * pixelStep, pixelStep * 0.5, vec2(1.0) - pixelStep * 0.5);\n"
            "        blurred += texture(uBackdrop, uvA).rgb;\n"
            "        float angleB = angle + (0.5 * tau / repeats);\n"
            "        vec2 dirB = vec2(cos(angleB), sin(angleB));\n"
            "        float radiusB = blurRadiusPx * (0.20 + 0.80 * rand(vec2(i + 2.0, uv.x + uv.y + 24.0)));\n"
            "        vec2 uvB = clamp(uv + dirB * radiusB * pixelStep, pixelStep * 0.5, vec2(1.0) - pixelStep * 0.5);\n"
            "        blurred += texture(uBackdrop, uvB).rgb;\n"
            "    }\n"
            "    return blurred / (repeats * 2.0 + 1.0);\n"
            "}\n"
            "void main() {\n"
            "    vec2 center = uRect.xy + uRect.zw * 0.5;\n"
            "    float distanceToEdge = roundedBoxDistance(vLocalPos - center, uRect.zw * 0.5, uRadius);\n"
            "    float blur = max(uShadowBlur, 1.0);\n"
            "    if (uShadowPass == 1) {\n"
            "        float shadowAlpha = 1.0 - smoothstep(-blur, blur, distanceToEdge);\n"
            "        if (shadowAlpha <= 0.0) discard;\n"
            "        FragColor = vec4(uShadowColor.rgb, uShadowColor.a * shadowAlpha * uOpacity);\n"
            "        return;\n"
            "    }\n"
            "    float edgeWidth = max(fwidth(distanceToEdge), 0.75);\n"
            "    float shapeAlpha = 1.0 - smoothstep(-edgeWidth, edgeWidth, distanceToEdge);\n"
            "    if (shapeAlpha <= 0.0) discard;\n"
            "    float gradientAmount = uGradientDirection == 0 ?\n"
            "        clamp((vLocalPos.x - uRect.x) / max(uRect.z, 1.0), 0.0, 1.0) :\n"
            "        clamp((vLocalPos.y - uRect.y) / max(uRect.w, 1.0), 0.0, 1.0);\n"
            "    vec4 fill = uUseGradient == 1 ? mix(uGradientStart, uGradientEnd, gradientAmount) : uFillColor;\n"
            "    if (uBlurAmount > 0.0) {\n"
            "        vec2 backdropUv = gl_FragCoord.xy / max(uWindowSize, vec2(1.0));\n"
            "        vec3 blurred = backdropBlur(backdropUv);\n"
            "        fill = vec4(mix(blurred, fill.rgb, fill.a), 1.0);\n"
            "    }\n"
            "    float borderAlpha = uBorderWidth > 0.0 ? smoothstep(-uBorderWidth - edgeWidth, -uBorderWidth + edgeWidth, distanceToEdge) : 0.0;\n"
            "    vec4 color = mix(fill, uBorderColor, borderAlpha);\n"
            "    FragColor = vec4(color.rgb, color.a * shapeAlpha * uOpacity);\n"
            "}\n";

        if (!retainSharedResources(vertexSource, fragmentSource)) {
            return false;
        }

        const SharedResources& resources = sharedResources();
        vao_ = resources.vao;
        vbo_ = resources.vbo;
        shaderProgram_ = resources.shaderProgram;
        windowSizeLocation_ = resources.windowSizeLocation;
        fillColorLocation_ = resources.fillColorLocation;
        gradientStartLocation_ = resources.gradientStartLocation;
        gradientEndLocation_ = resources.gradientEndLocation;
        borderColorLocation_ = resources.borderColorLocation;
        shadowColorLocation_ = resources.shadowColorLocation;
        rectLocation_ = resources.rectLocation;
        radiusLocation_ = resources.radiusLocation;
        borderWidthLocation_ = resources.borderWidthLocation;
        opacityLocation_ = resources.opacityLocation;
        shadowBlurLocation_ = resources.shadowBlurLocation;
        blurAmountLocation_ = resources.blurAmountLocation;
        useGradientLocation_ = resources.useGradientLocation;
        gradientDirectionLocation_ = resources.gradientDirectionLocation;
        shadowPassLocation_ = resources.shadowPassLocation;
        backdropLocation_ = resources.backdropLocation;
        return true;
    }

    void destroy() {
        if (shaderProgram_) {
            releaseSharedResources();
        }
        vbo_ = 0;
        vao_ = 0;
        shaderProgram_ = 0;
        windowSizeLocation_ = -1;
        fillColorLocation_ = -1;
        gradientStartLocation_ = -1;
        gradientEndLocation_ = -1;
        borderColorLocation_ = -1;
        shadowColorLocation_ = -1;
        rectLocation_ = -1;
        radiusLocation_ = -1;
        borderWidthLocation_ = -1;
        opacityLocation_ = -1;
        shadowBlurLocation_ = -1;
        blurAmountLocation_ = -1;
        useGradientLocation_ = -1;
        gradientDirectionLocation_ = -1;
        shadowPassLocation_ = -1;
        backdropLocation_ = -1;
    }

    void setBounds(float x, float y, float width, float height) { bounds_ = {x, y, width, height}; }
    void setColor(const Color& color) { color_ = color; }
    void setGradient(const Gradient& gradient) { gradient_ = gradient; }
    void setCornerRadius(float radius) { cornerRadius_ = radius; }
    void setOpacity(float opacity) { opacity_ = std::clamp(opacity, 0.0f, 1.0f); }
    void setBorder(const Border& border) { border_ = border; }
    void setShadow(const Shadow& shadow) { shadow_ = shadow; }
    void setBlur(float blur) { blur_ = std::max(0.0f, blur); }
    void setTranslate(float x, float y) { transform_.translate = {x, y}; }
    void setScale(float x, float y) { transform_.scale = {x, y}; }
    void setRotate(float radians) { transform_.rotate = radians; }
    void setTransformOrigin(float x, float y) { transform_.origin = {x, y}; }
    void setTransform(const Transform& transform) { transform_ = transform; }

    const Rect& bounds() const { return bounds_; }
    const Color& color() const { return color_; }
    const Gradient& gradient() const { return gradient_; }
    const Border& border() const { return border_; }
    const Shadow& shadow() const { return shadow_; }
    float blur() const { return blur_; }
    const Transform& transform() const { return transform_; }
    float cornerRadius() const { return cornerRadius_; }
    float opacity() const { return opacity_; }

    void render(int windowWidth, int windowHeight) const {
        if (!shaderProgram_ || !vao_ || !vbo_) {
            return;
        }

        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (blur_ > 0.0f) {
            captureBackdrop(windowWidth, windowHeight);
        }

        if (shadow_.enabled) {
            drawShadow(windowWidth, windowHeight);
        }

        drawLayer(windowWidth, windowHeight, bounds_, bounds_, false, color_, shadow_.blur);

        if (!blendEnabled) {
            glDisable(GL_BLEND);
        }
    }

private:
    struct SharedResources {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint shaderProgram = 0;
        GLint windowSizeLocation = -1;
        GLint fillColorLocation = -1;
        GLint gradientStartLocation = -1;
        GLint gradientEndLocation = -1;
        GLint borderColorLocation = -1;
        GLint shadowColorLocation = -1;
        GLint rectLocation = -1;
        GLint radiusLocation = -1;
        GLint borderWidthLocation = -1;
        GLint opacityLocation = -1;
        GLint shadowBlurLocation = -1;
        GLint blurAmountLocation = -1;
        GLint useGradientLocation = -1;
        GLint gradientDirectionLocation = -1;
        GLint shadowPassLocation = -1;
        GLint backdropLocation = -1;
        GLuint backdropTexture = 0;
        int backdropWidth = 0;
        int backdropHeight = 0;
        int references = 0;
    };

    static SharedResources& sharedResources() {
        static SharedResources resources;
        return resources;
    }

    static bool retainSharedResources(const char* vertexSource, const char* fragmentSource) {
        SharedResources& resources = sharedResources();
        ++resources.references;
        if (resources.shaderProgram != 0) {
            return true;
        }

        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!vertexShader || !fragmentShader) {
            if (vertexShader) {
                glDeleteShader(vertexShader);
            }
            if (fragmentShader) {
                glDeleteShader(fragmentShader);
            }
            resources.references = std::max(0, resources.references - 1);
            return false;
        }

        resources.shaderProgram = glCreateProgram();
        glAttachShader(resources.shaderProgram, vertexShader);
        glAttachShader(resources.shaderProgram, fragmentShader);
        glLinkProgram(resources.shaderProgram);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = 0;
        glGetProgramiv(resources.shaderProgram, GL_LINK_STATUS, &linked);
        if (!linked) {
            glDeleteProgram(resources.shaderProgram);
            resources.shaderProgram = 0;
            resources.references = std::max(0, resources.references - 1);
            return false;
        }

        resources.windowSizeLocation = glGetUniformLocation(resources.shaderProgram, "uWindowSize");
        resources.fillColorLocation = glGetUniformLocation(resources.shaderProgram, "uFillColor");
        resources.gradientStartLocation = glGetUniformLocation(resources.shaderProgram, "uGradientStart");
        resources.gradientEndLocation = glGetUniformLocation(resources.shaderProgram, "uGradientEnd");
        resources.borderColorLocation = glGetUniformLocation(resources.shaderProgram, "uBorderColor");
        resources.shadowColorLocation = glGetUniformLocation(resources.shaderProgram, "uShadowColor");
        resources.rectLocation = glGetUniformLocation(resources.shaderProgram, "uRect");
        resources.radiusLocation = glGetUniformLocation(resources.shaderProgram, "uRadius");
        resources.borderWidthLocation = glGetUniformLocation(resources.shaderProgram, "uBorderWidth");
        resources.opacityLocation = glGetUniformLocation(resources.shaderProgram, "uOpacity");
        resources.shadowBlurLocation = glGetUniformLocation(resources.shaderProgram, "uShadowBlur");
        resources.blurAmountLocation = glGetUniformLocation(resources.shaderProgram, "uBlurAmount");
        resources.useGradientLocation = glGetUniformLocation(resources.shaderProgram, "uUseGradient");
        resources.gradientDirectionLocation = glGetUniformLocation(resources.shaderProgram, "uGradientDirection");
        resources.shadowPassLocation = glGetUniformLocation(resources.shaderProgram, "uShadowPass");
        resources.backdropLocation = glGetUniformLocation(resources.shaderProgram, "uBackdrop");

        glGenVertexArrays(1, &resources.vao);
        glGenBuffers(1, &resources.vbo);
        glBindVertexArray(resources.vao);
        glBindBuffer(GL_ARRAY_BUFFER, resources.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void*>(sizeof(float) * 2));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return resources.shaderProgram != 0 && resources.vao != 0 && resources.vbo != 0;
    }

    static void releaseSharedResources() {
        SharedResources& resources = sharedResources();
        resources.references = std::max(0, resources.references - 1);
        if (resources.references > 0) {
            return;
        }

        if (resources.vbo) {
            glDeleteBuffers(1, &resources.vbo);
            resources.vbo = 0;
        }
        if (resources.vao) {
            glDeleteVertexArrays(1, &resources.vao);
            resources.vao = 0;
        }
        if (resources.shaderProgram) {
            glDeleteProgram(resources.shaderProgram);
            resources.shaderProgram = 0;
        }
        if (resources.backdropTexture) {
            glDeleteTextures(1, &resources.backdropTexture);
            resources.backdropTexture = 0;
        }
        resources.backdropWidth = 0;
        resources.backdropHeight = 0;
        resources.windowSizeLocation = -1;
        resources.fillColorLocation = -1;
        resources.gradientStartLocation = -1;
        resources.gradientEndLocation = -1;
        resources.borderColorLocation = -1;
        resources.shadowColorLocation = -1;
        resources.rectLocation = -1;
        resources.radiusLocation = -1;
        resources.borderWidthLocation = -1;
        resources.opacityLocation = -1;
        resources.shadowBlurLocation = -1;
        resources.blurAmountLocation = -1;
        resources.useGradientLocation = -1;
        resources.gradientDirectionLocation = -1;
        resources.shadowPassLocation = -1;
        resources.backdropLocation = -1;
    }

    static void ensureBackdropTexture(int width, int height) {
        SharedResources& resources = sharedResources();
        width = std::max(1, width);
        height = std::max(1, height);
        if (resources.backdropTexture != 0 &&
            resources.backdropWidth == width &&
            resources.backdropHeight == height) {
            return;
        }

        if (resources.backdropTexture == 0) {
            glGenTextures(1, &resources.backdropTexture);
        }
        resources.backdropWidth = width;
        resources.backdropHeight = height;
        glBindTexture(GL_TEXTURE_2D, resources.backdropTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    static void captureBackdrop(int width, int height) {
        ensureBackdropTexture(width, height);
        SharedResources& resources = sharedResources();
        glBindTexture(GL_TEXTURE_2D, resources.backdropTexture);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, std::max(1, width), std::max(1, height));
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    static GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    Vec2 transformPoint(float x, float y) const {
        const Vec2 origin = {
            bounds_.x + bounds_.width * transform_.origin.x,
            bounds_.y + bounds_.height * transform_.origin.y
        };

        const float scaledX = (x - origin.x) * transform_.scale.x;
        const float scaledY = (y - origin.y) * transform_.scale.y;
        const float cosine = std::cos(transform_.rotate);
        const float sine = std::sin(transform_.rotate);

        return {
            origin.x + scaledX * cosine - scaledY * sine + transform_.translate.x,
            origin.y + scaledX * sine + scaledY * cosine + transform_.translate.y
        };
    }

    static Rect expandRect(const Rect& rect, float amount) {
        return {
            rect.x - amount,
            rect.y - amount,
            rect.width + amount * 2.0f,
            rect.height + amount * 2.0f
        };
    }

    static Color withAlpha(Color color, float alphaScale) {
        color.a *= alphaScale;
        return color;
    }

    void drawShadow(int windowWidth, int windowHeight) const {
        Rect shadowShape = bounds_;
        shadowShape.x += shadow_.offset.x - shadow_.spread;
        shadowShape.y += shadow_.offset.y - shadow_.spread;
        shadowShape.width += shadow_.spread * 2.0f;
        shadowShape.height += shadow_.spread * 2.0f;

        const float blur = std::max(shadow_.blur, 1.0f);

        Rect ambientShape = shadowShape;
        ambientShape.x += shadow_.offset.x * 0.15f;
        ambientShape.y += shadow_.offset.y * 0.15f;
        drawLayer(windowWidth, windowHeight, expandRect(ambientShape, blur * 1.4f), ambientShape,
                  true, withAlpha(shadow_.color, 0.22f), blur * 1.4f);

        Rect midShape = shadowShape;
        midShape.x += shadow_.offset.x * 0.65f;
        midShape.y += shadow_.offset.y * 0.65f;
        drawLayer(windowWidth, windowHeight, expandRect(midShape, blur * 0.85f), midShape,
                  true, withAlpha(shadow_.color, 0.34f), blur * 0.85f);

        drawLayer(windowWidth, windowHeight, expandRect(shadowShape, blur * 0.38f), shadowShape,
                  true, withAlpha(shadow_.color, 0.26f), blur * 0.38f);
    }

    void drawLayer(int windowWidth,
                   int windowHeight,
                   const Rect& geometryBounds,
                   const Rect& sdfBounds,
                   bool shadowPass,
                   const Color& layerColor,
                   float blur) const {
        const float left = geometryBounds.x;
        const float top = geometryBounds.y;
        const float right = geometryBounds.x + geometryBounds.width;
        const float bottom = geometryBounds.y + geometryBounds.height;

        const Vec2 p0 = transformPoint(left, top);
        const Vec2 p1 = transformPoint(right, top);
        const Vec2 p2 = transformPoint(right, bottom);
        const Vec2 p3 = transformPoint(left, bottom);

        const float vertices[] = {
            p0.x, p0.y, left, top,
            p1.x, p1.y, right, top,
            p2.x, p2.y, right, bottom,
            p0.x, p0.y, left, top,
            p2.x, p2.y, right, bottom,
            p3.x, p3.y, left, bottom
        };

        const float radius = std::clamp(cornerRadius_, 0.0f, std::min(sdfBounds.width, sdfBounds.height) * 0.5f);
        const float borderWidth = shadowPass ? 0.0f : std::clamp(border_.width, 0.0f, std::min(sdfBounds.width, sdfBounds.height) * 0.5f);

        glUseProgram(shaderProgram_);
        glUniform2f(windowSizeLocation_, static_cast<float>(windowWidth), static_cast<float>(windowHeight));
        glUniform4f(fillColorLocation_, color_.r, color_.g, color_.b, color_.a);
        glUniform4f(gradientStartLocation_, gradient_.start.r, gradient_.start.g, gradient_.start.b, gradient_.start.a);
        glUniform4f(gradientEndLocation_, gradient_.end.r, gradient_.end.g, gradient_.end.b, gradient_.end.a);
        glUniform4f(borderColorLocation_, border_.color.r, border_.color.g, border_.color.b, border_.color.a);
        glUniform4f(shadowColorLocation_, layerColor.r, layerColor.g, layerColor.b, layerColor.a);
        glUniform4f(rectLocation_, sdfBounds.x, sdfBounds.y, sdfBounds.width, sdfBounds.height);
        glUniform1f(radiusLocation_, radius);
        glUniform1f(borderWidthLocation_, borderWidth);
        glUniform1f(opacityLocation_, opacity_);
        glUniform1f(shadowBlurLocation_, blur);
        glUniform1f(blurAmountLocation_, shadowPass ? 0.0f : blur_);
        glUniform1i(useGradientLocation_, gradient_.enabled && !shadowPass ? 1 : 0);
        glUniform1i(gradientDirectionLocation_, static_cast<int>(gradient_.direction));
        glUniform1i(shadowPassLocation_, shadowPass ? 1 : 0);
        glUniform1i(backdropLocation_, 0);

        if (!shadowPass && blur_ > 0.0f) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sharedResources().backdropTexture);
        }
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        if (!shadowPass && blur_ > 0.0f) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    Rect bounds_;
    Color color_ = {1.0f, 1.0f, 1.0f, 1.0f};
    Gradient gradient_;
    Border border_;
    Shadow shadow_;
    Transform transform_;
    float cornerRadius_ = 0.0f;
    float blur_ = 0.0f;
    float opacity_ = 1.0f;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint shaderProgram_ = 0;
    GLint windowSizeLocation_ = -1;
    GLint fillColorLocation_ = -1;
    GLint gradientStartLocation_ = -1;
    GLint gradientEndLocation_ = -1;
    GLint borderColorLocation_ = -1;
    GLint shadowColorLocation_ = -1;
    GLint rectLocation_ = -1;
    GLint radiusLocation_ = -1;
    GLint borderWidthLocation_ = -1;
    GLint opacityLocation_ = -1;
    GLint shadowBlurLocation_ = -1;
    GLint blurAmountLocation_ = -1;
    GLint useGradientLocation_ = -1;
    GLint gradientDirectionLocation_ = -1;
    GLint shadowPassLocation_ = -1;
    GLint backdropLocation_ = -1;
};

inline Color mixColor(const Color& from, const Color& to, float amount) {
    const float clampedAmount = std::clamp(amount, 0.0f, 1.0f);
    const float inverse = 1.0f - clampedAmount;
    return {
        from.r * inverse + to.r * clampedAmount,
        from.g * inverse + to.g * clampedAmount,
        from.b * inverse + to.b * clampedAmount,
        from.a * inverse + to.a * clampedAmount
    };
}

} // namespace core
