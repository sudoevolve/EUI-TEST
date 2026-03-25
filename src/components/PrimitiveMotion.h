#pragma once

#include "../ui/UIPrimitive.h"
#include <algorithm>
#include <cmath>

namespace EUINEO {

struct ScalarMotionSpec {
    bool enabled = false;
    float from = 0.0f;
    float to = 0.0f;
    float duration = 0.0f;
    Easing easing = Easing::EaseInOut;
    bool loop = true;
    bool pingPong = true;
};

struct ColorMotionSpec {
    bool enabled = false;
    Color from;
    Color to;
    float duration = 0.0f;
    Easing easing = Easing::EaseInOut;
    bool loop = true;
    bool pingPong = true;
};

struct HoverScalarMotionSpec {
    bool enabled = false;
    float idle = 0.0f;
    float hover = 0.0f;
    float duration = 0.0f;
    Easing easing = Easing::EaseInOut;
};

struct HoverColorMotionSpec {
    bool enabled = false;
    Color idle;
    Color hover;
    float duration = 0.0f;
    Easing easing = Easing::EaseInOut;
};

inline bool MotionEq(const ScalarMotionSpec& lhs, const ScalarMotionSpec& rhs, float epsilon = 0.0001f) {
    return lhs.enabled == rhs.enabled &&
           std::abs(lhs.from - rhs.from) <= epsilon &&
           std::abs(lhs.to - rhs.to) <= epsilon &&
           std::abs(lhs.duration - rhs.duration) <= epsilon &&
           lhs.easing == rhs.easing &&
           lhs.loop == rhs.loop &&
           lhs.pingPong == rhs.pingPong;
}

inline bool MotionEq(const ColorMotionSpec& lhs, const ColorMotionSpec& rhs, float epsilon = 0.0001f) {
    return lhs.enabled == rhs.enabled &&
           std::abs(lhs.from.r - rhs.from.r) <= epsilon &&
           std::abs(lhs.from.g - rhs.from.g) <= epsilon &&
           std::abs(lhs.from.b - rhs.from.b) <= epsilon &&
           std::abs(lhs.from.a - rhs.from.a) <= epsilon &&
           std::abs(lhs.to.r - rhs.to.r) <= epsilon &&
           std::abs(lhs.to.g - rhs.to.g) <= epsilon &&
           std::abs(lhs.to.b - rhs.to.b) <= epsilon &&
           std::abs(lhs.to.a - rhs.to.a) <= epsilon &&
           std::abs(lhs.duration - rhs.duration) <= epsilon &&
           lhs.easing == rhs.easing &&
           lhs.loop == rhs.loop &&
           lhs.pingPong == rhs.pingPong;
}

inline bool MotionEq(const HoverScalarMotionSpec& lhs, const HoverScalarMotionSpec& rhs, float epsilon = 0.0001f) {
    return lhs.enabled == rhs.enabled &&
           std::abs(lhs.idle - rhs.idle) <= epsilon &&
           std::abs(lhs.hover - rhs.hover) <= epsilon &&
           std::abs(lhs.duration - rhs.duration) <= epsilon &&
           lhs.easing == rhs.easing;
}

inline bool MotionEq(const HoverColorMotionSpec& lhs, const HoverColorMotionSpec& rhs, float epsilon = 0.0001f) {
    return lhs.enabled == rhs.enabled &&
           std::abs(lhs.idle.r - rhs.idle.r) <= epsilon &&
           std::abs(lhs.idle.g - rhs.idle.g) <= epsilon &&
           std::abs(lhs.idle.b - rhs.idle.b) <= epsilon &&
           std::abs(lhs.idle.a - rhs.idle.a) <= epsilon &&
           std::abs(lhs.hover.r - rhs.hover.r) <= epsilon &&
           std::abs(lhs.hover.g - rhs.hover.g) <= epsilon &&
           std::abs(lhs.hover.b - rhs.hover.b) <= epsilon &&
           std::abs(lhs.hover.a - rhs.hover.a) <= epsilon &&
           std::abs(lhs.duration - rhs.duration) <= epsilon &&
           lhs.easing == rhs.easing;
}

class PrimitiveMotionState {
public:
    PrimitiveMotionState() {
        scaleAnimation_.Bind(&scaleValue_);
        rotationAnimation_.Bind(&rotationValue_);
        opacityAnimation_.Bind(&opacityValue_);
        translateXAnimation_.Bind(&translateXValue_);
        translateYAnimation_.Bind(&translateYValue_);
        backgroundAnimation_.Bind(&backgroundValue_);
    }

