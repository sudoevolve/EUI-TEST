#pragma once

#include "../EUINEO.h"
#include "../ui/UIContext.h"
#include "../ui/ThemeTokens.h"
#include <algorithm>
#include <array>
#include <string>

namespace EUINEO {

class TypographyPage {
public:
    static void Compose(UIContext& ui, const std::string& idPrefix, const RectFrame& bounds) {
        if (bounds.width <= 0.0f || bounds.height <= 0.0f) return;

        const PageVisualTokens visuals = CurrentPageVisuals();
        const PageHeaderLayout header = ComposePageHeader(
            ui, idPrefix, bounds, "Typography",
            "SDF text preview across different sizes, mixed content, and icon rendering."
        );

        const RectFrame viewport{
            bounds.x,
            header.contentY,
            bounds.width,
            std::max(0.0f, bounds.y + bounds.height - header.contentY)
        };
        if (viewport.height <= 0.0f) return;

        ComposePageSection(ui, idPrefix + ".viewport", viewport);

        const bool compact = viewport.width < 640.0f;

        ui.scrollArea(
            idPrefix + ".scroll",
            viewport.x,
            viewport.y,
            viewport.width,
            viewport.height,
            visuals.sectionInset
                + MeasureRowsSectionHeight(ScaleSection(), TypeRows(), compact)
                + visuals.sectionGap
                + MeasureMixedSectionHeight()
                + visuals.sectionGap
                + MeasureRowsSectionHeight(IconSection(), IconRows(), compact),
            [&](float) {
            float y = viewport.y + visuals.sectionInset;
            ComposeRowsSection(
                ui, idPrefix + ".scale",
                RectFrame{
                    viewport.x + visuals.sectionInset,
                    y,
                    std::max(0.0f, viewport.width - visuals.sectionInset * 2.0f),
                    MeasureRowsSectionHeight(ScaleSection(), TypeRows(), compact)
                },
                ScaleSection(), TypeRows(), compact
            );
            y += MeasureRowsSectionHeight(ScaleSection(), TypeRows(), compact) + visuals.sectionGap;

            ComposeMixedSection(
                ui, idPrefix + ".mixed",
                RectFrame{
                    viewport.x + visuals.sectionInset,
                    y,
                    std::max(0.0f, viewport.width - visuals.sectionInset * 2.0f),
                    MeasureMixedSectionHeight()
                }
            );
            y += MeasureMixedSectionHeight() + visuals.sectionGap;

            ComposeRowsSection(
                ui, idPrefix + ".icons",
                RectFrame{
                    viewport.x + visuals.sectionInset,
                    y,
                    std::max(0.0f, viewport.width - visuals.sectionInset * 2.0f),
                    MeasureRowsSectionHeight(IconSection(), IconRows(), compact)
                },
                IconSection(), IconRows(), compact
            );
        });
    }

private:
    struct SectionText { const char* title = ""; const char* subtitle = ""; };
    struct PreviewRow { float size = 16.0f; const char* sample = ""; };

    static constexpr float kMetaFontSize = 18.0f;
    static constexpr float kHeaderTitleSize = 22.0f;
    static constexpr float kHeaderSubtitleSize = 16.0f;
    static constexpr float kWideSampleOffset = 108.0f;
    static constexpr float kRowGap = 8.0f;
    static constexpr float kRowBottomPadding = 14.0f;
    static constexpr float kHeaderBottomPadding = 18.0f;
    static constexpr const char* kIconSampleText =
        "\xEF\x80\xB1   \xEF\x80\x95   \xEF\x80\x93   \xEF\x84\x87   \xEF\x86\x85   \xEF\x86\x86";
    static constexpr const char* kMixedHeroText = "SDF keeps large counters clean";
    static constexpr const char* kMixedBodyText = "0123456789  ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr const char* kMixedBody2Text = "The quick brown fox jumps over the lazy dog.";
    static constexpr const char* kMixedBody3Text =
        "\xE4\xB8\xAD\xE6\x96\x87\xE5\x9B\x9E\xE9\x80\x80\xE9\xA2\x84\xE8\xA7\x88"
        "  "
        "\xE4\xBD\xA0\xE5\xA5\xBD\xEF\xBC\x8C\xE4\xB8\x96\xE7\x95\x8C"
        "  "
        "\xE5\xAD\x97\xE4\xBD\x93\xE6\xB7\xB7\xE6\x8E\x92";

