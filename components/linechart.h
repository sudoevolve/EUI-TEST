#pragma once

#include "components/theme.h"
#include "core/dsl.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace components {

struct LineChartStyle {
    LineChartStyle() : LineChartStyle(theme::DarkThemeColors()) {}

    explicit LineChartStyle(const theme::ThemeColorTokens& tokens) {
        background = tokens.surface;
        title = tokens.text;
        label = theme::withOpacity(tokens.text, 0.56f);
        grid = theme::withOpacity(tokens.border, tokens.dark ? 0.38f : 0.36f);
        line = tokens.primary;
        point = tokens.primary;
        pointHover = core::mixColor(tokens.primary, theme::color(1.0f, 1.0f, 1.0f), tokens.dark ? 0.18f : 0.10f);
        pointPressed = core::mixColor(tokens.primary, theme::color(0.0f, 0.0f, 0.0f), 0.12f);
        tooltipBackground = tokens.dark
            ? core::mixColor(tokens.surface, theme::color(0.0f, 0.0f, 0.0f), 0.18f)
            : theme::color(1.0f, 1.0f, 1.0f, 0.96f);
        tooltipText = tokens.text;
        border = theme::withOpacity(tokens.border, 0.76f);
        shadow = theme::shadow(tokens, 18.0f, 4.0f, 0.20f, 0.10f);
    }

    core::Color background;
    core::Color title;
    core::Color label;
    core::Color grid;
    core::Color line;
    core::Color point;
    core::Color pointHover;
    core::Color pointPressed;
    core::Color tooltipBackground;
    core::Color tooltipText;
    core::Color border;
    core::Shadow shadow;
    float radius = 18.0f;
};

class LineChartBuilder {
    struct TooltipItem {
        std::string sourceId;
        std::string text;
        float x = 0.0f;
        float y = 0.0f;
    };

public:
    LineChartBuilder(core::dsl::Ui& ui, std::string id)
        : ui_(ui), id_(std::move(id)) {}

    LineChartBuilder& size(float width, float height) { width_ = width; height_ = height; return *this; }
    LineChartBuilder& title(const std::string& value) { title_ = value; return *this; }
    LineChartBuilder& values(std::vector<float> value) { values_ = std::move(value); return *this; }
    LineChartBuilder& labels(std::vector<std::string> value) { labels_ = std::move(value); return *this; }
    LineChartBuilder& style(const LineChartStyle& value) { style_ = value; return *this; }
    LineChartBuilder& theme(const theme::ThemeColorTokens& tokens) { style_ = LineChartStyle(tokens); return *this; }
    LineChartBuilder& transition(const core::Transition& value) { transition_ = value; return *this; }

