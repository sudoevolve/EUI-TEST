#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "core/dsl.h"
#include "core/event.h"
#include "core/image.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <utility>
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

    bool initialize(GLFWwindow* window) {
        installInputCallbacks(window);
        return true;
    }

    template <typename ComposeFn>
    void compose(const std::string& pageId, float logicalWidth, float logicalHeight, ComposeFn&& composeFn) {
        const Screen screen{logicalWidth, logicalHeight};
        ui_.begin(pageId);
        ui_.setFocusedId(focusedId_);
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
        const auto inputEvents = consumeInputEvents();
        const KeyboardEvent& keyboardEvent = inputEvents.first;
        const ScrollEvent& scrollEvent = inputEvents.second;
        animating_ = false;
        needsCompose_ = false;
        wantsHandCursor_ = false;
        if (ImagePrimitive::consumeRemoteImageReady()) {
            fullRedraw_ = true;
            needsRender_ = true;
        }

        if (event.pressedThisFrame) {
            setFocusedId(hitTestFocusable(event, dpiScale));
        }

        const std::string capturedId = capturedInteractionId();
        const std::string hoverTargetId = !capturedId.empty() ? capturedId : hitTestInteractive(event, dpiScale);
        updateElementTree(event, deltaSeconds, dpiScale, hoverTargetId);

        if (scrollEvent.active()) {
            updateScroll(scrollEvent, hitTestScrollable(event, dpiScale));
        }
        if (keyboardEvent.hasInput()) {
            updateTextInput(keyboardEvent);
        }
        applyCursor(window);

        promoteBackdropBlurDirtyRegions();

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
            glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            glClear(GL_COLOR_BUFFER_BIT);
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
        const std::vector<const Element*> roots = orderedElements(ui_.roots());
        for (const Element* root : roots) {
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
        for (auto& item : images_) {
            if (item.second.initialized) {
                item.second.primitive->destroy();
            }
        }
        rects_.clear();
        texts_.clear();
        images_.clear();
        interactions_.clear();
        frameTargets_.clear();
        ImagePrimitive::releaseCachedTextures();
        releaseRenderCache();
        destroyCursors();
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

    struct ImageInstance {
        std::unique_ptr<ImagePrimitive> primitive = std::make_unique<ImagePrimitive>();
        bool initialized = false;
        AnimatedValue<LayoutRect> frame;
        AnimatedValue<Color> tint;
        AnimatedValue<float> radius;
        AnimatedValue<float> opacity;
        AnimatedValue<Transform> transform;
        std::string source;
        bool flipVertically = false;
        ImageFit fit = ImageFit::Cover;
    };

    struct InteractionInstance {
        InteractionState state;
    };

    struct LogicalDirtyRect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    template <typename Fn>
    void forEachElement(Fn&& fn) const {
        const std::vector<const Element*> roots = orderedElements(ui_.roots());
        for (const Element* root : roots) {
            forEachElement(*root, fn);
        }
    }

    template <typename Fn>
    static void forEachElement(const Element& element, Fn&& fn) {
        fn(element);
        const std::vector<const Element*> children = orderedElements(element.children);
        for (const Element* child : children) {
            forEachElement(*child, fn);
        }
    }

    static std::vector<const Element*> orderedElements(const std::vector<std::unique_ptr<Element>>& elements) {
        std::vector<const Element*> ordered;
        ordered.reserve(elements.size());
        for (const auto& element : elements) {
            ordered.push_back(element.get());
        }
        std::stable_sort(ordered.begin(), ordered.end(), [](const Element* a, const Element* b) {
            return a->zIndex < b->zIndex;
        });
        return ordered;
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

    static bool intersectRect(const Rect& a, const Rect& b, Rect& out) {
        const float left = std::max(a.x, b.x);
        const float top = std::max(a.y, b.y);
        const float right = std::min(a.x + a.width, b.x + b.width);
        const float bottom = std::min(a.y + a.height, b.y + b.height);
        if (right <= left || bottom <= top) {
            out = {};
            return false;
        }
        out = {left, top, right - left, bottom - top};
        return true;
    }

    static Rect inflateRect(Rect rect, float amount) {
        rect.x -= amount;
        rect.y -= amount;
        rect.width += amount * 2.0f;
        rect.height += amount * 2.0f;
        return rect;
    }

    static Rect scaleRectFromCenter(Rect rect, float scale) {
        const float centerX = rect.x + rect.width * 0.5f;
        const float centerY = rect.y + rect.height * 0.5f;
        rect.width *= scale;
        rect.height *= scale;
        rect.x = centerX - rect.width * 0.5f;
        rect.y = centerY - rect.height * 0.5f;
        return rect;
    }

    static bool isIdentityTransform(const Transform& transform) {
        return closeEnough(transform.translate, Vec2{}) &&
               closeEnough(transform.scale, Vec2{1.0f, 1.0f}) &&
               closeEnough(transform.rotate, 0.0f);
    }

    static Vec2 transformPoint(Vec2 point, const LayoutRect& frame, const Transform& transform) {
        const Vec2 origin = {
            frame.x + frame.width * transform.origin.x,
            frame.y + frame.height * transform.origin.y
        };
        const float scaledX = (point.x - origin.x) * transform.scale.x;
        const float scaledY = (point.y - origin.y) * transform.scale.y;
        const float cosine = std::cos(transform.rotate);
        const float sine = std::sin(transform.rotate);
        return {
            origin.x + scaledX * cosine - scaledY * sine + transform.translate.x,
            origin.y + scaledX * sine + scaledY * cosine + transform.translate.y
        };
    }

    static Rect transformRect(const Rect& rect, const LayoutRect& frame, const Transform& transform) {
        if (isIdentityTransform(transform)) {
            return rect;
        }

        const Vec2 p0 = transformPoint({rect.x, rect.y}, frame, transform);
        const Vec2 p1 = transformPoint({rect.x + rect.width, rect.y}, frame, transform);
        const Vec2 p2 = transformPoint({rect.x + rect.width, rect.y + rect.height}, frame, transform);
        const Vec2 p3 = transformPoint({rect.x, rect.y + rect.height}, frame, transform);
        const float left = std::min(std::min(p0.x, p1.x), std::min(p2.x, p3.x));
        const float top = std::min(std::min(p0.y, p1.y), std::min(p2.y, p3.y));
        const float right = std::max(std::max(p0.x, p1.x), std::max(p2.x, p3.x));
        const float bottom = std::max(std::max(p0.y, p1.y), std::max(p2.y, p3.y));
        return {left, top, right - left, bottom - top};
    }

    static Rect visualRect(const LayoutRect& frame, const Shadow& shadow, float blur, const Transform& transform = {}) {
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
        return transformRect(rect, frame, transform);
    }

    static Rect imageVisualRect(const LayoutRect& frame, const Transform& transform = {}) {
        return transformRect({frame.x, frame.y, frame.width, frame.height}, frame, transform);
    }

    void applyCursor(GLFWwindow* window) {
        if (!arrowCursor_) {
            arrowCursor_ = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        }
        if (!handCursor_) {
            handCursor_ = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        }

        GLFWcursor* target = wantsHandCursor_ && handCursor_ ? handCursor_ : arrowCursor_;
        if (target != currentCursor_) {
            glfwSetCursor(window, target);
            currentCursor_ = target;
        }
    }

    void destroyCursors() {
        if (arrowCursor_) {
            glfwDestroyCursor(arrowCursor_);
            arrowCursor_ = nullptr;
        }
        if (handCursor_) {
            glfwDestroyCursor(handCursor_);
            handCursor_ = nullptr;
        }
        currentCursor_ = nullptr;
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

    void promoteBackdropBlurDirtyRegions() {
        if (fullRedraw_ || dirtyRects_.empty()) {
            return;
        }

        bool dirtyTouchesBackdropBlur = false;
        forEachElement([&](const Element& element) {
            if (dirtyTouchesBackdropBlur || element.kind != ElementKind::Rect) {
                return;
            }

            const RectInstance& instance = rectInstance(element.id);
            const float blur = std::max(element.blur, instance.blur.value());
            if (blur <= 0.0f) {
                return;
            }

            const LayoutRect frame = instance.frame.value();
            const Rect captureRect = scaleRectFromCenter(
                transformRect({frame.x, frame.y, frame.width, frame.height}, frame, instance.transform.value()),
                1.2f);

            for (const LogicalDirtyRect& dirty : dirtyRects_) {
                const Rect dirtyRect{dirty.x, dirty.y, dirty.width, dirty.height};
                if (intersects(captureRect, dirtyRect)) {
                    dirtyTouchesBackdropBlur = true;
                    return;
                }
            }
        });

        if (dirtyTouchesBackdropBlur) {
            fullRedraw_ = true;
            needsRender_ = true;
        }
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
        const bool interactive = element.interactive && !element.disabled;
        return instance.hoverBlend.isMovingTo(interactive && instance.interaction.hover ? 1.0f : 0.0f) ||
               instance.pressBlend.isMovingTo(interactive && instance.interaction.pressed ? 1.0f : 0.0f) ||
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

    static bool isImageAnimating(const ImageInstance& instance) {
        return instance.frame.isActive() ||
               instance.tint.isActive() ||
               instance.radius.isActive() ||
               instance.opacity.isActive() ||
               instance.transform.isActive() ||
               instance.primitive->hasPendingLoad();
    }

    static bool shouldAnimate(const Element& element, AnimProperty property) {
        return element.transition.enabled && hasAnimProperty(element.transition.properties, property);
    }

    static bool shouldAnimateFrame(const Element& element) {
        return element.transition.enabled &&
               hasAnimProperty(element.transition.properties, AnimProperty::Frame) &&
               element.explicitFrameAnimation;
    }

    bool updateFrameTarget(const Element& element) {
        auto item = frameTargets_.find(element.id);
        if (item == frameTargets_.end()) {
            frameTargets_.emplace(element.id, element.frame);
            return false;
        }

        const bool changed = !closeEnough(item->second, element.frame);
        item->second = element.frame;
        return changed;
    }

    void updateElementTree(const PointerEvent& event,
                           float deltaSeconds,
                           float dpiScale,
                           const std::string& hoverTargetId) {
        const std::vector<const Element*> roots = orderedElements(ui_.roots());
        for (const Element* root : roots) {
            updateElementTree(*root, event, deltaSeconds, dpiScale, hoverTargetId, false);
        }
    }

    void updateElementTree(const Element& element,
                           const PointerEvent& event,
                           float deltaSeconds,
                           float dpiScale,
                           const std::string& hoverTargetId,
                           bool ancestorFrameChanged) {
        const bool frameTargetChanged = updateFrameTarget(element);
        updateInteraction(element, event, dpiScale, hoverTargetId);

        if (element.kind == ElementKind::Rect) {
            updateRect(element, deltaSeconds, dpiScale, ancestorFrameChanged);
        } else if (element.kind == ElementKind::Text) {
            updateText(element, deltaSeconds, ancestorFrameChanged);
        } else if (element.kind == ElementKind::Image) {
            updateImage(element, deltaSeconds, ancestorFrameChanged);
        }

        const bool childAncestorFrameChanged = ancestorFrameChanged || frameTargetChanged;
        const std::vector<const Element*> children = orderedElements(element.children);
        for (const Element* child : children) {
            updateElementTree(*child, event, deltaSeconds, dpiScale, hoverTargetId, childAncestorFrameChanged);
        }
    }

    RectInstance& rectInstance(const std::string& id) {
        return rects_.try_emplace(id).first->second;
    }

    TextInstance& textInstance(const std::string& id) {
        return texts_.try_emplace(id).first->second;
    }

    ImageInstance& imageInstance(const std::string& id) {
        return images_.try_emplace(id).first->second;
    }

    InteractionInstance& interactionInstance(const std::string& id) {
        return interactions_.try_emplace(id).first->second;
    }

    std::string capturedInteractionId() const {
        for (const auto& item : interactions_) {
            if (item.second.state.active && ui_.find(item.first)) {
                return item.first;
            }
        }
        return {};
    }

    std::string hitTestInteractive(const PointerEvent& event, float dpiScale) const {
        return hitTest(event, dpiScale, [](const Element& element) {
            return element.interactive && !element.disabled;
        });
    }

    std::string hitTestFocusable(const PointerEvent& event, float dpiScale) const {
        return hitTest(event, dpiScale, [](const Element& element) {
            return element.focusable && !element.disabled;
        });
    }

    std::string hitTestScrollable(const PointerEvent& event, float dpiScale) const {
        return hitTest(event, dpiScale, [](const Element& element) {
            return static_cast<bool>(element.onScroll) && !element.disabled;
        });
    }

    template <typename Predicate>
    std::string hitTest(const PointerEvent& event, float dpiScale, Predicate&& predicate) const {
        std::string targetId;
        const std::vector<const Element*> roots = orderedElements(ui_.roots());
        for (const Element* root : roots) {
            hitTestElement(*root, event, dpiScale, predicate, false, {}, targetId);
        }
        return targetId;
    }

    template <typename Predicate>
    void hitTestElement(const Element& element,
                        const PointerEvent& event,
                        float dpiScale,
                        Predicate& predicate,
                        bool hasClip,
                        const Rect& clipRect,
                        std::string& targetId) const {
        Rect effectiveClip = clipRect;
        bool effectiveHasClip = hasClip;
        const Rect bounds = toPixelRect(element.frame, dpiScale);
        if (element.clip) {
            if (effectiveHasClip) {
                if (!intersectRect(effectiveClip, bounds, effectiveClip)) {
                    return;
                }
            } else {
                effectiveClip = bounds;
                effectiveHasClip = true;
            }
        }

        if (effectiveHasClip && !effectiveClip.contains(event.x, event.y)) {
            return;
        }

        if (predicate(element) && bounds.contains(event.x, event.y)) {
            targetId = element.id;
        }

        const std::vector<const Element*> children = orderedElements(element.children);
        for (const Element* child : children) {
            hitTestElement(*child, event, dpiScale, predicate, effectiveHasClip, effectiveClip, targetId);
        }
    }

    void setFocusedId(const std::string& id) {
        if (focusedId_ == id) {
            return;
        }

        const std::string oldId = focusedId_;
        focusedId_ = id;
        ui_.setFocusedId(focusedId_);

        if (const Element* oldElement = ui_.find(oldId)) {
            if (oldElement->onFocusChanged) {
                oldElement->onFocusChanged(false);
            }
        }
        if (const Element* newElement = ui_.find(focusedId_)) {
            if (newElement->onFocusChanged) {
                newElement->onFocusChanged(true);
            }
        }
        needsCompose_ = true;
        needsRender_ = true;
    }

    void updateScroll(const ScrollEvent& event, const std::string& targetId) {
        if (targetId.empty()) {
            return;
        }

        if (const Element* element = ui_.find(targetId)) {
            if (element->onScroll && !element->disabled) {
                element->onScroll(event);
                needsCompose_ = true;
                needsRender_ = true;
            }
        }
    }

    void updateTextInput(const KeyboardEvent& event) {
        if (focusedId_.empty()) {
            return;
        }

        if (const Element* element = ui_.find(focusedId_)) {
            if (element->onTextInput && !element->disabled) {
                element->onTextInput(event);
                needsCompose_ = true;
                needsRender_ = true;
            }
        }
    }

    void updateInteraction(const Element& element, const PointerEvent& event, float dpiScale, const std::string& hoverTargetId) {
        if (!element.interactive && interactions_.find(element.id) == interactions_.end()) {
            return;
        }

        InteractionInstance& instance = interactionInstance(element.id);
        const Rect bounds = toPixelRect(element.frame, dpiScale);
        const bool enabled = element.interactive && !element.disabled;
        const bool topmostHover = enabled && element.id == hoverTargetId;
        instance.state.update(bounds, event, topmostHover, enabled);

        if (enabled && instance.state.hover && element.cursor == CursorShape::Hand) {
            wantsHandCursor_ = true;
        }

        if (enabled && instance.state.pressStarted && element.onPress) {
            element.onPress(event, bounds);
            needsCompose_ = true;
            needsRender_ = true;
        }

        if (enabled && instance.state.clicked && element.onClick) {
            element.onClick();
            needsCompose_ = true;
        }

        if (enabled && instance.state.pressed && element.onDrag &&
            (event.deltaX != 0.0 || event.deltaY != 0.0 || instance.state.drag)) {
            element.onDrag({
                event.x,
                event.y,
                event.deltaX,
                event.deltaY,
                instance.state.dragDeltaX,
                instance.state.dragDeltaY
            });
            needsCompose_ = true;
            needsRender_ = true;
        }
    }

    void updateRect(const Element& element, float deltaSeconds, float dpiScale, bool snapFrame) {
        RectInstance& instance = rectInstance(element.id);
        instance.interaction = interactionInstance(element.id).state;
        const Rect beforeRect = visualRect(instance.frame.value(), instance.shadow.value(), instance.blur.value(), instance.transform.value());

        const bool interactive = element.interactive && !element.disabled;
        const bool hoverChanged = instance.hoverBlend.update(interactive && instance.interaction.hover ? 1.0f : 0.0f, 9.0f, deltaSeconds);
        const bool pressChanged = instance.pressBlend.update(interactive && instance.interaction.pressed ? 1.0f : 0.0f, 16.0f, deltaSeconds);
        const float hover = instance.hoverBlend.value();
        const float press = instance.pressBlend.value();
        const Color hoverColor = mixColor(element.color, element.hoverColor, hover);
        const Color currentColor = mixColor(hoverColor, element.pressedColor, press);

        bool changed = hoverChanged || pressChanged || (interactive && instance.interaction.changed);
        changed = instance.frame.setTarget(element.frame, element.transition, !snapFrame && shouldAnimateFrame(element)) || changed;
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
            const Rect afterRect = visualRect(instance.frame.value(), instance.shadow.value(), instance.blur.value(), instance.transform.value());
            addDirtyUnion(beforeRect, afterRect);
        }
        animating_ = animating_ || isRectAnimating(element, instance);
    }

    void updateText(const Element& element, float deltaSeconds, bool snapFrame) {
        TextInstance& instance = textInstance(element.id);
        const Rect beforeRect{instance.frame.value().x, instance.frame.value().y, instance.frame.value().width, instance.frame.value().height};

        bool changed = false;
        changed = instance.frame.setTarget(element.frame, element.transition, !snapFrame && shouldAnimateFrame(element)) || changed;
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

    void updateImage(const Element& element, float deltaSeconds, bool snapFrame) {
        ImageInstance& instance = imageInstance(element.id);
        const Rect beforeRect = imageVisualRect(instance.frame.value(), instance.transform.value());

        bool changed = false;
        changed = instance.frame.setTarget(element.frame, element.transition, !snapFrame && shouldAnimateFrame(element)) || changed;
        changed = instance.tint.setTarget(element.color, element.transition, shouldAnimate(element, AnimProperty::Color)) || changed;
        changed = instance.radius.setTarget(element.radius, element.transition, shouldAnimate(element, AnimProperty::Radius)) || changed;
        changed = instance.opacity.setTarget(element.opacity, element.transition, shouldAnimate(element, AnimProperty::Opacity)) || changed;
        changed = instance.transform.setTarget(element.transform, element.transition, shouldAnimate(element, AnimProperty::Transform)) || changed;

        changed = instance.frame.tick(deltaSeconds) || changed;
        changed = instance.tint.tick(deltaSeconds) || changed;
        changed = instance.radius.tick(deltaSeconds) || changed;
        changed = instance.opacity.tick(deltaSeconds) || changed;
        changed = instance.transform.tick(deltaSeconds) || changed;

        const bool sourceChanged = instance.source != element.imageSource ||
                                   instance.flipVertically != element.imageFlipVertically ||
                                   instance.fit != element.imageFit;
        if (sourceChanged) {
            instance.source = element.imageSource;
            instance.flipVertically = element.imageFlipVertically;
            instance.fit = element.imageFit;
            instance.primitive->setSource(instance.source);
            instance.primitive->setFlipVertically(instance.flipVertically);
            instance.primitive->setFit(instance.fit);
            changed = true;
        }

        const LayoutRect frame = instance.frame.value();
        instance.primitive->setBounds(frame.x, frame.y, frame.width, frame.height);
        if (instance.primitive->updateTexture()) {
            changed = true;
        }

        if (changed) {
            const Rect afterRect = imageVisualRect(instance.frame.value(), instance.transform.value());
            addDirtyUnion(beforeRect, afterRect);
        }
        animating_ = animating_ || isImageAnimating(instance);
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

    static void applyOptionalScissor(bool enabled, const Rect& rect, int windowHeight) {
        if (!enabled) {
            glDisable(GL_SCISSOR_TEST);
            return;
        }
        glEnable(GL_SCISSOR_TEST);
        applyScissor(rect, windowHeight);
    }

    void blitRenderCache(int windowWidth, int windowHeight) {
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, cacheFramebuffer_);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, windowWidth, windowHeight,
                          0, 0, windowWidth, windowHeight,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderDirect(int windowWidth, int windowHeight, float dpiScale, const Rect* dirtyRect = nullptr) {
        const RenderTransform identity;
        const bool hasScissor = dirtyRect != nullptr;
        const Rect scissor = dirtyRect ? *dirtyRect : Rect{};
        const std::vector<const Element*> roots = orderedElements(ui_.roots());
        for (const Element* root : roots) {
            renderElement(*root, windowWidth, windowHeight, dpiScale, identity, dirtyRect, hasScissor, scissor);
        }
    }

    void renderElement(const Element& element,
                       int windowWidth,
                       int windowHeight,
                       float dpiScale,
                       const RenderTransform& inheritedTransform,
                       const Rect* dirtyRect = nullptr,
                       bool hasScissor = false,
                       const Rect& scissorRect = {}) {
        const RenderTransform renderTransform = resolveRenderTransform(element, dpiScale, inheritedTransform);
        Rect effectiveScissor = scissorRect;
        bool effectiveHasScissor = hasScissor;
        if (element.clip) {
            Rect clipFrame = applyRenderTransform(toPixelRect(element.frame, dpiScale), renderTransform);
            if (effectiveHasScissor) {
                if (!intersectRect(effectiveScissor, clipFrame, effectiveScissor)) {
                    return;
                }
            } else {
                effectiveScissor = clipFrame;
                effectiveHasScissor = true;
            }
        }

        if (element.kind == ElementKind::Rect) {
            Rect visual = toPixelRect(visualRect(rectInstance(element.id).frame.value(),
                                                rectInstance(element.id).shadow.value(),
                                                rectInstance(element.id).blur.value(),
                                                rectInstance(element.id).transform.value()), dpiScale);
            visual = applyRenderTransform(visual, renderTransform);
            if ((!dirtyRect || intersects(visual, *dirtyRect)) &&
                (!effectiveHasScissor || intersects(visual, effectiveScissor))) {
                applyOptionalScissor(effectiveHasScissor, effectiveScissor, windowHeight);
                renderRect(element, windowWidth, windowHeight, dpiScale, renderTransform);
            }
        } else if (element.kind == ElementKind::Text) {
            Rect frame = applyRenderTransform(toPixelRect(textInstance(element.id).frame.value(), dpiScale), renderTransform);
            if ((!dirtyRect || intersects(frame, *dirtyRect)) &&
                (!effectiveHasScissor || intersects(frame, effectiveScissor))) {
                applyOptionalScissor(effectiveHasScissor, effectiveScissor, windowHeight);
                renderText(element, windowWidth, windowHeight, dpiScale, renderTransform);
            }
        } else if (element.kind == ElementKind::Image) {
            Rect visual = toPixelRect(imageVisualRect(imageInstance(element.id).frame.value(),
                                                     imageInstance(element.id).transform.value()), dpiScale);
            visual = applyRenderTransform(visual, renderTransform);
            if ((!dirtyRect || intersects(visual, *dirtyRect)) &&
                (!effectiveHasScissor || intersects(visual, effectiveScissor))) {
                applyOptionalScissor(effectiveHasScissor, effectiveScissor, windowHeight);
                renderImage(element, windowWidth, windowHeight, dpiScale, renderTransform);
            }
        }

        const std::vector<const Element*> children = orderedElements(element.children);
        for (const Element* child : children) {
            renderElement(*child, windowWidth, windowHeight, dpiScale, renderTransform, dirtyRect, effectiveHasScissor, effectiveScissor);
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

    void renderImage(const Element& element,
                     int windowWidth,
                     int windowHeight,
                     float dpiScale,
                     const RenderTransform& renderTransform) {
        ImageInstance& instance = imageInstance(element.id);
        if (!instance.initialized) {
            instance.initialized = instance.primitive->initialize();
            if (!instance.initialized) {
                return;
            }
        }

        const Rect frame = toPixelRect(instance.frame.value(), dpiScale);
        const float visualScale = renderTransform.active ? renderTransform.scale : 1.0f;
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
        instance.primitive->setTint(instance.tint.value());
        instance.primitive->setCornerRadius(toPixels(instance.radius.value(), dpiScale));
        instance.primitive->setOpacity(instance.opacity.value());
        instance.primitive->setTransform(transform);
        instance.primitive->setFit(instance.fit);
        instance.primitive->render(windowWidth, windowHeight);
    }

    Ui ui_;
    std::unordered_map<std::string, RectInstance> rects_;
    std::unordered_map<std::string, TextInstance> texts_;
    std::unordered_map<std::string, ImageInstance> images_;
    std::unordered_map<std::string, InteractionInstance> interactions_;
    std::unordered_map<std::string, LayoutRect> frameTargets_;
    std::vector<LogicalDirtyRect> dirtyRects_;
    bool needsRender_ = true;
    bool animating_ = false;
    bool needsCompose_ = false;
    bool fullRedraw_ = true;
    bool wantsHandCursor_ = false;
    std::string focusedId_;
    float logicalWidth_ = 0.0f;
    float logicalHeight_ = 0.0f;
    GLuint cacheFramebuffer_ = 0;
    GLuint cacheTexture_ = 0;
    int cacheWidth_ = 0;
    int cacheHeight_ = 0;
    GLFWcursor* arrowCursor_ = nullptr;
    GLFWcursor* handCursor_ = nullptr;
    GLFWcursor* currentCursor_ = nullptr;
};

} // namespace core::dsl
