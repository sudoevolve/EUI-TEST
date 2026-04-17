#pragma once

#include "../EUINEO.h"
#include "../ui/UIBuilder.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace EUINEO {

class TableNode : public UINode {
public:
    class Builder : public UIBuilderBase<TableNode, Builder> {
    public:
        Builder(UIContext& context, TableNode& node) : UIBuilderBase<TableNode, Builder>(context, node) {}

        Builder& headers(std::vector<std::string> value) {
            this->node_.trackComposeValue("headers", value);
            this->node_.headers_ = std::move(value);
            return *this;
        }

        Builder& rows(std::vector<std::vector<std::string>> value) {
            this->node_.trackComposeValue("rows", value);
            this->node_.rows_ = std::move(value);
            return *this;
        }

        Builder& rowHeight(float value) {
            this->node_.trackComposeValue("rowHeight", value);
            this->node_.rowHeight_ = std::max(20.0f, value);
            return *this;
        }

        Builder& fontSize(float value) {
            this->node_.trackComposeValue("fontSize", value);
            this->node_.fontSize_ = std::max(12.0f, value);
            return *this;
        }

        Builder& selectedRow(int index) {
            this->node_.trackComposeValue("selectedRow", index);
            this->node_.selectedRow_ = index;
            return *this;
        }

        Builder& onRowClick(std::function<void(int)> handler) {
            this->node_.onRowClick_ = std::move(handler);
            return *this;
        }
    };

    explicit TableNode(const std::string& key) : UINode(key) {
        resetDefaults();
    }

    static constexpr const char* StaticTypeName() {
        return "TableNode";
    }

    const char* typeName() const override {
        return StaticTypeName();
    }

    bool wantsContinuousUpdate() const override {
        if (headerHover_ > 0.001f && headerHover_ < 0.999f) {
            return true;
        }
        for (float hover : rowHover_) {
            if (hover > 0.001f && hover < 0.999f) {
                return true;
            }
        }
        return false;
    }

    void update() override {
        ensureRuntimeState();
        const bool inputAllowed = !(State.inputBlockedByPopup && primitive_.renderLayer != RenderLayer::Popup);
        const RectFrame frame = PrimitiveFrame(primitive_);
        const float headerHeight = rowHeight_;
        const RectFrame headerFrame{frame.x, frame.y, frame.width, headerHeight};
        const bool hoveredHeader = primitive_.enabled && inputAllowed && contains(headerFrame, State.mouseX, State.mouseY);
        if (animateTowards(headerHover_, hoveredHeader ? 1.0f : 0.0f, State.deltaTime * 14.0f)) {
            requestVisualRepaint(0.12f);
        }

        for (int index = 0; index < static_cast<int>(rows_.size()); ++index) {
            const RectFrame rowFrame{
                frame.x,
                frame.y + headerHeight + rowHeight_ * static_cast<float>(index),
                frame.width,
                rowHeight_
            };
            const bool hoveredRow = primitive_.enabled && inputAllowed && contains(rowFrame, State.mouseX, State.mouseY);
            if (animateTowards(rowHover_[index], hoveredRow ? 1.0f : 0.0f, State.deltaTime * 14.0f)) {
                requestVisualRepaint(0.12f);
            }
            if (hoveredRow && State.mouseClicked) {
                selectedRow_ = index;
                if (onRowClick_) {
                    onRowClick_(index);
                }
                State.mouseClicked = false;
                requestVisualRepaint(0.14f);
                break;
            }
        }
    }

