#pragma once

#include "UIPrimitive.h"
#include <algorithm>

namespace EUINEO {

struct ThemeColorTokens {
    Color background;
    Color primary;
    Color surface;
    Color surfaceHover;
    Color surfaceActive;
    Color text;
    Color border;
    bool dark = false;
};

struct PageVisualTokens {
    Color titleColor;
    Color subtitleColor;
    Color bodyColor;
    Color cardColor;
    Color mutedCardColor;
    Color softAccentColor;
    float headerTopInset = 24.0f;
    float headerTitleGap = 30.0f;
    float headerContentGap = 38.0f;
    float headerTitleSize = 31.0f;
    float headerSubtitleSize = 24.0f;
    float sectionGap = 16.0f;
    float sectionInset = 20.0f;
    float sectionRounding = 18.0f;
    float labelSize = 17.0f;
    float fieldHeight = 35.0f;
};

struct UIFieldVisualTokens {
    float rounding = 6.0f;
    float horizontalInset = 10.0f;
    float focusLineHeight = 2.0f;
    float borderLineHeight = 1.0f;
    float popupRounding = 10.0f;
    float popupOverlap = 1.0f;
    Color popupShadowColor;
    float popupShadowBlur = 0.0f;
    float popupShadowOffsetY = 0.0f;
};

struct PageHeaderLayout {
    float titleY = 0.0f;
    float subtitleY = 0.0f;
    float contentY = 0.0f;
};

inline ThemeColorTokens LightThemeColors() {
    return ThemeColorTokens{
        Color(0.95f, 0.95f, 0.97f),
        Color(0.20f, 0.50f, 0.90f),
        Color(1.00f, 1.00f, 1.00f),
        Color(0.90f, 0.90f, 0.90f),
        Color(0.80f, 0.80f, 0.80f),
        Color(0.00f, 0.00f, 0.00f),
        Color(0.80f, 0.80f, 0.80f),
        false
    };
}

inline ThemeColorTokens DarkThemeColors() {
    return ThemeColorTokens{
        Color(0.10f, 0.10f, 0.12f),
        Color(0.30f, 0.60f, 1.00f),
        Color(0.15f, 0.15f, 0.18f),
        Color(0.25f, 0.25f, 0.28f),
        Color(0.35f, 0.35f, 0.38f),
        Color(1.00f, 1.00f, 1.00f),
        Color(0.30f, 0.30f, 0.30f),
        true
    };
}

inline Theme MakeTheme(const ThemeColorTokens& tokens) {
    return Theme{
        tokens.background,
        tokens.primary,
        tokens.surface,
        tokens.surfaceHover,
        tokens.surfaceActive,
        tokens.text,
        tokens.border
    };
}

inline ThemeColorTokens CurrentThemeColors() {
    return ThemeColorTokens{
        CurrentTheme->background,
        CurrentTheme->primary,
        CurrentTheme->surface,
        CurrentTheme->surfaceHover,
        CurrentTheme->surfaceActive,
        CurrentTheme->text,
        CurrentTheme->border,
        CurrentTheme == &DarkTheme
    };
}

inline PageVisualTokens CurrentPageVisuals() {
    const ThemeColorTokens palette = CurrentThemeColors();
    return PageVisualTokens{
        Color(palette.text.r, palette.text.g, palette.text.b, 0.98f),
        Color(palette.text.r, palette.text.g, palette.text.b, 0.72f),
        Color(palette.text.r, palette.text.g, palette.text.b, 0.68f),
        palette.surface,
        palette.surfaceHover,
        Color(palette.primary.r, palette.primary.g, palette.primary.b, 0.16f),
        24.0f,
        30.0f,
        40.0f,
        31.0f,
        20.0f,
        16.0f,
        20.0f,
        18.0f,
        17.0f,
        35.0f,
    };
}

inline UIFieldVisualTokens CurrentFieldVisuals() {
    const ThemeColorTokens palette = CurrentThemeColors();
    UIFieldVisualTokens tokens;
    tokens.popupShadowColor = palette.dark
        ? Color(0.0f, 0.0f, 0.0f, 0.28f)
        : Color(0.10f, 0.14f, 0.22f, 0.14f);
    tokens.popupShadowBlur = palette.dark ? 18.0f : 12.0f;
    tokens.popupShadowOffsetY = palette.dark ? 8.0f : 5.0f;
    return tokens;
}

inline Color ResolveFieldFill(const UIPrimitive& primitive, float hoverAmount, float activeAmount) {
    const ThemeColorTokens palette = CurrentThemeColors();
    const float hover = std::clamp(hoverAmount, 0.0f, 1.0f);
    const float active = std::clamp(activeAmount, 0.0f, 1.0f);
    const Color baseColor = primitive.background.a > 0.0f ? primitive.background : palette.surface;
    const Color hoverColor = primitive.background.a > 0.0f
        ? Lerp(baseColor, palette.surfaceHover, 0.65f)
        : palette.surfaceHover;
    return Lerp(Lerp(baseColor, palette.surfaceActive, active), hoverColor, hover);
}

inline void DrawFieldChrome(const UIPrimitive& primitive, float hoverAmount, float activeAmount,
                            float rounding = 0.0f) {
    const ThemeColorTokens palette = CurrentThemeColors();
    const RectFrame frame = PrimitiveFrame(primitive);
    const UIFieldVisualTokens tokens = CurrentFieldVisuals();
    const float cornerRadius = rounding > 0.0f ? rounding : tokens.rounding;
    const float active = std::clamp(activeAmount, 0.0f, 1.0f);

    RectStyle style = MakeStyle(primitive);
    style.color = ApplyOpacity(ResolveFieldFill(primitive, hoverAmount, activeAmount), primitive.opacity);
    style.rounding = cornerRadius;
    Renderer::DrawRect(frame.x, frame.y, frame.width, frame.height, style);

    const float lineTopY = frame.y - 0.5f;
    const float borderLineHeight = std::max(tokens.borderLineHeight, cornerRadius > 2.0f ? 1.5f : 1.0f) + 0.5f;
    Renderer::DrawRect(
        frame.x,
        lineTopY,
        frame.width,
        borderLineHeight,
        ApplyOpacity(palette.border, primitive.opacity),
        cornerRadius
    );

    if (active > 0.01f) {
        Color accent = palette.primary;
        accent.a = active;
        const float focusLineHeight = std::max(tokens.focusLineHeight, borderLineHeight);
        Renderer::DrawRect(
            frame.x,
            lineTopY,
            frame.width,
            focusLineHeight,
            ApplyOpacity(accent, primitive.opacity),
            cornerRadius
        );
    }
}

inline RectFrame PopupListFrame(const RectFrame& anchorFrame, float visibleHeight, float overlap = 1.0f) {
    return RectFrame{
        anchorFrame.x,
        anchorFrame.y + anchorFrame.height - overlap,
        anchorFrame.width,
        visibleHeight + overlap
    };
}

inline RectStyle MakePopupChromeStyle(const UIPrimitive& primitive, float rounding = 0.0f) {
    const ThemeColorTokens palette = CurrentThemeColors();
    const UIFieldVisualTokens tokens = CurrentFieldVisuals();
    const float cornerRadius = rounding > 0.0f ? rounding : tokens.popupRounding;
    RectStyle style = MakeStyle(primitive);
    style.color = ApplyOpacity(palette.surface, primitive.opacity);
    style.gradient = RectGradient{};
    style.rounding = cornerRadius;
    style.shadowBlur = tokens.popupShadowBlur;
    style.shadowOffsetX = 0.0f;
    style.shadowOffsetY = tokens.popupShadowOffsetY;
    style.shadowColor = ApplyOpacity(tokens.popupShadowColor, primitive.opacity);
    return style;
}

inline void DrawPopupChrome(const UIPrimitive& primitive, const RectFrame& frame, float rounding = 0.0f) {
    Renderer::DrawRect(frame.x, frame.y, frame.width, frame.height, MakePopupChromeStyle(primitive, rounding));
}

template <typename UI>
inline PageHeaderLayout ComposePageHeader(UI& ui, const std::string& idPrefix, const RectFrame& bounds,
                                          const std::string& title, const std::string& subtitle) {
    const PageVisualTokens visuals = CurrentPageVisuals();
    PageHeaderLayout layout;
    layout.titleY = bounds.y + visuals.headerTopInset;
    layout.subtitleY = layout.titleY + visuals.headerTitleGap;
    layout.contentY = layout.subtitleY + visuals.headerContentGap;

    ui.label(idPrefix + ".title")
        .text(title)
        .position(bounds.x, layout.titleY)
        .fontSize(visuals.headerTitleSize)
        .color(visuals.titleColor)
        .build();

    ui.label(idPrefix + ".subtitle")
        .text(subtitle)
        .position(bounds.x, layout.subtitleY)
        .fontSize(visuals.headerSubtitleSize)
        .color(visuals.subtitleColor)
        .build();

    return layout;
}

template <typename UI>
inline void ComposePageSection(UI& ui, const std::string& idPrefix, const RectFrame& bounds,
                               const Color& background = Color(0.0f, 0.0f, 0.0f, 0.0f)) {
    const PageVisualTokens visuals = CurrentPageVisuals();
    ui.panel(idPrefix)
        .position(bounds.x, bounds.y)
        .size(bounds.width, bounds.height)
        .background(background.a > 0.0f ? background : visuals.cardColor)
        .rounding(visuals.sectionRounding)
        .build();
}

} // namespace EUINEO
