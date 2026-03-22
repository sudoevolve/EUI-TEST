#pragma once

#include "AnimationPage.h"
#include "../ui/UIContext.h"
#include "MainPageView.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace EUINEO {

class MainPage {
public:
    MainPage() = default;

    void Update() {
        if (pageReveal_ < 1.0f) {
            const float previous = pageReveal_;
            pageReveal_ = Lerp(pageReveal_, 1.0f, State.deltaTime * 11.0f);
            if (std::abs(1.0f - pageReveal_) < 0.01f) {
                pageReveal_ = 1.0f;
            }
            if (std::abs(previous - pageReveal_) > 0.0001f) {
                Renderer::RequestRepaint(0.18f);
            }
        }

        Compose();
        ui_.update();
        UpdateActiveView();
        Compose();
    }

    void Draw() {
        ui_.draw();
    }

private:
    void Compose() {
        const Layout layout = MakeLayout();
        const float sidebarX = layout.sidebarX;
        const float sidebarY = layout.sidebarY;
        const float sidebarH = layout.sidebarH;
        const float contentX = layout.contentX;
        const float contentY = layout.contentY;
        const float contentW = layout.contentW;
        const float contentH = layout.contentH;
        const float currentContentOffsetX = layout.currentContentOffsetX;
        const float contentCenterX = contentX + contentW * 0.5f + currentContentOffsetX;
        const float contentCenterY = contentY + contentH * 0.5f;
        const float controlsX = contentCenterX - 150.0f;
        const float titleScale = 32.0f / 24.0f;
        const float titleWidth = Renderer::MeasureTextWidth("EUI-NEO", titleScale);

        ui_.begin("main");

        ui_.sidebar("sidebar")
            .position(sidebarX, sidebarY)
            .size(sidebarWidth_, sidebarH)
            .width(60.0f, sidebarWidth_)
            .brand("EUI", "NEO")
            .selectedIndex(static_cast<int>(currentView_))
            .item("\xEF\x80\x95", "Home", [this] { SwitchView(MainPageView::Home); })
            .item("\xEF\x81\x8B", "Animation", [this] { SwitchView(MainPageView::Animation); })
            .themeToggle([this] { ToggleTheme(); })
            .build();

        ui_.glassPanel("content")
            .position(contentX, contentY)
            .size(contentW, contentH)
            .rounding(16.0f)
            .blur(progressValue_ * 0.15f)
            .zIndex(-2)
            .build();

        ui_.pushClip(contentX, contentY, contentW, contentH);

        ui_.panel("bg.red")
            .position(contentX + contentW * 0.10f - 84.0f, contentY + 58.0f)
            .size(196.0f, 196.0f)
            .background(0.98f, 0.36f, 0.36f, 0.92f)
            .rounding(98.0f)
            .zIndex(-3)
            .build();

        ui_.panel("bg.green")
            .position(contentX + contentW * 0.58f, contentY + 86.0f)
            .size(164.0f, 164.0f)
            .background(0.30f, 0.92f, 0.58f, 0.88f)
            .rounding(82.0f)
            .zIndex(-3)
            .build();

        ui_.panel("bg.blue")
            .position(contentX + contentW * 0.30f, contentY + contentH * 0.44f)
            .size(246.0f, 246.0f)
            .background(0.34f, 0.52f, 1.0f, 0.90f)
            .rounding(123.0f)
            .zIndex(-3)
            .build();

        if (currentView_ == MainPageView::Home) {

            ui_.button("home.primary")
                .text("Primary")
                .position(contentCenterX - 130.0f, contentY + 62.0f)
                .style(ButtonStyle::Primary)
                .fontSize(20.0f)
                .onClick([this] {
                    progressValue_ += 0.1f;
                    if (progressValue_ > 1.0f) {
                        progressValue_ = 0.0f;
                    }
                })
                .build();

            ui_.button("home.outline")
                .text("Outline")
                .position(contentCenterX + 10.0f, contentY + 62.0f)
                .style(ButtonStyle::Outline)
                .fontSize(20.0f)
                .onClick([this] {
                    progressValue_ = 0.0f;
                })
                .build();

            ui_.button("home.icon")
                .text("Icon  \xEF\x80\x93")
                .position(contentCenterX - 60.0f, contentY + 122.0f)
                .fontSize(20.0f)
                .build();

            ui_.progress("home.progress")
                .position(controlsX, contentCenterY - 60.0f)
                .size(300.0f, 15.0f)
                .value(progressValue_)
                .build();

            ui_.slider("home.slider")
                .position(controlsX, contentCenterY - 10.0f)
                .size(300.0f, 20.0f)
                .value(progressValue_)
                .onChange([this](float value) {
                    progressValue_ = value;
                })
                .build();

            ui_.segmented("home.segmented")
                .position(controlsX, contentCenterY + 40.0f)
                .size(300.0f, 35.0f)
                .items(segmentedItems_)
                .selected(segmentedIndex_)
                .fontSize(20.0f)
                .onChange([this](int index) {
                    segmentedIndex_ = index;
                })
                .build();

            ui_.input("home.input")
                .position(controlsX, contentCenterY + 100.0f)
                .size(300.0f, 35.0f)
                .placeholder("Type something...")
                .fontSize(20.0f)
                .text(inputText_)
                .onChange([this](const std::string& text) {
                    inputText_ = text;
                })
                .build();

            ui_.combo("home.combo")
                .position(controlsX, contentCenterY + 160.0f)
                .size(300.0f, 35.0f)
                .placeholder("Select an option")
                .fontSize(20.0f)
                .items(comboItems_)
                .selected(comboSelection_)
                .onChange([this](int index) {
                    comboSelection_ = index;
                })
                .build();
        } else {
            animationPage_.Compose(ui_, "animation.page");
        }

        ui_.popClip();
        ui_.end();
    }