    static SectionText ScaleSection() { return {"Type Scale", "Preview the same renderer at small and large text sizes."}; }
    static SectionText MixedSection() { return {"Mixed Content", "Latin, numerals, punctuation, and CJK fallback in one place."}; }
    static SectionText IconSection() { return {"Icon Sizes", "Font Awesome glyphs previewed across multiple display sizes."}; }

    static const std::array<PreviewRow, 7>& TypeRows() {
        static const std::array<PreviewRow, 7> rows{{
            {12.0f, "Caption 12  SDF sample 012345  ABC"},
            {14.0f, "Body 14  Aa Bb Cc 012345  Sharp edges"},
            {18.0f, "Reading size 18  The quick brown fox " "\xE4\xBD\xA0\xE5\xA5\xBD"},
            {24.0f, "Title 24  Vector-like curves and counters"},
            {32.0f, "Display 32  Mixed Latin / " "\xE4\xB8\xAD\xE6\x96\x87" " glyphs"},
            {44.0f, "Poster 44 typography"},
            {60.0f, "Hero 60 距离场 SDF"},
        }};
        return rows;
    }

    static const std::array<PreviewRow, 5>& IconRows() {
        static const std::array<PreviewRow, 5> rows{{
            {16.0f, kIconSampleText}, {20.0f, kIconSampleText}, {24.0f, kIconSampleText},
            {32.0f, kIconSampleText}, {48.0f, kIconSampleText},
        }};
        return rows;
    }

    static std::string SizeLabel(float size) { return std::to_string(static_cast<int>(size)) + " px"; }

    static float TextHeight(const std::string& text, float fontSize) {
        const RectFrame bounds = Renderer::MeasureTextBounds(text, fontSize / 24.0f);
        return std::max(bounds.height, fontSize * 0.80f);
    }

    static float TextBottom(const std::string& text, float fontSize, float anchorY) {
        const RectFrame bounds = Renderer::MeasureTextBounds(text, fontSize / 24.0f);
        return anchorY + bounds.y + std::max(bounds.height, fontSize * 0.80f);
    }

    static float SectionHeaderHeight(const SectionText& section) {
        const PageVisualTokens visuals = CurrentPageVisuals();
        return std::max(
            TextBottom(section.title, kHeaderTitleSize, visuals.sectionInset),
            TextBottom(section.subtitle, kHeaderSubtitleSize, visuals.sectionInset + 28.0f)
        ) + kHeaderBottomPadding;
    }

    static float RowHeight(const PreviewRow& row, bool compact) {
        const float labelHeight = TextHeight(SizeLabel(row.size), kMetaFontSize);
        const float sampleHeight = TextHeight(row.sample, row.size);
        return compact
            ? labelHeight + kRowGap + sampleHeight + kRowBottomPadding
            : std::max(labelHeight, sampleHeight) + kRowBottomPadding;
    }

    template <size_t N>
    static float MeasureRowsSectionHeight(const SectionText& section, const std::array<PreviewRow, N>& rows, bool compact) {
        float height = SectionHeaderHeight(section);
        for (size_t i = 0; i < rows.size(); ++i) {
            height += RowHeight(rows[i], compact);
            if (i + 1 < rows.size()) height += kRowGap;
        }
        return height;
    }

    static float MeasureMixedSectionHeight() {
        const float y = SectionHeaderHeight(MixedSection());
        return std::max({
            TextBottom(kMixedHeroText, 30.0f, y),
            TextBottom(kMixedBodyText, 18.0f, y + 42.0f),
            TextBottom(kMixedBody2Text, 18.0f, y + 72.0f),
            TextBottom(kMixedBody3Text, 24.0f, y + 102.0f),
        }) + kRowBottomPadding;
    }

