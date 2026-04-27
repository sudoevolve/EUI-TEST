#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "core/dsl.h"
#include "core/event.h"

#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace core::dsl {

class Runtime {
    struct RenderTransform {
        bool active = false;
        Vec2 origin = {0.0f, 0.0f};
        float scale = 1.0f;
    };

public:
    bool initialize() {
        return true;
    }

    template <typename ComposeFn>
    void compose(const std::string& pageId, float logicalWidth, float logicalHeight, ComposeFn&& composeFn) {
        const Screen screen{logicalWidth, logicalHeight};
        ui_.begin(pageId);
        composeFn(ui_, screen);
        ui_.end();
        ui_.layout(screen);

        if (logicalWidth_ != logicalWidth || logicalHeight_ != logicalHeight) {
            needsRender_ = true;
            fullRedraw_ = true;
        }
        logicalWidth_ = logicalWidth;
        logicalHeight_ = logicalHeight;
    }

    bool update(GLFWwindow* window, float deltaSeconds, float pointerScale, float dpiScale) {
        const PointerEvent event = readPointerEvent(window, pointerScale);
        animating_ = false;
        needsCompose_ = false;

        forEachElement([&](const Element& element) {
            if (element.kind == ElementKind::Rect) {
                updateRect(element, event, deltaSeconds, dpiScale);
            } else if (element.kind == ElementKind::Text) {
                updateText(element, deltaSeconds);
            }
        });

        const bool result = needsRender_;
        needsRender_ = false;
        return result;
    }

    bool isAnimating() const {
        return animating_;
    }

    bool needsCompose() const {
        return needsCompose_;
    }

    void markFullRedraw() {
        fullRedraw_ = true;
        needsRender_ = true;
    }

