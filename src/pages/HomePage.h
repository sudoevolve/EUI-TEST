#pragma once

#include <algorithm>
#include "../EUINEO.h"
#include "../ui/UIContext.h"
#include "../ui/ThemeTokens.h"
#include <functional>
#include <string>
#include <vector>

namespace EUINEO {

class HomePage {
public:
    struct Actions {
        std::function<void()> onRandomizeThemeColor;
        std::function<void()> onToggleIconAccent;
        std::function<void(float)> onProgressChange;
        std::function<void(int)> onSegmentedChange;
        std::function<void(const std::string&)> onInputChange;
        std::function<void(int)> onComboChange;
    };

    static void Compose(UIContext& ui, const std::string& idPrefix, const RectFrame& bounds,
                        bool iconAccentEnabled, float progressValue, int segmentedIndex,
                        const std::string& inputText, int comboSelection, const Actions& actions) {
        const PageVisualTokens visuals = CurrentPageVisuals();
        const PageHeaderLayout header = ComposePageHeader(
            ui,
            idPrefix,
            bounds,
            "Home Controls",
            "Basic widgets use the same page spacing and top-aligned layout."
        );
        const bool wideActions = bounds.width >= 420.0f;
        const float buttonWidth = wideActions
            ? (bounds.width - 12.0f * 2.0f - 40.0f) / 3.0f
            : std::max(0.0f, bounds.width - 40.0f);
        const float formY = header.contentY + (wideActions ? 76.0f : 144.0f) + visuals.sectionGap;
        const float formHeight = std::max(0.0f, bounds.y + bounds.height - formY);

        ComposePageSection(
            ui,
            idPrefix + ".actions",
            RectFrame{bounds.x, header.contentY, bounds.width, wideActions ? 76.0f : 144.0f}
        );

        ui.button(idPrefix + ".primary")
            .text("Primary")
            .position(bounds.x + 20.0f, header.contentY + (wideActions ? 18.0f : 12.0f))
            .size(buttonWidth, 40.0f)
            .style(ButtonStyle::Primary)
            .fontSize(20.0f)
            .onClick([action = actions.onRandomizeThemeColor] {
                if (action) {
                    action();
                }
            })
            .build();

        ui.button(idPrefix + ".outline")
            .text("Outline")
            .position(
                wideActions ? bounds.x + 20.0f + buttonWidth + 12.0f : bounds.x + 20.0f,
                wideActions ? header.contentY + 18.0f : header.contentY + 12.0f + 40.0f + 6.0f
            )
            .size(buttonWidth, 40.0f)
            .style(ButtonStyle::Outline)
            .fontSize(20.0f)
            .build();

        ui.button(idPrefix + ".icon")
            .text("Icon")
            .icon("\xEF\x80\x93")
            .iconPlacement(ButtonIconPlacement::Trailing)
            .position(
                wideActions ? bounds.x + 20.0f + (buttonWidth + 12.0f) * 2.0f : bounds.x + 20.0f,
                wideActions ? header.contentY + 18.0f : header.contentY + 12.0f + (40.0f + 6.0f) * 2.0f
            )
            .size(buttonWidth, 40.0f)
            .fontSize(20.0f)
            .textColor(iconAccentEnabled ? CurrentTheme->primary : CurrentTheme->text)
            .onClick([action = actions.onToggleIconAccent] {
                if (action) {
                    action();
                }
            })
            .build();

        if (formHeight <= 0.0f) {
            return;
        }

        ComposePageSection(ui, idPrefix + ".form", RectFrame{bounds.x, formY, bounds.width, formHeight});

        const float innerX = bounds.x + visuals.sectionInset;
        const float innerY = formY + visuals.sectionInset;
        const float innerWidth = std::max(0.0f, bounds.width - visuals.sectionInset * 2.0f);
        const float innerHeight = std::max(0.0f, formHeight - visuals.sectionInset * 2.0f);
        if (innerWidth <= 0.0f || innerHeight <= 0.0f) {
            return;
        }

        const float columnWidth = std::max(0.0f, (innerWidth - visuals.sectionGap) * 0.5f);
        const float leftX = innerX;
        const float leftY = innerY;
        const float rightX = innerX + columnWidth + visuals.sectionGap;
        const float rightY = innerY;
        const float leftFieldWidth = std::max(0.0f, columnWidth - 36.0f);
        const float rightFieldWidth = std::max(0.0f, columnWidth - 36.0f);
        const float progressValueX = std::max(leftX + 18.0f, leftX + columnWidth - 58.0f);

        ui.label(idPrefix + ".progress.label")
            .text("Progress")
            .position(leftX + 18.0f, leftY + 28.0f)
            .fontSize(visuals.labelSize)
            .build();

        ui.label(idPrefix + ".progress.value")
            .text(std::to_string(static_cast<int>(progressValue * 100.0f)) + "%")
            .position(progressValueX, leftY + 28.0f)
            .fontSize(16.0f)
            .color(visuals.bodyColor)
            .build();

        ui.progress(idPrefix + ".progress")
            .position(leftX + 18.0f, leftY + 56.0f)
            .size(leftFieldWidth, 15.0f)
            .value(progressValue)
            .build();

        ui.slider(idPrefix + ".slider")
            .position(leftX + 18.0f, leftY + 82.0f)
            .size(leftFieldWidth, 20.0f)
            .value(progressValue)
            .onChange([action = actions.onProgressChange](float value) {
                if (action) {
                    action(value);
                }
            })
            .build();

        ui.label(idPrefix + ".segmented.label")
            .text("Segment")
            .position(leftX + 18.0f, leftY + 126.0f)
            .fontSize(visuals.labelSize)
            .build();

        ui.segmented(idPrefix + ".segmented")
            .position(leftX + 18.0f, leftY + 154.0f)
            .size(leftFieldWidth, 35.0f)
            .items({"Apple", "Banana", "Cherry"})
            .selected(segmentedIndex)
            .fontSize(20.0f)
            .onChange([action = actions.onSegmentedChange](int index) {
                if (action) {
                    action(index);
                }
            })
            .build();

        ui.label(idPrefix + ".input.label")
            .text("Input")
            .position(rightX + 18.0f, rightY + 28.0f)
            .fontSize(visuals.labelSize)
            .build();

        ui.input(idPrefix + ".input")
            .position(rightX + 18.0f, rightY + 56.0f)
            .size(rightFieldWidth, visuals.fieldHeight)
            .placeholder("Type something...")
            .fontSize(20.0f)
            .text(inputText)
            .onChange([action = actions.onInputChange](const std::string& text) {
                if (action) {
                    action(text);
                }
            })
            .build();

        ui.label(idPrefix + ".combo.label")
            .text("Combo")
            .position(rightX + 18.0f, rightY + 108.0f)
            .fontSize(visuals.labelSize)
            .build();

        ui.combo(idPrefix + ".combo")
            .position(rightX + 18.0f, rightY + 136.0f)
            .size(rightFieldWidth, visuals.fieldHeight)
            .placeholder("Select an option")
            .fontSize(20.0f)
            .maxVisibleItems(4)
            .startOpen(false)
            .items({"Apple", "Banana", "Cherry", "Grape", "Orange", "Peach", "Pear"})
            .selected(comboSelection)
            .onChange([action = actions.onComboChange](int index) {
                if (action) {
                    action(index);
                }
            })
            .build();
    }
};

} // namespace EUINEO
