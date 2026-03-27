#pragma once

#include "../EUINEO.h"
#include "../ui/UIContext.h"
#include "../ui/ThemeTokens.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace EUINEO {

class AnimationPage {
public:
    static void Compose(UIContext& ui, const std::string& idPrefix, const RectFrame& bounds) {
        if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
            return;
        }

        const PageHeaderLayout header = ComposePageHeader(
            ui,
            idPrefix,
            bounds,
            "Animation Page",
            "Hover cards to preview DSL tracks. The samples use ui.panel / ui.polygon directly."
        );

        const Layout layout = MakeLayout(bounds, header.contentY);
        for (int index = 0; index < static_cast<int>(Cards().size()); ++index) {
            ComposeCard(
                ui,
                idPrefix + ".card." + std::to_string(index),
                CardFrame(bounds, layout, header.contentY, index),
                Cards()[index]
            );
        }
    }

private:
    enum class SampleKind {
        Fade,
        Scale,
        Move,
        Rotate
    };

    struct CardSpec {
        const char* title;
        const char* line1;
        const char* line2;
        const char* badge;
        SampleKind kind;
    };

    struct Layout {
        float gap = 18.0f;
        float topOffset = 98.0f;
        int columns = 1;
        float cardWidth = 0.0f;
        float cardHeight = 0.0f;
    };

    static const std::array<CardSpec, 4>& Cards() {
        static const std::array<CardSpec, 4> cards{{
            {"Fade Alpha", "Hover sample to fade between", "two opacity states.", ".hoverOpacity()", SampleKind::Fade},
            {"Uniform Scale", "Hover sample to scale", "both axes together.", ".hoverScale()", SampleKind::Scale},
            {"Move XY", "Hover sample to shift", "x / y with one builder.", ".hoverTranslate*", SampleKind::Move},
            {"Triangle Rotate", "Hover sample to rotate triangle", "and blend plate color.", "panel + polygon", SampleKind::Rotate},
        }};
        return cards;
    }

    static Layout MakeLayout(const RectFrame& bounds, float contentY) {
        Layout layout;
        layout.topOffset = std::max(0.0f, contentY - bounds.y);
        layout.columns = bounds.width >= 520.0f ? 2 : 1;
        layout.cardWidth = layout.columns == 2 ? (bounds.width - layout.gap) * 0.5f : bounds.width;

        const float availableHeight = std::max(240.0f, bounds.height - layout.topOffset - layout.gap);
        layout.cardHeight = layout.columns == 2
            ? std::clamp((availableHeight - layout.gap) * 0.5f, 170.0f, 210.0f)
            : std::clamp((availableHeight - layout.gap * 3.0f) * 0.25f, 116.0f, 152.0f);
        return layout;
    }

    static RectFrame CardFrame(const RectFrame& bounds, const Layout& layout, float contentY, int index) {
        const int column = index % layout.columns;
        const int row = index / layout.columns;
        return RectFrame{
            bounds.x + column * (layout.cardWidth + layout.gap),
            contentY + row * (layout.cardHeight + layout.gap),
            layout.cardWidth,
            layout.cardHeight
        };
    }

    static RectFrame SampleFrame(const RectFrame& cardFrame) {
        RectFrame frame;
        frame.width = std::min(84.0f, cardFrame.width * 0.24f);
        frame.height = std::min(56.0f, cardFrame.height * 0.30f);
        frame.x = cardFrame.x + cardFrame.width - frame.width - 26.0f;

        const float desiredY = cardFrame.y + cardFrame.height * 0.58f - frame.height * 0.5f;
        const float minY = cardFrame.y + cardFrame.height * 0.38f;
        const float maxY = cardFrame.y + cardFrame.height - frame.height - std::max(28.0f, cardFrame.height * 0.18f);
        frame.y = maxY >= minY ? std::clamp(desiredY, minY, maxY) : desiredY;
        return frame;
    }

    static void ComposeBadge(UIContext& ui, const std::string& idPrefix, const PageVisualTokens& visuals,
                             float x, float y, const std::string& text) {
        const float badgeSize = std::max(14.0f, visuals.labelSize - 1.0f);
        const float scale = badgeSize / 24.0f;
        const float width = Renderer::MeasureTextWidth(text, scale);
        ui.panel(idPrefix + ".bg")
            .position(x, y)
            .size(width + 20.0f, badgeSize + 12.0f)
            .background(CurrentTheme->surfaceHover)
            .rounding(10.0f)
            .build();

        ui.label(idPrefix + ".label")
            .text(text)
            .position(x + 10.0f, y + badgeSize + 3.0f)
            .fontSize(badgeSize)
            .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.82f))
            .build();
    }

    static void ComposeCard(UIContext& ui, const std::string& idPrefix, const RectFrame& frame, const CardSpec& card) {
        const PageVisualTokens visuals = CurrentPageVisuals();
        const Color accent = CurrentTheme->primary;
        const bool dark = CurrentTheme == &DarkTheme;
        const RectFrame sampleFrame = SampleFrame(frame);
        const float sampleCenterX = sampleFrame.x + sampleFrame.width * 0.5f;
        const float sampleCenterY = sampleFrame.y + sampleFrame.height * 0.5f;
        const float cardTitleSize = visuals.headerSubtitleSize;
        const float cardBodySize = visuals.labelSize;
        const RectGradient sampleGradient = RectGradient::Corners(
            Color(std::min(accent.r + 0.18f, 1.0f), std::min(accent.g + 0.14f, 1.0f), std::min(accent.b + 0.08f, 1.0f), 1.0f),
            Color(std::min(accent.r + 0.28f, 1.0f), std::min(accent.g + 0.08f, 1.0f), std::min(accent.b + 0.24f, 1.0f), 1.0f),
            Color(std::max(accent.r - 0.08f, 0.0f), std::max(accent.g - 0.14f, 0.0f), std::max(accent.b - 0.02f, 0.0f), 1.0f),
            Color(std::min(accent.r + 0.06f, 1.0f), std::max(accent.g - 0.10f, 0.0f), std::min(accent.b + 0.20f, 1.0f), 1.0f)
        );

        ui.panel(idPrefix + ".card")
            .position(frame.x, frame.y)
            .size(frame.width, frame.height)
            .background(CurrentTheme->surface)
            .rounding(16.0f)
            .build();

        ui.label(idPrefix + ".title")
            .text(card.title)
            .position(frame.x + 24.0f, frame.y + 40.0f)
            .fontSize(cardTitleSize)
            .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.96f))
            .build();

        ui.label(idPrefix + ".line1")
            .text(card.line1)
            .position(frame.x + 24.0f, frame.y + 72.0f)
            .fontSize(cardBodySize)
            .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.66f))
            .build();

        ui.label(idPrefix + ".line2")
            .text(card.line2)
            .position(frame.x + 24.0f, frame.y + 90.0f)
            .fontSize(cardBodySize)
            .color(Color(CurrentTheme->text.r, CurrentTheme->text.g, CurrentTheme->text.b, 0.66f))
            .build();

        ComposeBadge(ui, idPrefix + ".badge", visuals, frame.x + 24.0f, frame.y + frame.height - 38.0f, card.badge);

        switch (card.kind) {
        case SampleKind::Fade:
            ui.panel(idPrefix + ".sample.shape")
                .position(sampleFrame.x, sampleFrame.y)
                .size(sampleFrame.width, sampleFrame.height)
                .background(accent.r, accent.g, accent.b, 1.0f)
                .gradient(sampleGradient)
                .rounding(12.0f)
                .hoverOpacity(dark ? 0.36f : 0.48f, 1.0f, 0.18f)
                .build();
            break;
        case SampleKind::Scale:
            ui.panel(idPrefix + ".sample.shape")
                .position(sampleFrame.x, sampleFrame.y)
                .size(sampleFrame.width, sampleFrame.height)
                .background(accent.r, accent.g, accent.b, 1.0f)
                .gradient(sampleGradient)
                .rounding(12.0f)
                .hoverScale(1.0f, 1.18f, 0.18f)
                .build();
            break;
        case SampleKind::Move:
            ui.panel(idPrefix + ".sample.shape")
                .position(sampleFrame.x, sampleFrame.y)
                .size(sampleFrame.width, sampleFrame.height)
                .background(accent.r, accent.g, accent.b, 1.0f)
                .gradient(sampleGradient)
                .rounding(12.0f)
                .hoverTranslateX(0.0f, 18.0f, 0.18f)
                .hoverTranslateY(0.0f, -8.0f, 0.18f)
                .build();
            break;
        case SampleKind::Rotate:
            ui.panel(idPrefix + ".sample.plate")
                .position(sampleFrame.x - 10.0f, sampleFrame.y - 6.0f)
                .size(sampleFrame.width + 20.0f, sampleFrame.height + 12.0f)
                .background(accent.r, accent.g, accent.b, 0.08f)
                .rounding(14.0f)
                .hoverBackground(
                    Color(accent.r, accent.g, accent.b, 0.08f),
                    Color(0.16f, 0.82f, 0.66f, 0.18f),
                    0.18f
                )
                .build();

            ui.polygon(idPrefix + ".sample.shape")
                .position(sampleCenterX - 30.0f, sampleCenterY - 28.0f)
                .size(60.0f, 56.0f)
                .background(accent.r, accent.g, accent.b, 0.94f)
                .gradient(sampleGradient)
                .points({
                    Point2{0.50f, 0.00f},
                    Point2{1.00f, 1.00f},
                    Point2{0.00f, 1.00f},
                })
                .hoverRotation(-16.0f, 16.0f, 0.18f)
                .hoverOpacity(0.40f, 1.0f, 0.18f)
                .build();
            break;
        }
    }
};

} // namespace EUINEO
