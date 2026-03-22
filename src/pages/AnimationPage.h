#pragma once

#include "../ui/UIContext.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace EUINEO {

class AnimationPage {
public:
    AnimationPage() {
        for (int index = 0; index < static_cast<int>(cards_.size()); ++index) {
            cards_[index].hoverAnimation.Bind(&cards_[index].hoverAmount);
            cards_[index].burstAnimation.Bind(&cards_[index].burstAmount);
        }
    }

    void Update(const RectFrame& bounds) {
        bounds_ = bounds;
        if (bounds_.width <= 0.0f || bounds_.height <= 0.0f) {
            return;
        }

        const Layout layout = MakeLayout();
        bool needsAnimatedRepaint = false;

        for (int index = 0; index < static_cast<int>(cards_.size()); ++index) {
            CardRuntime& card = cards_[index];
            const RectFrame frame = CardFrame(layout, index);
            const bool hovered = State.mouseX >= frame.x && State.mouseX <= frame.x + frame.width &&
                                 State.mouseY >= frame.y && State.mouseY <= frame.y + frame.height;

            if (hovered != card.hovered) {
                card.hovered = hovered;
                card.hoverAnimation.PlayTo(hovered ? 1.0f : 0.0f, 0.18f, hovered ? Easing::EaseOut : Easing::EaseInOut);
                Renderer::RequestRepaint(0.18f);
            }

            if (hovered && State.mouseClicked) {
                card.burstAnimation.PlayTo(1.0f, 0.10f, Easing::EaseOut);
                card.burstAnimation.Queue(0.0f, 0.18f, Easing::EaseInOut);
                Renderer::RequestRepaint(0.20f);
            }

            if (card.hoverAnimation.Update(State.deltaTime)) {
                needsAnimatedRepaint = true;
            }
            if (card.burstAnimation.Update(State.deltaTime)) {
                needsAnimatedRepaint = true;
            }
        }

        if (needsAnimatedRepaint) {
            Renderer::RequestRepaint(0.18f);
        }
    }

    void Compose(UIContext& ui, const std::string& idPrefix) const {
        if (bounds_.width <= 0.0f || bounds_.height <= 0.0f) {
            return;
        }

        const Layout layout = MakeLayout();
        const auto& specs = CardSpecs();
        const bool dark = CurrentTheme == &DarkTheme;

        ui.label(idPrefix + ".title")
            .text("Animation Page")
            .position(bounds_.x, bounds_.y + 24.0f)
            .fontSize(31.0f)
            .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.98f))
            .build();

        ui.label(idPrefix + ".subtitle")
            .text("Hover cards to preview rect property tracks. Click for a queued burst.")
            .position(bounds_.x, bounds_.y + 54.0f)
            .fontSize(17.0f)
            .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.72f))
            .build();

        for (int index = 0; index < static_cast<int>(cards_.size()); ++index) {
            const CardRuntime& card = cards_[index];
            const float hover = std::clamp(card.hoverAmount, 0.0f, 1.0f);
            const float burst = std::clamp(card.burstAmount, 0.0f, 1.0f);
            const RectFrame panelFrame = CardFrame(layout, index);
            const RectFrame sampleFrame = SampleFrame(panelFrame);
            const float panelLift = hover * -4.0f + burst * -3.0f;
            const float sampleLift = (index == 3 ? hover * -6.0f + burst * -4.0f : 0.0f);

            ui.panel(idPrefix + ".card" + std::to_string(index) + ".panel")
                .position(panelFrame.x, panelFrame.y)
                .size(panelFrame.width, panelFrame.height)
                .background(CurrentTheme->surface.r, CurrentTheme->surface.g, CurrentTheme->surface.b, 0.98f)
                .rounding(16.0f)
                .translateY(panelLift)
                .build();

            ui.label(idPrefix + ".card" + std::to_string(index) + ".title")
                .text(specs[index].title)
                .position(panelFrame.x + 24.0f, panelFrame.y + 40.0f + panelLift)
                .fontSize(23.0f)
                .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.96f))
                .build();

            ui.label(idPrefix + ".card" + std::to_string(index) + ".detail1")
                .text(specs[index].detailLine1)
                .position(panelFrame.x + 24.0f, panelFrame.y + 72.0f + panelLift)
                .fontSize(15.0f)
                .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.66f))
                .build();

            ui.label(idPrefix + ".card" + std::to_string(index) + ".detail2")
                .text(specs[index].detailLine2)
                .position(panelFrame.x + 24.0f, panelFrame.y + 90.0f + panelLift)
                .fontSize(15.0f)
                .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.66f))
                .build();

            const float badgeScale = 14.0f / 24.0f;
            const float badgeTextWidth = Renderer::MeasureTextWidth(specs[index].badge, badgeScale);
            const float badgeX = panelFrame.x + 24.0f;
            const float badgeY = panelFrame.y + panelFrame.height - 38.0f + panelLift;

            ui.panel(idPrefix + ".card" + std::to_string(index) + ".badgePanel")
                .position(badgeX, badgeY)
                .size(badgeTextWidth + 24.0f, 24.0f)
                .background(CurrentTheme->surfaceHover)
                .rounding(10.0f)
                .build();

            ui.label(idPrefix + ".card" + std::to_string(index) + ".badgeText")
                .text(specs[index].badge)
                .position(badgeX + 12.0f, badgeY + 16.5f)
                .fontSize(14.0f)
                .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.82f))
                .build();

            const SampleVisual sample = MakeSampleVisual(index, hover, burst, dark);
            ui.panel(idPrefix + ".card" + std::to_string(index) + ".sample")
                .position(sampleFrame.x, sampleFrame.y)
                .size(sampleFrame.width, sampleFrame.height)
                .background(sample.color)
                .gradient(sample.gradient)
                .rounding(12.0f)
                .scaleX(sample.scaleX)
                .scaleY(sample.scaleY)
                .translateY(panelLift + sampleLift)
                .rotation(sample.rotation)
                .build();
        }
    }