    void ToggleTheme() {
        CurrentTheme = CurrentTheme == &DarkTheme ? &LightTheme : &DarkTheme;
        Renderer::InvalidateBackdrop();
        Renderer::InvalidateAll();
    }

    void SwitchView(MainPageView view) {
        if (view == currentView_) {
            return;
        }
        const int previousIndex = static_cast<int>(currentView_);
        const int nextIndex = static_cast<int>(view);
        currentView_ = view;
        pageReveal_ = 0.0f;
        pageRevealDirection_ = nextIndex >= previousIndex ? 1 : -1;
        Renderer::InvalidateAll();
    }

    struct Layout {
        float sidebarX = 0.0f;
        float sidebarY = 0.0f;
        float sidebarH = 0.0f;
        float contentX = 0.0f;
        float contentY = 0.0f;
        float contentW = 0.0f;
        float contentH = 0.0f;
        float currentContentOffsetX = 0.0f;
    };

    Layout MakeLayout() const {
        Layout layout;
        layout.sidebarX = shellPadding_;
        layout.sidebarY = shellPadding_;
        layout.sidebarH = std::max(240.0f, State.screenH - shellPadding_ * 2.0f);
        layout.contentX = layout.sidebarX + sidebarWidth_ + 24.0f;
        layout.contentY = shellPadding_;
        layout.contentW = std::max(280.0f, State.screenW - layout.contentX - shellPadding_);
        layout.contentH = std::max(240.0f, State.screenH - shellPadding_ * 2.0f);
        layout.currentContentOffsetX = (1.0f - pageReveal_) * 28.0f * static_cast<float>(pageRevealDirection_);
        return layout;
    }

    RectFrame AnimationBounds() const {
        const Layout layout = MakeLayout();
        RectFrame frame;
        frame.x = layout.contentX + contentInset_ + layout.currentContentOffsetX;
        frame.y = layout.contentY + 18.0f;
        frame.width = layout.contentW - contentInset_ * 2.0f;
        frame.height = layout.contentH - 36.0f;
        return frame;
    }

    void UpdateActiveView() {
        if (currentView_ == MainPageView::Animation) {
            animationPage_.Update(AnimationBounds());
        }
    }

    UIContext ui_;
    AnimationPage animationPage_;
    MainPageView currentView_ = MainPageView::Home;
    float pageReveal_ = 1.0f;
    int pageRevealDirection_ = 1;
    float shellPadding_ = 22.0f;
    float sidebarWidth_ = 86.0f;
    float contentInset_ = 34.0f;
    float progressValue_ = 0.30f;
    std::vector<std::string> segmentedItems_{"Apple", "Banana", "Cherry"};
    int segmentedIndex_ = 0;
    std::string inputText_;
    std::vector<std::string> comboItems_{"Item 1", "Item 2", "Item 3"};
    int comboSelection_ = -1;
};

} // namespace EUINEO