    bool wantsContinuousUpdate(const ScalarMotionSpec& scaleSpec, const HoverScalarMotionSpec& hoverScaleSpec,
                               const ScalarMotionSpec& rotationSpec, const HoverScalarMotionSpec& hoverRotationSpec,
                               const ScalarMotionSpec& opacitySpec, const HoverScalarMotionSpec& hoverOpacitySpec,
                               const ScalarMotionSpec& translateXSpec, const HoverScalarMotionSpec& hoverTranslateXSpec,
                               const ScalarMotionSpec& translateYSpec, const HoverScalarMotionSpec& hoverTranslateYSpec,
                               const ColorMotionSpec& backgroundSpec, const HoverColorMotionSpec& hoverBackgroundSpec) const {
        return WantsTrack(scaleSpec, appliedScaleSpec_, hoverScaleSpec, appliedHoverScaleSpec_, scaleAnimation_) ||
               WantsTrack(rotationSpec, appliedRotationSpec_, hoverRotationSpec, appliedHoverRotationSpec_, rotationAnimation_) ||
               WantsTrack(opacitySpec, appliedOpacitySpec_, hoverOpacitySpec, appliedHoverOpacitySpec_, opacityAnimation_) ||
               WantsTrack(translateXSpec, appliedTranslateXSpec_, hoverTranslateXSpec, appliedHoverTranslateXSpec_, translateXAnimation_) ||
               WantsTrack(translateYSpec, appliedTranslateYSpec_, hoverTranslateYSpec, appliedHoverTranslateYSpec_, translateYAnimation_) ||
               WantsTrack(backgroundSpec, appliedBackgroundSpec_, hoverBackgroundSpec, appliedHoverBackgroundSpec_, backgroundAnimation_);
    }

    bool update(float deltaTime, bool hovered,
                const ScalarMotionSpec& scaleSpec, const HoverScalarMotionSpec& hoverScaleSpec,
                const ScalarMotionSpec& rotationSpec, const HoverScalarMotionSpec& hoverRotationSpec,
                const ScalarMotionSpec& opacitySpec, const HoverScalarMotionSpec& hoverOpacitySpec,
                const ScalarMotionSpec& translateXSpec, const HoverScalarMotionSpec& hoverTranslateXSpec,
                const ScalarMotionSpec& translateYSpec, const HoverScalarMotionSpec& hoverTranslateYSpec,
                const ColorMotionSpec& backgroundSpec, const HoverColorMotionSpec& hoverBackgroundSpec) {
        bool changed = false;
        changed |= UpdateTrack(deltaTime, hovered, scaleSpec, appliedScaleSpec_, hoverScaleSpec, appliedHoverScaleSpec_, hoverScaleActive_, scaleValue_, scaleAnimation_);
        changed |= UpdateTrack(deltaTime, hovered, rotationSpec, appliedRotationSpec_, hoverRotationSpec, appliedHoverRotationSpec_, hoverRotationActive_, rotationValue_, rotationAnimation_);
        changed |= UpdateTrack(deltaTime, hovered, opacitySpec, appliedOpacitySpec_, hoverOpacitySpec, appliedHoverOpacitySpec_, hoverOpacityActive_, opacityValue_, opacityAnimation_);
        changed |= UpdateTrack(deltaTime, hovered, translateXSpec, appliedTranslateXSpec_, hoverTranslateXSpec, appliedHoverTranslateXSpec_, hoverTranslateXActive_, translateXValue_, translateXAnimation_);
        changed |= UpdateTrack(deltaTime, hovered, translateYSpec, appliedTranslateYSpec_, hoverTranslateYSpec, appliedHoverTranslateYSpec_, hoverTranslateYActive_, translateYValue_, translateYAnimation_);
        changed |= UpdateTrack(deltaTime, hovered, backgroundSpec, appliedBackgroundSpec_, hoverBackgroundSpec, appliedHoverBackgroundSpec_, hoverBackgroundActive_, backgroundValue_, backgroundAnimation_);
        return changed;
    }

