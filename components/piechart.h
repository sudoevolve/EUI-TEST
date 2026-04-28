#pragma once

#include "components/theme.h"
#include "core/dsl.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace components {

struct PieChartStyle {
    PieChartStyle() : PieChartStyle(theme::DarkThemeColors()) {}

    explicit PieChartStyle(const theme::ThemeColorTokens& tokens) {
        background = tokens.surface;
        title = tokens.text;
        tooltipBackground = tokens.dark
            ? core::mixColor(tokens.surface, theme::color(0.0f, 0.0f, 0.0f), 0.18f)
            : theme::color(1.0f, 1.0f, 1.0f, 0.96f);
        tooltipText = tokens.text;
        border = theme::withOpacity(tokens.border, 0.76f);
        shadow = theme::shadow(tokens, 18.0f, 4.0f, 0.20f, 0.10f);
        palette = {
            theme::color(0.22f, 0.50f, 0.88f),
            theme::color(0.20f, 0.76f, 0.58f),
            theme::color(0.98f, 0.62f, 0.15f),
            theme::color(0.86f, 0.28f, 0.44f)
        };
    }

    core::Color background;
    core::Color title;
    core::Color tooltipBackground;
    core::Color tooltipText;
    core::Color border;
    core::Shadow shadow;
    std::vector<core::Color> palette;
    float radius = 18.0f;
};

class PieChartBuilder {
    struct TooltipItem {
        std::string sourceId;
        std::string text;
        float x = 0.0f;
        float y = 0.0f;
    };

public:
    PieChartBuilder(core::dsl::Ui& ui, std::string id)
        : ui_(ui), id_(std::move(id)) {}

    PieChartBuilder& size(float width, float height) { width_ = width; height_ = height; return *this; }
    PieChartBuilder& title(const std::string& value) { title_ = value; return *this; }
    PieChartBuilder& values(std::vector<float> value) { values_ = std::move(value); return *this; }
    PieChartBuilder& labels(std::vector<std::string> value) { labels_ = std::move(value); return *this; }
    PieChartBuilder& colors(std::vector<core::Color> value) { style_.palette = std::move(value); return *this; }
    PieChartBuilder& style(const PieChartStyle& value) { style_ = value; return *this; }
    PieChartBuilder& theme(const theme::ThemeColorTokens& tokens) { style_ = PieChartStyle(tokens); return *this; }
    PieChartBuilder& transition(const core::Transition& value) { transition_ = value; return *this; }

    void build() {
        const float titleX = 20.0f;
        const float pieSize = std::max(96.0f, std::min(width_ - 48.0f, height_ - 82.0f));
        const float pieX = (width_ - pieSize) * 0.5f;
        const float pieY = 70.0f;
        if (values_.empty()) {
            values_ = {0.42f, 0.24f, 0.18f, 0.16f};
        }
        if (labels_.empty()) {
            labels_ = {"Blue", "Green", "Orange", "Pink"};
        }
        const float total = valueTotal();
        float startAngle = -1.57079632679f;

        ui_.stack(id_)
            .size(width_, height_)
            .content([&] {
                ui_.rect(id_ + ".bg")
                    .size(width_, height_)
                    .color(style_.background)
                    .radius(style_.radius)
                    .border(1.0f, style_.border)
                    .shadow(style_.shadow)
                    .build();

                ui_.text(id_ + ".title")
                    .x(titleX)
                    .y(18.0f)
                    .size(std::max(0.0f, width_ - titleX * 2.0f), 28.0f)
                    .text(title_)
                    .customFont("YouSheBiaoTiHei")
                    .fontSize(22.0f)
                    .lineHeight(26.0f)
                    .color(style_.title)
                    .build();

                std::vector<TooltipItem> tooltips;
                tooltips.reserve(values_.size());
                for (int index = 0; index < static_cast<int>(values_.size()); ++index) {
                    const float amount = total > 0.0f ? std::max(0.0f, values_[index]) / total : 0.0f;
                    const float sweep = amount * 6.28318530718f;
                    const float endAngle = startAngle + sweep;
                    const core::Color color = sliceColor(index);
                    const std::string sliceId = id_ + ".slice." + std::to_string(index);

                    ui_.polygon(sliceId)
                        .x(pieX)
                        .y(pieY)
                        .size(pieSize, pieSize)
                        .points(slicePoints(pieSize, startAngle, endAngle))
                        .states(color,
                                core::mixColor(color, theme::color(1.0f, 1.0f, 1.0f), 0.18f),
                                core::mixColor(color, theme::color(0.0f, 0.0f, 0.0f), 0.12f))
                        .instantStates()
                        .transition(transition_)
                        .onClick([] {})
                        .build();

                    const core::Vec2 anchor = sliceAnchor(pieX, pieY, pieSize, (startAngle + endAngle) * 0.5f);
                    tooltips.push_back({sliceId, dataLabel(index) + "  " + percent(amount), anchor.x, anchor.y});
                    startAngle = endAngle;
                }

                for (const TooltipItem& item : tooltips) {
                    tooltip(item.sourceId, item.text, item.x, item.y);
                }
            })
            .build();
    }

private:
    float valueTotal() const {
        float total = 0.0f;
        for (float value : values_) {
            total += std::max(0.0f, value);
        }
        return total;
    }