    void build() {
        if (values_.empty()) {
            values_ = {0.22f, 0.30f, 0.20f, 0.55f, 0.42f, 0.86f};
        }

        const float titleX = 20.0f;
        const float titleY = 18.0f;
        const float plotX = 28.0f;
        const float plotY = 70.0f;
        const float plotWidth = std::max(1.0f, width_ - 56.0f);
        const float plotHeight = std::max(1.0f, height_ - 112.0f);
        const float bottomY = plotY + plotHeight;
        const int count = static_cast<int>(values_.size());
        const float stepX = count > 1 ? plotWidth / static_cast<float>(count - 1) : 0.0f;
        const float lineHeight = 4.0f;
        const float pointSize = 13.0f;

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
                    .y(titleY)
                    .size(std::max(0.0f, width_ - titleX * 2.0f), 28.0f)
                    .text(title_)
                    .customFont("YouSheBiaoTiHei")
                    .fontSize(22.0f)
                    .lineHeight(26.0f)
                    .color(style_.title)
                    .build();

                for (int line = 0; line < 4; ++line) {
                    const float y = plotY + static_cast<float>(line) * plotHeight / 3.0f;
                    ui_.rect(id_ + ".grid." + std::to_string(line))
                        .x(plotX)
                        .y(y)
                        .size(plotWidth, 1.0f)
                        .color(style_.grid)
                        .build();
                }

                for (int index = 0; index + 1 < count; ++index) {
                    const core::Vec2 from = pointAt(index, plotX, bottomY, plotHeight, stepX);
                    const core::Vec2 to = pointAt(index + 1, plotX, bottomY, plotHeight, stepX);
                    const float dx = to.x - from.x;
                    const float dy = to.y - from.y;
                    const float length = std::sqrt(dx * dx + dy * dy);
                    const float angle = std::atan2(dy, dx);
                    const float midX = (from.x + to.x) * 0.5f;
                    const float midY = (from.y + to.y) * 0.5f;

                    ui_.rect(id_ + ".segment." + std::to_string(index))
                        .x(midX - length * 0.5f)
                        .y(midY - lineHeight * 0.5f)
                        .size(length, lineHeight)
                        .color(style_.line)
                        .radius(lineHeight * 0.5f)
                        .rotate(angle)
                        .transformOrigin(0.5f, 0.5f)
                        .transition(transition_)
                        .build();
                }

                std::vector<TooltipItem> tooltips;
                tooltips.reserve(values_.size());
                for (int index = 0; index < count; ++index) {
                    const core::Vec2 point = pointAt(index, plotX, bottomY, plotHeight, stepX);
                    const std::string pointId = id_ + ".point." + std::to_string(index);
                    ui_.rect(pointId)
                        .x(point.x - pointSize * 0.5f)
                        .y(point.y - pointSize * 0.5f)
                        .size(pointSize, pointSize)
                        .states(style_.point, style_.pointHover, style_.pointPressed)
                        .radius(pointSize * 0.5f)
                        .transition(transition_)
                        .onClick([] {})
                        .build();

                    tooltips.push_back({pointId, dataLabel(index) + "  " + percent(values_[index]), point.x, point.y});
                }

                const float labelWidth = std::max(28.0f, std::min(42.0f, count > 1 ? stepX : plotWidth));
                for (int index = 0; index < count; ++index) {
                    const core::Vec2 point = pointAt(index, plotX, bottomY, plotHeight, stepX);
                    const float labelX = std::clamp(point.x - labelWidth * 0.5f, 0.0f, std::max(0.0f, width_ - labelWidth));
                    label(id_ + ".label." + std::to_string(index), dataLabel(index), labelX, height_ - 34.0f, labelWidth);
                }

                for (const TooltipItem& item : tooltips) {
                    tooltip(item.sourceId, item.text, item.x, item.y);
                }
            })
            .build();
    }

private:
    core::Vec2 pointAt(int index, float plotX, float bottomY, float plotHeight, float stepX) const {
        const float value = std::clamp(values_[index], 0.0f, 1.0f);
        return {
            plotX + static_cast<float>(index) * stepX,
            bottomY - value * plotHeight
        };
    }

    void label(const std::string& id, const std::string& value, float x, float y, float width) {
        ui_.text(id)
            .x(x)
            .y(y)
            .size(width, 22.0f)
            .text(value)
            .customFont("YouSheBiaoTiHei")
            .fontSize(14.0f)
            .lineHeight(18.0f)
            .color(style_.label)
            .horizontalAlign(core::HorizontalAlign::Center)
            .build();
    }

    std::string dataLabel(int index) const {
        if (index >= 0 && index < static_cast<int>(labels_.size())) {
            return labels_[index];
        }
        return "P" + std::to_string(index + 1);
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
    std::string title_ = "LineChart";
    std::vector<float> values_;
    std::vector<std::string> labels_ = {"Jan", "Feb", "Mar", "Apr", "May", "Jun"};
    LineChartStyle style_;
    core::Transition transition_ = core::Transition::make(0.16f, core::Ease::OutCubic);
    float width_ = 206.0f;
    float height_ = 236.0f;
};

inline LineChartBuilder linechart(core::dsl::Ui& ui, const std::string& id) {
    return LineChartBuilder(ui, id);
}

inline LineChartBuilder lineChart(core::dsl::Ui& ui, const std::string& id) {
    return LineChartBuilder(ui, id);
}

} // namespace components
