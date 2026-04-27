#include "app/dsl_app.h"

#include "components/components.h"

#include <algorithm>
#include <functional>
#include <string>

namespace app {

namespace {

int selectedPage = 0;
bool optionDense = false;
bool optionGlass = true;
bool optionMotion = true;
bool animationMoved = false;
bool animationRotated = false;
bool animationFaded = false;

constexpr float kSidebarWidth = 272.0f;
constexpr float kNavTop = 128.0f;
constexpr float kNavHeight = 50.0f;
constexpr float kNavGap = 14.0f;

core::Transition pageTransition() {
    if (!optionMotion) {
        return core::Transition::none();
    }
    return core::Transition::make(0.28f, core::Ease::OutCubic);
}

core::Transition motionTransition() {
    if (!optionMotion) {
        return core::Transition::none();
    }
    return core::Transition::make(0.42f, core::Ease::OutBack);
}

core::Color appBg() {
    return {0.07f, 0.08f, 0.10f, 1.0f};
}

core::Color surface() {
    return {0.11f, 0.13f, 0.17f, 1.0f};
}

core::Color surfaceSoft() {
    return {0.15f, 0.17f, 0.22f, 1.0f};
}

core::Color textPrimary() {
    return {0.94f, 0.97f, 1.0f, 1.0f};
}

core::Color textMuted() {
    return {0.62f, 0.70f, 0.82f, 1.0f};
}

core::Color accentForPage(int page) {
    if (page == 1) {
        return {0.88f, 0.42f, 0.58f, 1.0f};
    }
    if (page == 2) {
        return {0.38f, 0.68f, 0.96f, 1.0f};
    }
    if (page == 3) {
        return {0.52f, 0.72f, 0.36f, 1.0f};
    }
    if (page == 4) {
        return {0.86f, 0.64f, 0.34f, 1.0f};
    }
    return {0.26f, 0.58f, 0.94f, 1.0f};
}

core::Color accent() {
    return accentForPage(selectedPage);
}

const char* pageTitle() {
    if (selectedPage == 1) {
        return "Text Gallery";
    }
    if (selectedPage == 2) {
        return "Animation";
    }
    if (selectedPage == 3) {
        return "Settings";
    }
    if (selectedPage == 4) {
        return "About";
    }
    return "Controls";
}

const char* pageSubtitle() {
    if (selectedPage == 1) {
        return "Different text sizes, weights, alignment and icon text.";
    }
    if (selectedPage == 2) {
        return "Click and hover samples driven by DSL transitions.";
    }
    if (selectedPage == 3) {
        return "Interactive settings built with the same rect and text primitives.";
    }
    if (selectedPage == 4) {
        return "A small map of the current EUI DSL runtime and component model.";
    }
    return "Basic controls, states and visual properties in one surface.";
}

void caption(core::dsl::Ui& ui, const std::string& id, const std::string& text, float width, float y) {
    ui.text(id)
        .y(y)
        .size(width, 24.0f)
        .text(text)
        .fontSize(16.0f)
        .lineHeight(22.0f)
        .color(textMuted())
        .horizontalAlign(core::HorizontalAlign::Center)
        .build();
}

void navItem(core::dsl::Ui& ui, const std::string& id, const std::string& label, unsigned int icon, int page) {
    const bool active = selectedPage == page;
    components::button(ui, id)
        .size(212.0f, 50.0f)
        .icon(icon)
        .iconSize(16.0f)
        .fontSize(17.0f)
        .text(label)
        .colors(active ? accentForPage(page) : core::Color{0.12f, 0.14f, 0.18f, 1.0f},
                active ? core::Color{0.36f, 0.68f, 0.98f, 1.0f} : core::Color{0.17f, 0.20f, 0.26f, 1.0f},
                active ? core::Color{0.12f, 0.30f, 0.54f, 1.0f} : core::Color{0.08f, 0.10f, 0.14f, 1.0f})
        .radius(12.0f)
        .transition(pageTransition())
        .onClick([page] {
            selectedPage = page;
        })
        .build();
}

void composeSidebar(core::dsl::Ui& ui, float height) {
    ui.stack("sidebar")
        .size(kSidebarWidth, height)
        .content([&] {
            ui.rect("sidebar.bg")
                .size(kSidebarWidth, height)
                .color({0.055f, 0.065f, 0.085f, 1.0f})
                .border(1.0f, {0.18f, 0.22f, 0.28f, 1.0f})
                .build();

            ui.rect("sidebar.accent")
                .x(0.0f)
                .y(kNavTop + selectedPage * (kNavHeight + kNavGap))
                .size(4.0f, 50.0f)
                .color(accent())
                .radius(2.0f)
                .transition(pageTransition())
                .build();

            ui.column("sidebar.content")
                .size(kSidebarWidth, std::max(0.0f, height - 42.0f))
                .margin(0.0f, 30.0f, 0.0f, 0.0f)
                .gap(14.0f)
                .alignItems(core::Align::CENTER)
                .content([&] {
                    ui.text("brand.icon")
                        .size(212.0f, 34.0f)
                        .icon(0xF5FD)
                        .fontSize(27.0f)
                        .lineHeight(32.0f)
                        .color(accent())
                        .transition(pageTransition())
                        .horizontalAlign(core::HorizontalAlign::Center)
                        .build();

                    ui.text("brand.title")
                        .size(212.0f, 36.0f)
                        .text("EUI Gallery")
                        .customFont("YouSheBiaoTiHei")
                        .fontSize(30.0f)
                        .lineHeight(34.0f)
                        .color(textPrimary())
                        .horizontalAlign(core::HorizontalAlign::Center)
                        .build();

                    navItem(ui, "nav.controls", "Controls", 0xF1B2, 0);
                    navItem(ui, "nav.text", "Text", 0xF031, 1);
                    navItem(ui, "nav.animation", "Animation", 0xF2F1, 2);
                    navItem(ui, "nav.settings", "Settings", 0xF013, 3);
                    navItem(ui, "nav.about", "About", 0xF05A, 4);
                });
        });
}

void propertyCard(core::dsl::Ui& ui, const std::string& id, const std::string& title, const std::string& note,
                  const core::Color& color, const std::string& kind, float width) {
    ui.stack(id)
        .size(width, 144.0f)
        .visualStateFrom(id + ".bg", 0.95f)
        .content([&] {
            auto rect = ui.rect(id + ".bg")
                .size(width, 144.0f)
                .states(color, {color.r + 0.08f, color.g + 0.08f, color.b + 0.08f, color.a}, {color.r * 0.62f, color.g * 0.62f, color.b * 0.62f, color.a})
                .radius(18.0f)
                .transition(pageTransition());

            if (kind == "border") {
                rect.border(3.0f, {0.62f, 0.82f, 1.0f, 1.0f});
            } else if (kind == "shadow") {
                rect.shadow(28.0f, 0.0f, 12.0f, {0.0f, 0.0f, 0.0f, 0.34f});
            } else if (kind == "blur") {
                rect.opacity(optionGlass ? 1.0f : 0.82f)
                    .blur(optionGlass ? 18.0f : 0.0f)
                    .border(1.0f, {1.0f, 1.0f, 1.0f, optionGlass ? 0.35f : 0.18f});
            } else if (kind == "rotate") {
                rect.rotate(0.08f).transformOrigin(0.5f, 0.5f);
            }
            rect.build();

            ui.text(id + ".title")
                .size(width, 32.0f)
                .margin(0.0f, 36.0f, 0.0f, 0.0f)
                .text(title)
                .fontSize(22.0f)
                .lineHeight(28.0f)
                .color(textPrimary())
                .horizontalAlign(core::HorizontalAlign::Center)
                .build();

            caption(ui, id + ".note", note, width, 90.0f);
        })
        .build();
}

void composeControlsPage(core::dsl::Ui& ui, float width, float height) {
    const float cardGap = 18.0f;
    const float cardWidth = std::max(90.0f, std::min(204.0f, (width - cardGap * 2.0f) / 3.0f));
    const float rowWidth = cardWidth * 3.0f + cardGap * 2.0f;
    const float rowHeight = 144.0f;
    const float buttonWidth = std::max(92.0f, std::min(178.0f, (width - 36.0f) / 3.0f));

    ui.row("controls.buttons")
        .size(buttonWidth * 3.0f + 36.0f, 78.0f)
        .gap(18.0f)
        .content([&] {
            components::button(ui, "control.primary")
                .size(buttonWidth, 54.0f)
                .icon(0xF00C)
                .text("Primary")
                .transition(pageTransition())
                .build();

            components::button(ui, "control.soft")
                .size(buttonWidth, 54.0f)
                .icon(0xF0C8)
                .text("Soft")
                .colors(surfaceSoft(), {0.22f, 0.28f, 0.36f, 1.0f}, {0.09f, 0.11f, 0.15f, 1.0f})
                .transition(pageTransition())
                .build();

            components::button(ui, "control.warn")
                .size(buttonWidth, 54.0f)
                .icon(0xF071)
                .text("Warning")
                .colors({0.76f, 0.48f, 0.20f, 1.0f}, {0.92f, 0.62f, 0.30f, 1.0f}, {0.46f, 0.24f, 0.08f, 1.0f})
                .transition(pageTransition())
                .build();
        });

    ui.row("properties.a")
        .size(rowWidth, rowHeight)
        .gap(cardGap)
        .content([&] {
            propertyCard(ui, "prop.color", "Color", "hover + press", {0.22f, 0.48f, 0.82f, 1.0f}, "color", cardWidth);
            propertyCard(ui, "prop.border", "Border", "animated edge", {0.10f, 0.12f, 0.16f, 1.0f}, "border", cardWidth);
            propertyCard(ui, "prop.shadow", "Shadow", "elevation", {0.18f, 0.22f, 0.30f, 1.0f}, "shadow", cardWidth);
        });

    ui.row("properties.b")
        .size(rowWidth, rowHeight)
        .gap(cardGap)
        .content([&] {
            propertyCard(ui, "prop.alpha", "Opacity", "transparent fill", {0.86f, 0.38f, 0.52f, 0.58f}, "color", cardWidth);
            propertyCard(ui, "prop.blur", "Blur", "glass card", {0.78f, 0.92f, 1.0f, 0.22f}, "blur", cardWidth);
            propertyCard(ui, "prop.rotate", "Rotate", "transform", {0.48f, 0.64f, 0.36f, 1.0f}, "rotate", cardWidth);
        });
}

void textSample(core::dsl::Ui& ui, const std::string& id, const std::string& text, float size, float height, float width, const core::Color& color) {
    ui.text(id)
        .size(width, height)
        .text(text)
        .fontSize(size)
        .lineHeight(height)
        .color(color)
        .transition(pageTransition())
        .build();
}

void composeTextPage(core::dsl::Ui& ui, float width, float height) {
    const float textWidth = std::max(240.0f, std::min(width, 760.0f));
    const float iconGap = 20.0f;
    const float iconCardWidth = std::max(60.0f, std::min(120.0f, (width - iconGap * 3.0f) / 4.0f));
    const float iconRowWidth = iconCardWidth * 4.0f + iconGap * 3.0f;

    ui.column("text.samples")
        .size(width, std::min(height, 380.0f))
        .gap(12.0f)
        .content([&] {
            textSample(ui, "txt.display", "Display 48 - Gallery Title", 48.0f, 58.0f, textWidth, textPrimary());
            textSample(ui, "txt.h1", "Heading 36 - Section Header", 36.0f, 46.0f, textWidth, {0.86f, 0.92f, 1.0f, 1.0f});
            textSample(ui, "txt.h2", "Heading 28 - Component Name", 28.0f, 38.0f, textWidth, {0.78f, 0.86f, 1.0f, 1.0f});
            textSample(ui, "txt.body", "Body 20 - Text can wrap, align and use custom colors.", 20.0f, 30.0f, textWidth, textMuted());
            textSample(ui, "txt.small", "Small 15 - Secondary metadata and compact labels.", 15.0f, 24.0f, textWidth, {0.50f, 0.58f, 0.70f, 1.0f});

            ui.row("text.icons")
                .size(iconRowWidth, 74.0f)
                .gap(iconGap)
                .content([&] {
                    const unsigned int icons[] = {0xF015, 0xF1FC, 0xF013, 0xF05A};
                    const char* names[] = {"Home", "Theme", "Settings", "Info"};
                    for (int i = 0; i < 4; ++i) {
                        ui.stack(std::string("text.icon.card.") + std::to_string(i))
                            .size(iconCardWidth, 72.0f)
                            .content([&, i] {
                                ui.text(std::string("text.icon.") + std::to_string(i))
                                    .size(iconCardWidth, 36.0f)
                                    .icon(icons[i])
                                    .fontSize(28.0f)
                                    .lineHeight(34.0f)
                                    .color(accent())
                                    .horizontalAlign(core::HorizontalAlign::Center)
                                    .transition(pageTransition())
                                    .build();

                                caption(ui, std::string("text.icon.label.") + std::to_string(i), names[i], iconCardWidth, 40.0f);
                            })
                            .build();
                    }
                });
        });
}

void composeAnimationPage(core::dsl::Ui& ui, float width, float height) {
    const float stageWidth = std::max(260.0f, std::min(width, 820.0f));
    const float stageHeight = std::min(std::max(190.0f, height - 88.0f), 250.0f);
    const float actorWidth = animationRotated ? 150.0f : 118.0f;
    const float actorHeight = animationRotated ? 96.0f : 72.0f;
    const float actorTravel = std::max(46.0f, stageWidth - actorWidth - 56.0f);
    const float buttonWidth = std::max(92.0f, std::min(166.0f, (width - 36.0f) / 3.0f));

    ui.row("animation.controls")
        .size(buttonWidth * 3.0f + 36.0f, 66.0f)
        .gap(18.0f)
        .content([&] {
            components::button(ui, "anim.move")
                .size(buttonWidth, 50.0f)
                .text("Move")
                .colors(animationMoved ? accent() : surfaceSoft(), {0.34f, 0.64f, 0.96f, 1.0f}, {0.10f, 0.24f, 0.44f, 1.0f})
                .onClick([] { animationMoved = !animationMoved; })
                .transition(pageTransition())
                .build();

            components::button(ui, "anim.rotate")
                .size(buttonWidth, 50.0f)
                .text("Rotate")
                .colors(animationRotated ? core::Color{0.84f, 0.46f, 0.60f, 1.0f} : surfaceSoft(),
                        {0.96f, 0.54f, 0.68f, 1.0f},
                        {0.46f, 0.18f, 0.30f, 1.0f})
                .onClick([] { animationRotated = !animationRotated; })
                .transition(pageTransition())
                .build();

            components::button(ui, "anim.fade")
                .size(buttonWidth, 50.0f)
                .text("Fade")
                .colors(animationFaded ? core::Color{0.50f, 0.72f, 0.34f, 1.0f} : surfaceSoft(),
                        {0.62f, 0.84f, 0.44f, 1.0f},
                        {0.24f, 0.42f, 0.18f, 1.0f})
                .onClick([] { animationFaded = !animationFaded; })
                .transition(pageTransition())
                .build();
        });

    ui.stack("animation.stage")
        .size(stageWidth, stageHeight)
        .content([&] {
            ui.rect("animation.stage.bg")
                .size(stageWidth, stageHeight)
                .color({0.09f, 0.11f, 0.15f, 1.0f})
                .radius(24.0f)
                .border(1.0f, {0.22f, 0.28f, 0.36f, 1.0f})
                .build();

            ui.rect("animation.actor")
                .x(animationMoved ? actorTravel : 46.0f)
                .y(animationMoved ? 58.0f : 74.0f)
                .size(actorWidth, actorHeight)
                .color(animationMoved ? accent() : core::Color{0.32f, 0.62f, 0.92f, 1.0f})
                .radius(animationRotated ? 30.0f : 18.0f)
                .rotate(animationRotated ? 0.42f : 0.0f)
                .opacity(animationFaded ? 0.36f : 1.0f)
                .shadow(26.0f, 0.0f, 12.0f, {0.0f, 0.0f, 0.0f, 0.32f})
                .transition(motionTransition())
                .build();
        });
}

void settingRow(core::dsl::Ui& ui, const std::string& id, const std::string& title, const std::string& note, bool enabled, float width, const std::function<void()>& onClick) {
    const float toggleX = std::max(0.0f, width - 80.0f);
    const float textWidth = std::max(0.0f, width - 132.0f);

    ui.stack(id)
        .size(width, 72.0f)
        .visualStateFrom(id + ".hit", 0.985f)
        .content([&] {
            ui.rect(id + ".hit")
                .size(width, 72.0f)
                .states(surfaceSoft(), {0.20f, 0.24f, 0.30f, 1.0f}, {0.09f, 0.11f, 0.15f, 1.0f})
                .radius(16.0f)
                .transition(pageTransition())
                .onClick(onClick)
                .build();

            ui.text(id + ".title")
                .x(24.0f)
                .y(12.0f)
                .size(textWidth, 28.0f)
                .text(title)
                .fontSize(20.0f)
                .lineHeight(26.0f)
                .color(textPrimary())
                .build();

            ui.text(id + ".note")
                .x(24.0f)
                .y(42.0f)
                .size(textWidth, 22.0f)
                .text(note)
                .fontSize(15.0f)
                .lineHeight(20.0f)
                .color(textMuted())
                .build();

            ui.rect(id + ".toggle")
                .x(toggleX)
                .y(22.0f)
                .size(46.0f, 26.0f)
                .color(enabled ? accent() : core::Color{0.26f, 0.30f, 0.36f, 1.0f})
                .radius(13.0f)
                .transition(pageTransition())
                .build();

            ui.rect(id + ".knob")
                .x(enabled ? toggleX + 22.0f : toggleX + 4.0f)
                .y(26.0f)
                .size(18.0f, 18.0f)
                .color({0.96f, 0.98f, 1.0f, 1.0f})
                .radius(9.0f)
                .transition(pageTransition())
                .build();
        })
        .build();
}

void composeSettingsPage(core::dsl::Ui& ui, float width, float height) {
    const float rowWidth = std::max(0.0f, std::min(width, 720.0f));

    ui.column("settings.list")
        .size(rowWidth, std::min(height, 260.0f))
        .gap(14.0f)
        .content([&] {
            settingRow(ui, "setting.dense", "Dense layout", "Use tighter spacing for gallery pages.", optionDense, rowWidth, [] { optionDense = !optionDense; });
            settingRow(ui, "setting.glass", "Glass surfaces", "Show transparent panel examples in controls.", optionGlass, rowWidth, [] { optionGlass = !optionGlass; });
            settingRow(ui, "setting.motion", "Animated transitions", "Keep page and property transitions enabled.", optionMotion, rowWidth, [] { optionMotion = !optionMotion; });
        });
}

void composeAboutPage(core::dsl::Ui& ui, float width, float height) {
    const float leadWidth = std::max(240.0f, std::min(width, 760.0f));
    const float cardGap = 18.0f;
    const float cardWidth = std::max(90.0f, std::min(204.0f, (width - cardGap * 2.0f) / 3.0f));
    const float rowWidth = cardWidth * 3.0f + cardGap * 2.0f;

    ui.column("about.body")
        .size(width, std::min(height, 340.0f))
        .gap(18.0f)
        .content([&] {
            ui.text("about.lead")
                .size(leadWidth, 70.0f)
                .text("This gallery is built from Row, Column, Stack, Rect and Text. Components are thin DSL helpers, while Runtime owns layout, interaction and animation.")
                .fontSize(22.0f)
                .lineHeight(32.0f)
                .maxWidth(leadWidth)
                .wrap(true)
                .color(textPrimary())
                .build();

            ui.row("about.cards")
                .size(rowWidth, 150.0f)
                .gap(cardGap)
                .content([&] {
                    propertyCard(ui, "about.dsl", "DSL", "declarative UI", {0.22f, 0.48f, 0.82f, 1.0f}, "border", cardWidth);
                    propertyCard(ui, "about.runtime", "Runtime", "layout + render", {0.54f, 0.38f, 0.74f, 1.0f}, "shadow", cardWidth);
                    propertyCard(ui, "about.anim", "Animation", "target values", {0.48f, 0.68f, 0.36f, 1.0f}, "rotate", cardWidth);
                });
        });
}

void composePageBody(core::dsl::Ui& ui, float width, float height) {
    if (selectedPage == 0) {
        composeControlsPage(ui, width, height);
    } else if (selectedPage == 1) {
        composeTextPage(ui, width, height);
    } else if (selectedPage == 2) {
        composeAnimationPage(ui, width, height);
    } else if (selectedPage == 3) {
        composeSettingsPage(ui, width, height);
    } else {
        composeAboutPage(ui, width, height);
    }
}

void composeContent(core::dsl::Ui& ui, float width, float height) {
    const float shellWidth = std::max(0.0f, width - 72.0f);
    const float innerWidth = std::max(0.0f, shellWidth - 64.0f);
    const float shellHeight = std::max(0.0f, height - 72.0f);
    const float innerHeight = std::max(0.0f, shellHeight - 64.0f);
    const float headerGap = optionDense ? 18.0f : 26.0f;
    const float bodyHeight = std::max(0.0f, innerHeight - 46.0f - 30.0f - headerGap * 2.0f);

    ui.stack("content.area")
        .size(width, height)
        .content([&] {
            ui.rect("content.bg")
                .size(width, height)
                .color(appBg())
                .build();

            ui.rect("page.shell")
                .size(shellWidth, shellHeight)
                .margin(36.0f)
                .color(surface())
                .radius(26.0f)
                .border(1.0f, {0.22f, 0.27f, 0.34f, 1.0f})
                .shadow(30.0f, 0.0f, 16.0f, {0.0f, 0.0f, 0.0f, 0.28f})
                .transition(pageTransition())
                .build();

            ui.column("page.content")
                .size(innerWidth, innerHeight)
                .margin(68.0f)
                .gap(headerGap)
                .content([&] {
                    ui.text("page.title")
                        .size(innerWidth, 46.0f)
                        .text(pageTitle())
                        .customFont("YouSheBiaoTiHei")
                        .fontSize(38.0f)
                        .lineHeight(44.0f)
                        .color(accent())
                        .transition(pageTransition())
                        .build();

                    ui.text("page.subtitle")
                        .size(innerWidth, 30.0f)
                        .text(pageSubtitle())
                        .fontSize(20.0f)
                        .lineHeight(28.0f)
                        .color(textMuted())
                        .transition(pageTransition())
                        .build();

                    ui.column("page.body")
                        .size(innerWidth, bodyHeight)
                        .gap(headerGap)
                        .content([&] {
                            composePageBody(ui, innerWidth, bodyHeight);
                        })
                        .build();
                });
        });
}

} // namespace

const DslAppConfig& dslAppConfig() {
    static const DslAppConfig config = {
        "EUI Gallery",
        "gallery",
        {0.07f, 0.08f, 0.10f, 1.0f},
        1440,
        900
    };
    return config;
}

void compose(core::dsl::Ui& ui, const core::dsl::Screen& screen) {
    const float contentWidth = std::max(0.0f, screen.width - kSidebarWidth);

    ui.row("root")
        .size(screen.width, screen.height)
        .content([&] {
            composeSidebar(ui, screen.height);
            composeContent(ui, contentWidth, screen.height);
        });
}

} // namespace app