    core::Color sliceColor(int index) const {
        if (style_.palette.empty()) {
            return theme::color(0.22f, 0.50f, 0.88f);
        }
        return style_.palette[index % static_cast<int>(style_.palette.size())];
    }

    static std::vector<core::Vec2> slicePoints(float size, float startAngle, float endAngle) {
        const float radius = size * 0.5f;
        const core::Vec2 center{radius, radius};
        const float sweep = std::max(0.0f, endAngle - startAngle);
        const int steps = std::max(2, static_cast<int>(std::ceil(sweep / 6.28318530718f * 56.0f)));
        std::vector<core::Vec2> points;
        points.reserve(static_cast<std::size_t>(steps) + 2u);
        points.push_back(center);
        for (int step = 0; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            const float angle = startAngle + sweep * t;
            points.push_back({
                center.x + std::cos(angle) * radius,
                center.y + std::sin(angle) * radius
            });
        }
        return points;
    }

    static core::Vec2 sliceAnchor(float pieX, float pieY, float pieSize, float angle) {
        const float radius = pieSize * 0.33f;
        return {
            pieX + pieSize * 0.5f + std::cos(angle) * radius,
            pieY + pieSize * 0.5f + std::sin(angle) * radius
        };
    }

    std::string dataLabel(int index) const {
        if (index >= 0 && index < static_cast<int>(labels_.size())) {
            return labels_[index];
        }
        return "S" + std::to_string(index + 1);
    }

    static std::string percent(float value) {
        return std::to_string(static_cast<int>(std::clamp(value, 0.0f, 1.0f) * 100.0f + 0.5f)) + "%";
    }

    void tooltip(const std::string& sourceId, const std::string& value, float anchorX, float anchorY) {
        const float tooltipWidth = std::min(112.0f, std::max(86.0f, width_ - 42.0f));
        const float tooltipHeight = 32.0f;
        const float pointerHeight = 8.0f;
        const float pointerHalfWidth = 7.0f;
        const float stackHeight = tooltipHeight + pointerHeight;
        const float tooltipGap = 0.0f;
        const float tooltipX = std::clamp(anchorX - tooltipWidth * 0.5f, 12.0f, std::max(12.0f, width_ - tooltipWidth - 12.0f));
        const bool belowAnchor = anchorY - stackHeight - tooltipGap < 46.0f;
        const float wantedY = belowAnchor ? anchorY + tooltipGap : anchorY - stackHeight - tooltipGap;
        const float tooltipY = std::clamp(wantedY, 44.0f, std::max(44.0f, height_ - stackHeight - 14.0f));
        const float panelY = belowAnchor ? pointerHeight : 0.0f;
        const float pointerY = belowAnchor ? 0.0f : tooltipHeight;
        const float pointerX = std::clamp(anchorX - tooltipX, pointerHalfWidth + 4.0f, tooltipWidth - pointerHalfWidth - 4.0f);
        const std::string tooltipId = sourceId + ".tooltip";
        ui_.stack(tooltipId)
            .x(tooltipX)
            .y(tooltipY)
            .size(tooltipWidth, stackHeight)
            .hoverOpacityFrom(sourceId)
            .content([&] {
                ui_.rect(tooltipId + ".bg")
                    .y(panelY)
                    .size(tooltipWidth, tooltipHeight)
                    .color(style_.tooltipBackground)
                    .radius(9.0f)
                    .border(1.0f, style_.border)
                    .shadow(12.0f, 0.0f, 4.0f, theme::color(0.0f, 0.0f, 0.0f, 0.16f))
                    .build();

                std::vector<core::Vec2> pointerPoints;
                if (belowAnchor) {
                    pointerPoints = {
                        {pointerX, 0.0f},
                        {pointerX + pointerHalfWidth, pointerHeight},
                        {pointerX - pointerHalfWidth, pointerHeight}
                    };
                } else {
                    pointerPoints = {
                        {pointerX - pointerHalfWidth, 0.0f},
                        {pointerX + pointerHalfWidth, 0.0f},
                        {pointerX, pointerHeight}
                    };
                }
                ui_.polygon(tooltipId + ".pointer")
                    .x(0.0f)
                    .y(pointerY)
                    .size(tooltipWidth, pointerHeight)
                    .points(pointerPoints)
                    .color(style_.tooltipBackground)
                    .build();

                ui_.text(tooltipId + ".text")
                    .x(10.0f)
                    .y(panelY)
                    .size(std::max(0.0f, tooltipWidth - 20.0f), tooltipHeight)
                    .text(value)
                    .fontSize(13.0f)
                    .lineHeight(16.0f)
                    .color(style_.tooltipText)
                    .horizontalAlign(core::HorizontalAlign::Center)
                    .verticalAlign(core::VerticalAlign::Center)
                    .build();
            })
            .build();
    }

    core::dsl::Ui& ui_;
    std::string id_;
    std::string title_ = "PieChart";
    std::vector<float> values_;
    std::vector<std::string> labels_;
    PieChartStyle style_;
    core::Transition transition_ = core::Transition::make(0.16f, core::Ease::OutCubic);
    float width_ = 206.0f;
    float height_ = 236.0f;
};

inline PieChartBuilder piechart(core::dsl::Ui& ui, const std::string& id) {
    return PieChartBuilder(ui, id);
}

inline PieChartBuilder pieChart(core::dsl::Ui& ui, const std::string& id) {
    return PieChartBuilder(ui, id);
}

} // namespace components