    void apply(UIPrimitive& primitive,
               const ScalarMotionSpec& scaleSpec, const HoverScalarMotionSpec& hoverScaleSpec,
               const ScalarMotionSpec& rotationSpec, const HoverScalarMotionSpec& hoverRotationSpec,
               const ScalarMotionSpec& opacitySpec, const HoverScalarMotionSpec& hoverOpacitySpec,
               const ScalarMotionSpec& translateXSpec, const HoverScalarMotionSpec& hoverTranslateXSpec,
               const ScalarMotionSpec& translateYSpec, const HoverScalarMotionSpec& hoverTranslateYSpec,
               const ColorMotionSpec& backgroundSpec, const HoverColorMotionSpec& hoverBackgroundSpec) const {
        if (hoverScaleSpec.enabled || scaleSpec.enabled) {
            primitive.scaleX = scaleValue_;
            primitive.scaleY = scaleValue_;
        }
        if (hoverRotationSpec.enabled || rotationSpec.enabled) {
            primitive.rotation = rotationValue_;
        }
        if (hoverOpacitySpec.enabled || opacitySpec.enabled) {
            primitive.opacity = std::clamp(opacityValue_, 0.0f, 1.0f);
        }
        if (hoverTranslateXSpec.enabled || translateXSpec.enabled) {
            primitive.translateX = translateXValue_;
        }
        if (hoverTranslateYSpec.enabled || translateYSpec.enabled) {
            primitive.translateY = translateYValue_;
        }
        if (hoverBackgroundSpec.enabled || backgroundSpec.enabled) {
            primitive.background = backgroundValue_;
        }
    }

private:
    template <typename LoopSpecT, typename HoverSpecT, typename AnimationT>
    static bool WantsTrack(const LoopSpecT& loopSpec, const LoopSpecT& appliedLoopSpec,
                           const HoverSpecT& hoverSpec, const HoverSpecT& appliedHoverSpec,
                           const AnimationT& animation) {
        if (hoverSpec.enabled) {
            return !MotionEq(hoverSpec, appliedHoverSpec) || animation.IsActive();
        }
        if (!loopSpec.enabled) {
            return false;
        }
        return !MotionEq(loopSpec, appliedLoopSpec) || loopSpec.loop || animation.IsActive();
    }

    static float EffectiveDuration(float value) {
        return std::max(0.0001f, value);
    }

    template <typename SpecT, typename ValueT, typename AnimationT>
    static void StartTrack(const SpecT& spec, ValueT& value, AnimationT& animation) {
        value = spec.from;
        animation.Clear();
        animation.SetCurrent(spec.from);
        animation.Play(spec.from, spec.to, EffectiveDuration(spec.duration), spec.easing);
        if (spec.pingPong) {
            animation.Queue(spec.from, EffectiveDuration(spec.duration), spec.easing);
        }
    }