    void render(int windowWidth, int windowHeight, float dpiScale, const Color& clearColor) {
        if (!ensureRenderCache(windowWidth, windowHeight)) {
            renderDirect(windowWidth, windowHeight, dpiScale);
            dirtyRects_.clear();
            fullRedraw_ = false;
            return;
        }

        const std::vector<Rect> dirtyRects = resolveDirtyRects(windowWidth, windowHeight, dpiScale);
        if (dirtyRects.empty() && !fullRedraw_) {
            blitRenderCache(windowWidth, windowHeight);
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, cacheFramebuffer_);
        glViewport(0, 0, windowWidth, windowHeight);

        if (fullRedraw_) {
            glDisable(GL_SCISSOR_TEST);
            glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            glClear(GL_COLOR_BUFFER_BIT);
            renderDirect(windowWidth, windowHeight, dpiScale);
        } else {
            glEnable(GL_SCISSOR_TEST);
            for (const Rect& dirty : dirtyRects) {
                applyScissor(dirty, windowHeight);
                glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
                glClear(GL_COLOR_BUFFER_BIT);
                renderDirect(windowWidth, windowHeight, dpiScale, &dirty);
            }
            glDisable(GL_SCISSOR_TEST);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        blitRenderCache(windowWidth, windowHeight);
        dirtyRects_.clear();
        fullRedraw_ = false;
    }

    void render(int windowWidth, int windowHeight, float dpiScale) {
        const RenderTransform identity;
        for (const auto& root : ui_.roots()) {
            renderElement(*root, windowWidth, windowHeight, dpiScale, identity);
        }
    }

    void shutdown() {
        for (auto& item : rects_) {
            if (item.second.initialized) {
                item.second.primitive->destroy();
            }
        }
        for (auto& item : texts_) {
            if (item.second.initialized) {
                item.second.primitive->destroy();
            }
        }
        rects_.clear();
        texts_.clear();
        releaseRenderCache();
    }

private:
    struct RectInstance {
        std::unique_ptr<RoundedRectPrimitive> primitive = std::make_unique<RoundedRectPrimitive>();
        InteractionState interaction;
        bool initialized = false;
        SmoothedValue<float> hoverBlend;
        SmoothedValue<float> pressBlend;
        AnimatedValue<LayoutRect> frame;
        AnimatedValue<Color> color;
        AnimatedValue<float> radius;
        AnimatedValue<float> blur;
        AnimatedValue<float> opacity;
        AnimatedValue<Border> border;
        AnimatedValue<Shadow> shadow;
        AnimatedValue<Transform> transform;
    };

    struct TextInstance {
        std::unique_ptr<TextPrimitive> primitive = std::make_unique<TextPrimitive>();
        bool initialized = false;
        AnimatedValue<LayoutRect> frame;
        AnimatedValue<Color> color;
        AnimatedValue<float> opacity;
    };

    struct LogicalDirtyRect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    template <typename Fn>
    void forEachElement(Fn&& fn) const {
        for (const auto& root : ui_.roots()) {
            forEachElement(*root, fn);
        }
    }

    template <typename Fn>
    static void forEachElement(const Element& element, Fn&& fn) {
        fn(element);
        for (const auto& child : element.children) {
            forEachElement(*child, fn);
        }
    }

    static float toPixels(float value, float dpiScale) {
        return value * dpiScale;
    }

    static Rect toPixelRect(const LayoutRect& frame, float dpiScale) {
        return {
            toPixels(frame.x, dpiScale),
            toPixels(frame.y, dpiScale),
            toPixels(frame.width, dpiScale),
            toPixels(frame.height, dpiScale)
        };
    }

    static Rect toPixelRect(const Rect& rect, float dpiScale) {
        return {
            toPixels(rect.x, dpiScale),
            toPixels(rect.y, dpiScale),
            toPixels(rect.width, dpiScale),
            toPixels(rect.height, dpiScale)
        };
    }

    static bool intersects(const Rect& a, const Rect& b) {
        return a.x < b.x + b.width &&
               a.x + a.width > b.x &&
               a.y < b.y + b.height &&
               a.y + a.height > b.y;
    }

    static Rect unionRect(const Rect& a, const Rect& b) {
        const float left = std::min(a.x, b.x);
        const float top = std::min(a.y, b.y);
        const float right = std::max(a.x + a.width, b.x + b.width);
        const float bottom = std::max(a.y + a.height, b.y + b.height);
        return {left, top, right - left, bottom - top};
    }

    static Rect inflateRect(Rect rect, float amount) {
        rect.x -= amount;
        rect.y -= amount;
        rect.width += amount * 2.0f;
        rect.height += amount * 2.0f;
        return rect;
    }

    static Rect visualRect(const LayoutRect& frame, const Shadow& shadow, float blur) {
        Rect rect{frame.x, frame.y, frame.width, frame.height};
        if (shadow.enabled) {
            Rect shadowRect{
                frame.x + shadow.offset.x - shadow.spread,
                frame.y + shadow.offset.y - shadow.spread,
                frame.width + shadow.spread * 2.0f,
                frame.height + shadow.spread * 2.0f
            };
            shadowRect = inflateRect(shadowRect, shadow.blur * 1.4f + 2.0f);
            rect = unionRect(rect, shadowRect);
        }
        if (blur > 0.0f) {
            rect = inflateRect(rect, blur + 2.0f);
        }
        return rect;
    }

    void addDirtyRect(const Rect& rect) {
        if (rect.width <= 0.0f || rect.height <= 0.0f) {
            return;
        }
        dirtyRects_.push_back({rect.x, rect.y, rect.width, rect.height});
        needsRender_ = true;
    }

    void addDirtyUnion(const Rect& before, const Rect& after) {
        addDirtyRect(unionRect(before, after));
    }

    static Vec2 applyRenderTransform(Vec2 point, const RenderTransform& transform) {
        if (!transform.active) {
            return point;
        }
        return {
            transform.origin.x + (point.x - transform.origin.x) * transform.scale,
            transform.origin.y + (point.y - transform.origin.y) * transform.scale
        };
    }

    static Rect applyRenderTransform(const Rect& rect, const RenderTransform& transform) {
        if (!transform.active) {
            return rect;
        }

        const Vec2 topLeft = applyRenderTransform(Vec2{rect.x, rect.y}, transform);
        return {
            topLeft.x,
            topLeft.y,
            rect.width * transform.scale,
            rect.height * transform.scale
        };
    }

    static Border scaleBorder(Border border, float dpiScale) {
        border.width = toPixels(border.width, dpiScale);
        return border;
    }

    static Shadow scaleShadow(Shadow shadow, float dpiScale) {
        shadow.offset.x = toPixels(shadow.offset.x, dpiScale);
        shadow.offset.y = toPixels(shadow.offset.y, dpiScale);
        shadow.blur = toPixels(shadow.blur, dpiScale);
        shadow.spread = toPixels(shadow.spread, dpiScale);
        return shadow;
    }

    static Transform scaleTransform(Transform transform, float dpiScale) {
        transform.translate.x = toPixels(transform.translate.x, dpiScale);
        transform.translate.y = toPixels(transform.translate.y, dpiScale);
        return transform;
    }

    static bool isRectAnimating(const Element& element, const RectInstance& instance) {
        return instance.hoverBlend.isMovingTo(element.interactive && instance.interaction.hover ? 1.0f : 0.0f) ||
               instance.pressBlend.isMovingTo(element.interactive && instance.interaction.pressed ? 1.0f : 0.0f) ||
               instance.frame.isActive() ||
               instance.color.isActive() ||
               instance.radius.isActive() ||
               instance.blur.isActive() ||
               instance.opacity.isActive() ||
               instance.border.isActive() ||
               instance.shadow.isActive() ||
               instance.transform.isActive();
    }

    static bool isTextAnimating(const TextInstance& instance) {
        return instance.frame.isActive() ||
               instance.color.isActive() ||
               instance.opacity.isActive();
    }

    static bool shouldAnimate(const Element& element, AnimProperty property) {
        return element.transition.enabled && hasAnimProperty(element.transition.properties, property);
    }

    RectInstance& rectInstance(const std::string& id) {
        return rects_.try_emplace(id).first->second;
    }

    TextInstance& textInstance(const std::string& id) {
        return texts_.try_emplace(id).first->second;
    }

    void updateRect(const Element& element, const PointerEvent& event, float deltaSeconds, float dpiScale) {
        RectInstance& instance = rectInstance(element.id);
        const Rect beforeRect = visualRect(instance.frame.value(), instance.shadow.value(), instance.blur.value());

        if (element.interactive) {
            const Rect bounds = toPixelRect(element.frame, dpiScale);
            instance.interaction.update(bounds, event);
            if (instance.interaction.clicked && element.onClick) {
                element.onClick();
                needsCompose_ = true;
            }
        }

        const bool hoverChanged = instance.hoverBlend.update(element.interactive && instance.interaction.hover ? 1.0f : 0.0f, 9.0f, deltaSeconds);
        const bool pressChanged = instance.pressBlend.update(element.interactive && instance.interaction.pressed ? 1.0f : 0.0f, 16.0f, deltaSeconds);
        const float hover = instance.hoverBlend.value();
        const float press = instance.pressBlend.value();
        const Color hoverColor = mixColor(element.color, element.hoverColor, hover);
        const Color currentColor = mixColor(hoverColor, element.pressedColor, press);

        bool changed = hoverChanged || pressChanged || (element.interactive && instance.interaction.changed);
        changed = instance.frame.setTarget(element.frame, element.transition, shouldAnimate(element, AnimProperty::Frame)) || changed;
        changed = instance.color.setTarget(currentColor, element.transition, shouldAnimate(element, AnimProperty::Color)) || changed;
        changed = instance.radius.setTarget(element.radius, element.transition, shouldAnimate(element, AnimProperty::Radius)) || changed;
        changed = instance.blur.setTarget(element.blur, element.transition, shouldAnimate(element, AnimProperty::Blur)) || changed;
        changed = instance.opacity.setTarget(element.opacity, element.transition, shouldAnimate(element, AnimProperty::Opacity)) || changed;
        changed = instance.border.setTarget(element.border, element.transition, shouldAnimate(element, AnimProperty::Border)) || changed;
        changed = instance.shadow.setTarget(element.shadow, element.transition, shouldAnimate(element, AnimProperty::Shadow)) || changed;
        changed = instance.transform.setTarget(element.transform, element.transition, shouldAnimate(element, AnimProperty::Transform)) || changed;

        changed = instance.frame.tick(deltaSeconds) || changed;
        changed = instance.color.tick(deltaSeconds) || changed;
        changed = instance.radius.tick(deltaSeconds) || changed;
        changed = instance.blur.tick(deltaSeconds) || changed;
        changed = instance.opacity.tick(deltaSeconds) || changed;
        changed = instance.border.tick(deltaSeconds) || changed;
        changed = instance.shadow.tick(deltaSeconds) || changed;
        changed = instance.transform.tick(deltaSeconds) || changed;

        if (changed) {
            const Rect afterRect = visualRect(instance.frame.value(), instance.shadow.value(), instance.blur.value());
            addDirtyUnion(beforeRect, afterRect);
        }
        animating_ = animating_ || isRectAnimating(element, instance);
    }

    void updateText(const Element& element, float deltaSeconds) {
        TextInstance& instance = textInstance(element.id);
        const Rect beforeRect{instance.frame.value().x, instance.frame.value().y, instance.frame.value().width, instance.frame.value().height};

        bool changed = false;
        changed = instance.frame.setTarget(element.frame, element.transition, shouldAnimate(element, AnimProperty::Frame)) || changed;
        changed = instance.color.setTarget(element.textColor, element.transition, shouldAnimate(element, AnimProperty::TextColor)) || changed;
        changed = instance.opacity.setTarget(element.opacity, element.transition, shouldAnimate(element, AnimProperty::Opacity)) || changed;

        changed = instance.frame.tick(deltaSeconds) || changed;
        changed = instance.color.tick(deltaSeconds) || changed;
        changed = instance.opacity.tick(deltaSeconds) || changed;

        if (changed) {
            const Rect afterRect{instance.frame.value().x, instance.frame.value().y, instance.frame.value().width, instance.frame.value().height};
            addDirtyUnion(beforeRect, afterRect);
        }
        animating_ = animating_ || isTextAnimating(instance);
    }

    RenderTransform resolveRenderTransform(const Element& element, float dpiScale, const RenderTransform& inherited) {
        if (element.visualStateSourceId.empty()) {
            return inherited;
        }

        const auto instance = rects_.find(element.visualStateSourceId);
        if (instance == rects_.end()) {
            return inherited;
        }

        const float scale = 1.0f - (1.0f - element.pressedScale) * instance->second.pressBlend.value();
        if (std::fabs(scale - 1.0f) <= 0.0001f) {
            return inherited;
        }

        const Rect frame = applyRenderTransform(toPixelRect(instance->second.frame.value(), dpiScale), inherited);
        return {
            true,
            {frame.x + frame.width * 0.5f, frame.y + frame.height * 0.5f},
            scale
        };
    }

    bool ensureRenderCache(int width, int height) {
        width = std::max(1, width);
        height = std::max(1, height);
        if (cacheFramebuffer_ != 0 && cacheTexture_ != 0 && cacheWidth_ == width && cacheHeight_ == height) {
            return true;
        }

        releaseRenderCache();

        glGenFramebuffers(1, &cacheFramebuffer_);
        glGenTextures(1, &cacheTexture_);
        glBindTexture(GL_TEXTURE_2D, cacheTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, cacheFramebuffer_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cacheTexture_, 0);
        const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (!complete) {
            releaseRenderCache();
            return false;
        }

        cacheWidth_ = width;
        cacheHeight_ = height;
        fullRedraw_ = true;
        return true;
    }

    void releaseRenderCache() {
        if (cacheTexture_ != 0) {
            glDeleteTextures(1, &cacheTexture_);
            cacheTexture_ = 0;
        }
        if (cacheFramebuffer_ != 0) {
            glDeleteFramebuffers(1, &cacheFramebuffer_);
            cacheFramebuffer_ = 0;
        }
        cacheWidth_ = 0;
        cacheHeight_ = 0;
    }

    std::vector<Rect> resolveDirtyRects(int windowWidth, int windowHeight, float dpiScale) const {
        if (fullRedraw_) {
            return {};
        }

        std::vector<Rect> rects;
        Rect merged;
        bool hasMerged = false;
        for (const LogicalDirtyRect& dirty : dirtyRects_) {
            const Rect logicalRect{dirty.x, dirty.y, dirty.width, dirty.height};
            Rect rect = toPixelRect(logicalRect, dpiScale);
            const float left = std::clamp(std::floor(rect.x), 0.0f, static_cast<float>(windowWidth));
            const float top = std::clamp(std::floor(rect.y), 0.0f, static_cast<float>(windowHeight));
            const float right = std::clamp(std::ceil(rect.x + rect.width), 0.0f, static_cast<float>(windowWidth));
            const float bottom = std::clamp(std::ceil(rect.y + rect.height), 0.0f, static_cast<float>(windowHeight));
            if (right <= left || bottom <= top) {
                continue;
            }
            rect = {left, top, right - left, bottom - top};
            merged = hasMerged ? unionRect(merged, rect) : rect;
            hasMerged = true;
        }

        if (hasMerged) {
            rects.push_back(merged);
        }
        return rects;
    }

    static void applyScissor(const Rect& rect, int windowHeight) {
        const GLint x = static_cast<GLint>(std::floor(rect.x));
        const GLint y = static_cast<GLint>(std::floor(static_cast<float>(windowHeight) - rect.y - rect.height));
        const GLsizei width = static_cast<GLsizei>(std::ceil(rect.width));
        const GLsizei height = static_cast<GLsizei>(std::ceil(rect.height));
        glScissor(x, std::max<GLint>(0, y), std::max<GLsizei>(1, width), std::max<GLsizei>(1, height));
    }

    void blitRenderCache(int windowWidth, int windowHeight) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, cacheFramebuffer_);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, windowWidth, windowHeight,
                          0, 0, windowWidth, windowHeight,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderDirect(int windowWidth, int windowHeight, float dpiScale, const Rect* dirtyRect = nullptr) {
        const RenderTransform identity;
        for (const auto& root : ui_.roots()) {
            renderElement(*root, windowWidth, windowHeight, dpiScale, identity, dirtyRect);
        }
    }