    void draw() override {
        ensureRuntimeState();
        PrimitiveClipScope clip(primitive_);
        const RectFrame frame = PrimitiveFrame(primitive_);
        const float rounding = primitive_.rounding > 0.0f ? primitive_.rounding : 8.0f;
        Renderer::DrawRect(frame.x, frame.y, frame.width, frame.height,
                           ApplyOpacity(CurrentTheme->surface, primitive_.opacity), rounding);

        const int columnCount = std::max(1, resolveColumnCount());
        const float columnWidth = frame.width / static_cast<float>(columnCount);
        const float textScale = fontSize_ / 24.0f;
        const float headerHeight = rowHeight_;

        const Color headerBg = Lerp(
            Color(CurrentTheme->surfaceHover.r, CurrentTheme->surfaceHover.g, CurrentTheme->surfaceHover.b, 0.68f),
            Color(CurrentTheme->surfaceActive.r, CurrentTheme->surfaceActive.g, CurrentTheme->surfaceActive.b, 0.80f),
            headerHover_ * 0.35f
        );
        Renderer::DrawRect(frame.x, frame.y, frame.width, headerHeight, ApplyOpacity(headerBg, primitive_.opacity), rounding);

        for (int col = 0; col < columnCount; ++col) {
            const float x = frame.x + columnWidth * static_cast<float>(col);
            const std::string text = col < static_cast<int>(headers_.size()) ? headers_[col] : "";
            Renderer::DrawTextStr(
                text,
                x + 10.0f,
                frame.y + headerHeight * 0.5f + fontSize_ * 0.24f,
                ApplyOpacity(CurrentTheme->text, primitive_.opacity),
                textScale
            );
            if (col > 0) {
                Renderer::DrawRect(x, frame.y, 1.0f, frame.height, ApplyOpacity(CurrentTheme->border, primitive_.opacity), 0.0f);
            }
        }

        Renderer::DrawRect(frame.x, frame.y + headerHeight - 1.0f, frame.width, 1.0f,
                           ApplyOpacity(CurrentTheme->border, primitive_.opacity), 0.0f);

        for (int row = 0; row < static_cast<int>(rows_.size()); ++row) {
            const float rowY = frame.y + headerHeight + rowHeight_ * static_cast<float>(row);
            const bool selected = row == selectedRow_;
            Color rowBg = selected
                ? Color(CurrentTheme->primary.r, CurrentTheme->primary.g, CurrentTheme->primary.b, 0.14f)
                : Color(0.0f, 0.0f, 0.0f, 0.0f);
            rowBg = Lerp(rowBg, Color(CurrentTheme->surfaceHover.r, CurrentTheme->surfaceHover.g, CurrentTheme->surfaceHover.b, 0.72f), rowHover_[row] * (selected ? 0.25f : 1.0f));
            if (rowBg.a > 0.01f) {
                Renderer::DrawRect(frame.x + 1.0f, rowY, std::max(0.0f, frame.width - 2.0f), rowHeight_,
                                   ApplyOpacity(rowBg, primitive_.opacity), 0.0f);
            }

            for (int col = 0; col < columnCount; ++col) {
                const std::string text =
                    col < static_cast<int>(rows_[row].size()) ? rows_[row][col] : "";
                Renderer::DrawTextStr(
                    text,
                    frame.x + columnWidth * static_cast<float>(col) + 10.0f,
                    rowY + rowHeight_ * 0.5f + fontSize_ * 0.24f,
                    ApplyOpacity(CurrentTheme->text, primitive_.opacity),
                    textScale
                );
            }
            Renderer::DrawRect(frame.x, rowY + rowHeight_ - 1.0f, frame.width, 1.0f,
                               ApplyOpacity(CurrentTheme->border, primitive_.opacity), 0.0f);
        }
    }

protected:
    void resetDefaults() override {
        primitive_ = UIPrimitive{};
        primitive_.width = 420.0f;
        primitive_.height = 200.0f;
        primitive_.rounding = 8.0f;
        headers_.clear();
        rows_.clear();
        rowHeight_ = 32.0f;
        fontSize_ = 15.0f;
        selectedRow_ = -1;
        onRowClick_ = {};
    }

private:
    static bool contains(const RectFrame& frame, float x, float y) {
        return x >= frame.x && x <= frame.x + frame.width &&
               y >= frame.y && y <= frame.y + frame.height;
    }

    void ensureRuntimeState() {
        if (rowHover_.size() != rows_.size()) {
            rowHover_.assign(rows_.size(), 0.0f);
        }
    }

    int resolveColumnCount() const {
        int count = static_cast<int>(headers_.size());
        for (const auto& row : rows_) {
            count = std::max(count, static_cast<int>(row.size()));
        }
        return count;
    }

    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    float rowHeight_ = 32.0f;
    float fontSize_ = 15.0f;
    int selectedRow_ = -1;
    std::function<void(int)> onRowClick_;
    float headerHover_ = 0.0f;
    std::vector<float> rowHover_;
};

} // namespace EUINEO