    template <typename HoverSpecT, typename ValueT, typename AnimationT>
    static bool UpdateHoverTrack(float deltaTime, bool hovered, const HoverSpecT& spec, HoverSpecT& appliedSpec,
                                 bool& hoverActive, ValueT& value, AnimationT& animation) {
        if (!spec.enabled) {
            const bool hadState = appliedSpec.enabled || animation.IsActive();
            if (hadState) {
                animation.Clear();
                appliedSpec = HoverSpecT{};
                hoverActive = false;
            }
            return hadState;
        }

        bool changed = false;
        if (!MotionEq(spec, appliedSpec)) {
            appliedSpec = spec;
            hoverActive = hovered;
            value = hovered ? spec.hover : spec.idle;
            animation.Clear();
            animation.SetCurrent(value);
            changed = true;
        }

        const ValueT target = hovered ? spec.hover : spec.idle;
        if (hovered != hoverActive) {
            hoverActive = hovered;
            animation.PlayTo(target, EffectiveDuration(spec.duration), spec.easing);
            changed = true;
        }

        if (animation.Update(deltaTime)) {
            changed = true;
        }

        return changed;
    }

    template <typename LoopSpecT, typename HoverSpecT, typename ValueT, typename AnimationT>
    static bool UpdateTrack(float deltaTime, bool hovered,
                            const LoopSpecT& loopSpec, LoopSpecT& appliedLoopSpec,
                            const HoverSpecT& hoverSpec, HoverSpecT& appliedHoverSpec, bool& hoverActive,
                            ValueT& value, AnimationT& animation) {
        if (hoverSpec.enabled) {
            const bool changed = UpdateHoverTrack(deltaTime, hovered, hoverSpec, appliedHoverSpec, hoverActive, value, animation);
            if (appliedLoopSpec.enabled) {
                appliedLoopSpec = LoopSpecT{};
            }
            return changed;
        }

        if (appliedHoverSpec.enabled) {
            appliedHoverSpec = HoverSpecT{};
            hoverActive = false;
        }

        if (!loopSpec.enabled) {
            const bool hadState = appliedLoopSpec.enabled || animation.IsActive() || appliedHoverSpec.enabled;
            if (hadState) {
                animation.Clear();
                appliedLoopSpec = LoopSpecT{};
                appliedHoverSpec = HoverSpecT{};
                hoverActive = false;
            }
            return hadState;
        }

        bool changed = false;
        if (!MotionEq(loopSpec, appliedLoopSpec)) {
            appliedLoopSpec = loopSpec;
            StartTrack(loopSpec, value, animation);
            changed = true;
        }

        if (animation.Update(deltaTime)) {
            changed = true;
        }

        if (!animation.IsActive() && loopSpec.loop) {
            StartTrack(loopSpec, value, animation);
            changed = true;
        }

        return changed;
    }

    ScalarMotionSpec appliedScaleSpec_;
    ScalarMotionSpec appliedRotationSpec_;
    ScalarMotionSpec appliedOpacitySpec_;
    ScalarMotionSpec appliedTranslateXSpec_;
    ScalarMotionSpec appliedTranslateYSpec_;
    ColorMotionSpec appliedBackgroundSpec_;
    HoverScalarMotionSpec appliedHoverScaleSpec_;
    HoverScalarMotionSpec appliedHoverRotationSpec_;
    HoverScalarMotionSpec appliedHoverOpacitySpec_;
    HoverScalarMotionSpec appliedHoverTranslateXSpec_;
    HoverScalarMotionSpec appliedHoverTranslateYSpec_;
    HoverColorMotionSpec appliedHoverBackgroundSpec_;

    float scaleValue_ = 1.0f;
    float rotationValue_ = 0.0f;
    float opacityValue_ = 1.0f;
    float translateXValue_ = 0.0f;
    float translateYValue_ = 0.0f;
    Color backgroundValue_ = Color(1.0f, 1.0f, 1.0f, 1.0f);
    bool hoverScaleActive_ = false;
    bool hoverRotationActive_ = false;
    bool hoverOpacityActive_ = false;
    bool hoverTranslateXActive_ = false;
    bool hoverTranslateYActive_ = false;
    bool hoverBackgroundActive_ = false;

    FloatAnimation scaleAnimation_;
    FloatAnimation rotationAnimation_;
    FloatAnimation opacityAnimation_;
    FloatAnimation translateXAnimation_;
    FloatAnimation translateYAnimation_;
    ColorAnimation backgroundAnimation_;
};

} // namespace EUINEO