    void renderElement(const Element& element,
                       int windowWidth,
                       int windowHeight,
                       float dpiScale,
                       const RenderTransform& inheritedTransform,
                       const Rect* dirtyRect = nullptr) {
        const RenderTransform renderTransform = resolveRenderTransform(element, dpiScale, inheritedTransform);

        if (element.kind == ElementKind::Rect) {
            if (!dirtyRect || intersects(toPixelRect(visualRect(rectInstance(element.id).frame.value(), rectInstance(element.id).shadow.value(), rectInstance(element.id).blur.value()), dpiScale), *dirtyRect)) {
                renderRect(element, windowWidth, windowHeight, dpiScale, renderTransform);
            }
        } else if (element.kind == ElementKind::Text) {
            const LayoutRect frame = textInstance(element.id).frame.value();
            if (!dirtyRect || intersects(toPixelRect(frame, dpiScale), *dirtyRect)) {
                renderText(element, windowWidth, windowHeight, dpiScale, renderTransform);
            }
        }

        for (const auto& child : element.children) {
            renderElement(*child, windowWidth, windowHeight, dpiScale, renderTransform, dirtyRect);
        }
    }

    void renderRect(const Element& element,
                    int windowWidth,
                    int windowHeight,
                    float dpiScale,
                    const RenderTransform& renderTransform) {
        RectInstance& instance = rectInstance(element.id);
        if (!instance.initialized) {
            instance.initialized = instance.primitive->initialize();
            if (!instance.initialized) {
                return;
            }
        }

        const Rect frame = toPixelRect(instance.frame.value(), dpiScale);
        const float visualScale = renderTransform.active ? renderTransform.scale : 1.0f;
        const Color currentColor = instance.color.value();
        Transform transform = scaleTransform(instance.transform.value(), dpiScale);
        if (renderTransform.active && frame.width > 0.0f && frame.height > 0.0f) {
            transform.origin = {
                (renderTransform.origin.x - frame.x) / frame.width,
                (renderTransform.origin.y - frame.y) / frame.height
            };
            transform.scale.x *= visualScale;
            transform.scale.y *= visualScale;
        }

        instance.primitive->setBounds(frame.x, frame.y, frame.width, frame.height);
        instance.primitive->setColor(currentColor);
        instance.primitive->setGradient(element.gradient);
        instance.primitive->setBorder(scaleBorder(instance.border.value(), dpiScale));
        instance.primitive->setShadow(scaleShadow(instance.shadow.value(), dpiScale));
        instance.primitive->setCornerRadius(toPixels(instance.radius.value(), dpiScale));
        instance.primitive->setBlur(toPixels(instance.blur.value(), dpiScale));
        instance.primitive->setOpacity(instance.opacity.value());
        instance.primitive->setTransform(transform);
        instance.primitive->render(windowWidth, windowHeight);
    }