private:
    struct CardSpec {
        const char* title;
        const char* detailLine1;
        const char* detailLine2;
        const char* badge;
    };

    struct CardRuntime {
        bool hovered = false;
        float hoverAmount = 0.0f;
        float burstAmount = 0.0f;
        FloatAnimation hoverAnimation;
        FloatAnimation burstAnimation;
    };

    struct Layout {
        float gap = 18.0f;
        float topOffset = 98.0f;
        int columns = 1;
        float cardWidth = 0.0f;
        float cardHeight = 0.0f;
    };

    struct SampleVisual {
        Color color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        RectGradient gradient;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float rotation = 0.0f;
    };

    static const std::array<CardSpec, 4>& CardSpecs() {
        static const std::array<CardSpec, 4> specs{{
            {"Fade Alpha", "Use color.a to control", "the fill opacity.", "Color.a"},
            {"Uniform Scale", "Scale both axes together", "for lift and focus.", "scaleX = scaleY"},
            {"Axis Stretch", "Animate scaleX / scaleY", "for directional stretch.", "scaleX / scaleY"},
            {"Queue + Combo", "Blend move, rotate, scale", "and gradient in queue.", "PlayTo + Queue"},
        }};
        return specs;
    }

    Layout MakeLayout() const {
        Layout layout;
        layout.columns = bounds_.width >= 520.0f ? 2 : 1;
        layout.cardWidth = layout.columns == 2 ? (bounds_.width - layout.gap) * 0.5f : bounds_.width;

        const float availableHeight = std::max(240.0f, bounds_.height - layout.topOffset - layout.gap);
        layout.cardHeight = layout.columns == 2
            ? std::clamp((availableHeight - layout.gap) * 0.5f, 170.0f, 210.0f)
            : std::clamp((availableHeight - layout.gap * 3.0f) * 0.25f, 116.0f, 152.0f);
        return layout;
    }

    RectFrame CardFrame(const Layout& layout, int index) const {
        RectFrame frame;
        const int col = index % layout.columns;
        const int row = index / layout.columns;
        frame.x = bounds_.x + col * (layout.cardWidth + layout.gap);
        frame.y = bounds_.y + layout.topOffset + row * (layout.cardHeight + layout.gap);
        frame.width = layout.cardWidth;
        frame.height = layout.cardHeight;
        return frame;
    }

    static RectFrame SampleFrame(const RectFrame& panelFrame) {
        RectFrame frame;
        frame.width = std::min(84.0f, panelFrame.width * 0.24f);
        frame.height = std::min(56.0f, panelFrame.height * 0.30f);
        frame.x = panelFrame.x + panelFrame.width - frame.width - 26.0f;

        const float desiredY = panelFrame.y + panelFrame.height * 0.58f - frame.height * 0.5f;
        const float minY = panelFrame.y + panelFrame.height * 0.38f;
        const float maxY = panelFrame.y + panelFrame.height - frame.height - std::max(28.0f, panelFrame.height * 0.18f);
        frame.y = maxY >= minY ? std::clamp(desiredY, minY, maxY) : desiredY;
        return frame;
    }

    static SampleVisual MakeSampleVisual(int index, float hover, float burst, bool dark) {
        SampleVisual visual;
        visual.color = CurrentTheme->primary;

        if (index == 0) {
            const float baseAlpha = dark ? 0.36f : 0.48f;
            visual.color.a = Lerp(baseAlpha, 1.0f, std::clamp(hover + burst * 0.6f, 0.0f, 1.0f));
        } else if (index == 1) {
            visual.scaleX = 1.0f + hover * 0.18f + burst * 0.08f;
            visual.scaleY = 1.0f + hover * 0.18f + burst * 0.08f;
        } else if (index == 2) {
            visual.scaleX = 1.0f + hover * 0.26f + burst * 0.06f;
            visual.scaleY = 1.0f - hover * 0.22f - burst * 0.06f;
        } else {
            const float combo = std::clamp(hover + burst * 0.8f, 0.0f, 1.0f);
            visual.scaleX = 1.0f + hover * 0.10f + burst * 0.08f;
            visual.scaleY = 1.0f + hover * 0.10f + burst * 0.08f;
            visual.rotation = hover * 10.0f + burst * 8.0f;
            visual.gradient = RectGradient::Vertical(
                Lerp(CurrentTheme->primary, Color(1.0f, 1.0f, 1.0f, 1.0f), dark ? 0.10f + combo * 0.14f : 0.20f + combo * 0.04f),
                Lerp(CurrentTheme->primary, Color(0.0f, 0.0f, 0.0f, 1.0f), dark ? 0.18f + combo * 0.06f : 0.10f + combo * 0.04f)
            );
        }

        return visual;
    }

    RectFrame bounds_;
    std::array<CardRuntime, 4> cards_{};
};

} // namespace EUINEO