    static void ComposeSectionHeader(UIContext& ui, const std::string& idPrefix, const RectFrame& frame,
                                     const SectionText& section) {
        const PageVisualTokens visuals = CurrentPageVisuals();
        ui.label(idPrefix + ".title").text(section.title)
            .position(frame.x + visuals.sectionInset, frame.y + visuals.sectionInset)
            .fontSize(kHeaderTitleSize).color(visuals.titleColor).build();
        ui.label(idPrefix + ".subtitle").text(section.subtitle)
            .position(frame.x + visuals.sectionInset, frame.y + visuals.sectionInset + 28.0f)
            .fontSize(kHeaderSubtitleSize).color(visuals.bodyColor).build();
    }

    static void ComposePreviewRow(UIContext& ui, const std::string& idPrefix, const PreviewRow& row,
                                  float leftX, float sampleX, float rowTop, bool compact, const Color& labelColor) {
        const std::string label = SizeLabel(row.size);
        const RectFrame labelBounds = Renderer::MeasureTextBounds(label, kMetaFontSize / 24.0f);
        const RectFrame sampleBounds = Renderer::MeasureTextBounds(row.sample, row.size / 24.0f);
        const float labelY = rowTop - labelBounds.y;
        const float sampleTop = compact ? rowTop + TextHeight(label, kMetaFontSize) + kRowGap : rowTop;
        const float sampleY = sampleTop - sampleBounds.y;

        ui.label(idPrefix + ".label").text(label).position(leftX, labelY)
            .fontSize(kMetaFontSize).color(labelColor).build();
        ui.label(idPrefix + ".sample").text(row.sample).position(sampleX, sampleY)
            .fontSize(row.size).build();
    }

    template <size_t N>
    static void ComposeRowsSection(UIContext& ui, const std::string& idPrefix, const RectFrame& frame,
                                   const SectionText& section, const std::array<PreviewRow, N>& rows, bool compact) {
        const PageVisualTokens visuals = CurrentPageVisuals();
        ComposePageSection(ui, idPrefix + ".panel", frame);
        ComposeSectionHeader(ui, idPrefix, frame, section);

        const float leftX = frame.x + visuals.sectionInset;
        const float sampleX = compact ? leftX : leftX + kWideSampleOffset;
        float rowTop = frame.y + SectionHeaderHeight(section);
        for (size_t i = 0; i < rows.size(); ++i) {
            ComposePreviewRow(ui, idPrefix + ".row." + std::to_string(i), rows[i], leftX, sampleX, rowTop, compact, visuals.bodyColor);
            rowTop += RowHeight(rows[i], compact);
            if (i + 1 < rows.size()) rowTop += kRowGap;
        }
    }

    static void ComposeMixedSection(UIContext& ui, const std::string& idPrefix, const RectFrame& frame) {
        const PageVisualTokens visuals = CurrentPageVisuals();
        const SectionText section = MixedSection();
        const float x = frame.x + visuals.sectionInset;
        const float y = frame.y + SectionHeaderHeight(section);

        ComposePageSection(ui, idPrefix + ".panel", frame);
        ComposeSectionHeader(ui, idPrefix, frame, section);

        ui.label(idPrefix + ".hero").text(kMixedHeroText).position(x, y).fontSize(30.0f).build();
        ui.label(idPrefix + ".body").text(kMixedBodyText).position(x, y + 42.0f)
            .fontSize(18.0f).color(visuals.bodyColor).build();
        ui.label(idPrefix + ".body2").text(kMixedBody2Text).position(x, y + 72.0f)
            .fontSize(18.0f).color(visuals.bodyColor).build();
        ui.label(idPrefix + ".body3").text(kMixedBody3Text).position(x, y + 102.0f).fontSize(24.0f).build();
    }
};

} // namespace EUINEO