    void renderText(const Element& element,
                    int windowWidth,
                    int windowHeight,
                    float dpiScale,
                    const RenderTransform& renderTransform) {
        TextInstance& instance = textInstance(element.id);
        if (!instance.initialized) {
            instance.initialized = instance.primitive->initialize();
            if (!instance.initialized) {
                return;
            }
        }

        const Rect frame = toPixelRect(instance.frame.value(), dpiScale);
        const float visualScale = renderTransform.active ? renderTransform.scale : 1.0f;
        const float maxWidth = element.maxWidth > 0.0f ? toPixels(element.maxWidth, dpiScale) : frame.width;
        const float lineHeight = element.lineHeight > 0.0f ? toPixels(element.lineHeight, dpiScale) : 0.0f;
        Color textColor = instance.color.value();
        textColor.a *= instance.opacity.value();

        float x = frame.x;
        if (element.horizontalAlign == HorizontalAlign::Center) {
            x = frame.x + frame.width * 0.5f;
        } else if (element.horizontalAlign == HorizontalAlign::Right) {
            x = frame.x + frame.width;
        }

        float y = frame.y;
        if (element.verticalAlign == VerticalAlign::Center) {
            y = frame.y + frame.height * 0.5f;
        } else if (element.verticalAlign == VerticalAlign::Bottom) {
            y = frame.y + frame.height;
        }

        instance.primitive->setPosition(x, y);
        instance.primitive->setVisualScale(renderTransform.origin.x, renderTransform.origin.y, visualScale);
        instance.primitive->setText(element.text);
        instance.primitive->setFontFamily(element.fontFamily);
        instance.primitive->setFontSize(toPixels(element.fontSize, dpiScale));
        instance.primitive->setFontWeight(element.fontWeight);
        instance.primitive->setColor(textColor);
        instance.primitive->setMaxWidth(maxWidth);
        instance.primitive->setWrap(element.wrap);
        instance.primitive->setHorizontalAlign(element.horizontalAlign);
        instance.primitive->setVerticalAlign(element.verticalAlign);
        instance.primitive->setLineHeight(lineHeight);
        instance.primitive->render(windowWidth, windowHeight);
    }

    Ui ui_;
    std::unordered_map<std::string, RectInstance> rects_;
    std::unordered_map<std::string, TextInstance> texts_;
    std::vector<LogicalDirtyRect> dirtyRects_;
    bool needsRender_ = true;
    bool animating_ = false;
    bool needsCompose_ = false;
    bool fullRedraw_ = true;
    float logicalWidth_ = 0.0f;
    float logicalHeight_ = 0.0f;
    GLuint cacheFramebuffer_ = 0;
    GLuint cacheTexture_ = 0;
    int cacheWidth_ = 0;
    int cacheHeight_ = 0;
};

} // namespace core::dsl
