#include "app/dsl_app.h"

#include "components/components.h"
#include "core/network.h"
#include "core/platform.h"

#include <algorithm>
#include <functional>
#include <string>

namespace app {

namespace {

int selectedPage = 0;
bool optionDense = false;
bool optionGlass = false;
bool optionMotion = true;
bool optionLimit60 = true;
bool optionNight = true;
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

core::Transition textTransition() {
    core::Transition transition = pageTransition();
    if (transition.enabled) {
        transition.animate(core::AnimProperty::TextColor | core::AnimProperty::Opacity);
    }
    return transition;
}

core::Transition motionTransition() {
    if (!optionMotion) {
        return core::Transition::none();
    }
    return core::Transition::make(0.42f, core::Ease::OutBack);
}

double galleryFrameRateLimit() {
    return optionLimit60 ? 60.0 : 0.0;
}

components::theme::ThemeColorTokens themeColors() {
    return optionNight ? components::theme::DarkThemeColors() : components::theme::LightThemeColors();
}

components::theme::PageVisualTokens pageVisuals() {
    return components::theme::pageVisuals(themeColors());
}

core::Color withAlpha(core::Color color, float alpha) {
    return components::theme::withAlpha(color, alpha);
}

core::Color mixTheme(core::Color from, core::Color to, float amount) {
    return core::mixColor(from, to, amount);
}

core::Color appBg() {
    return themeColors().background;
}

core::Color surface() {
    return themeColors().surface;
}

core::Color surfaceSoft() {
    return themeColors().surfaceHover;
}

core::Color surfaceActive() {
    return themeColors().surfaceActive;
}

core::Color textPrimary() {
    return pageVisuals().titleColor;
}

core::Color textMuted() {
    return pageVisuals().subtitleColor;
}

core::Color bodyText() {
    return pageVisuals().bodyColor;
}

core::Color borderColor(float alpha = 1.0f) {
    return components::theme::withOpacity(themeColors().border, alpha);
}

core::Color shadowColor(float darkAlpha = 0.28f, float lightAlpha = 0.12f) {
    return optionNight
        ? core::Color{0.0f, 0.0f, 0.0f, darkAlpha}
        : core::Color{0.10f, 0.14f, 0.22f, lightAlpha};
}

core::Color buttonHover(const core::Color& base) {
    return mixTheme(base, optionNight ? core::Color{1.0f, 1.0f, 1.0f, base.a} : themeColors().primary, optionNight ? 0.16f : 0.10f);
}

core::Color buttonPressed(const core::Color& base) {
    return mixTheme(base, optionNight ? core::Color{0.0f, 0.0f, 0.0f, base.a} : themeColors().surfaceActive, optionNight ? 0.34f : 0.22f);
}

core::Color accentForPage(int page) {
    if (page == 0) {
        return themeColors().primary;
    }
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
    if (page == 5) {
        return {0.62f, 0.50f, 0.88f, 1.0f};
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
        return "Bing";
    }
    if (selectedPage == 5) {
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
        return "Network images, SVG rasterization and API text requests.";
    }
    if (selectedPage == 5) {
        return "A lightweight and elegant C++ GUI framework.";
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
    const core::Color normal = active ? accentForPage(page) : surface();
    const core::Color hover = active ? buttonHover(accentForPage(page)) : surfaceSoft();
    const core::Color pressed = active ? buttonPressed(accentForPage(page)) : surfaceActive();
    components::button(ui, id)
        .size(212.0f, 50.0f)
        .icon(icon)
        .iconSize(16.0f)
        .fontSize(17.0f)
        .text(label)
        .colors(normal, hover, pressed)
        .textColor(active || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
        .iconColor(active || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
        .radius(12.0f)
        .border(1.0f, active ? withAlpha(accentForPage(page), 0.58f) : borderColor(0.60f))
        .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
        .transition(pageTransition())
        .onClick([page] {
            selectedPage = page;
        })
        .build();
}

void composeSidebar(core::dsl::Ui& ui, float height) {
    const core::Color sidebarBg = optionNight ? mixTheme(appBg(), core::Color{0.0f, 0.0f, 0.0f, 1.0f}, 0.24f) : surface();
    ui.stack("sidebar")
        .size(kSidebarWidth, height)
        .content([&] {
            ui.rect("sidebar.bg")
                .size(kSidebarWidth, height)
                .color(sidebarBg)
                .border(1.0f, borderColor(0.80f))
                .build();

            ui.rect("sidebar.accent")
                .x(0.0f)
                .y(kNavTop)
                .size(4.0f, 50.0f)
                .color(accent())
                .radius(2.0f)
                .translateY(selectedPage * (kNavHeight + kNavGap))
                .transition(pageTransition())
                .animate(core::AnimProperty::Transform | core::AnimProperty::Color)
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
                        .transition(textTransition())
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
                    navItem(ui, "nav.bing", "Bing", 0xF1C5, 4);
                    navItem(ui, "nav.about", "About", 0xF05A, 5);
                });

            ui.stack("sidebar.theme")
                .x(30.0f)
                .y(std::max(0.0f, height - 82.0f))
                .size(212.0f, 50.0f)
                .content([&] {
                    components::button(ui, "nav.theme")
                        .size(212.0f, 50.0f)
                        .icon(optionNight ? 0xF185 : 0xF186)
                        .iconSize(16.0f)
                        .fontSize(17.0f)
                        .text(optionNight ? "Light Mode" : "Night Mode")
                        .colors(surface(), surfaceSoft(), surfaceActive())
                        .textColor(textPrimary())
                        .iconColor(accent())
                        .radius(12.0f)
                        .border(1.0f, borderColor(0.80f))
                        .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
                        .transition(pageTransition())
                        .onClick([] {
                            optionNight = !optionNight;
                        })
                        .build();
                })
                .build();
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
                .states(color, buttonHover(color), buttonPressed(color))
                .radius(18.0f)
                .transition(pageTransition());

            if (kind == "border") {
                rect.border(3.0f, accent());
            } else if (kind == "shadow") {
                rect.shadow(28.0f, 0.0f, 12.0f, shadowColor(0.34f, 0.18f));
            } else if (kind == "blur") {
                rect.opacity(optionGlass ? 1.0f : 0.82f)
                    .blur(optionGlass ? 18.0f : 0.0f)
                    .border(1.0f, optionGlass ? withAlpha(textPrimary(), 0.35f) : borderColor(0.70f));
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

void imageCard(core::dsl::Ui& ui, const std::string& id, const std::string& title, const std::string& source,
               float width, float height = 122.0f, float imageHeight = 72.0f) {
    const float labelY = std::max(14.0f, height - 30.0f);
    ui.stack(id)
        .size(width, height)
        .content([&] {
            ui.rect(id + ".bg")
                .size(width, height)
                .color(surface())
                .radius(18.0f)
                .border(1.0f, borderColor())
                .build();

            ui.image(id + ".image")
                .x(14.0f)
                .y(14.0f)
                .size(std::max(0.0f, width - 28.0f), imageHeight)
                .source(source)
                .radius(12.0f)
                .transition(pageTransition())
                .build();

            ui.text(id + ".label")
                .x(14.0f)
                .y(labelY)
                .size(std::max(0.0f, width - 28.0f), 22.0f)
                .text(title)
                .fontSize(15.0f)
                .lineHeight(20.0f)
                .color(textMuted())
                .horizontalAlign(core::HorizontalAlign::Center)
                .build();
        })
        .build();
}

void bingImageCard(core::dsl::Ui& ui, const std::string& id, float width,
                   float height = 122.0f, float imageHeight = 72.0f) {
    const float labelY = std::max(14.0f, height - 30.0f);
    ui.stack(id)
        .size(width, height)
        .content([&] {
            ui.rect(id + ".bg")
                .size(width, height)
                .color(surface())
                .radius(18.0f)
                .border(1.0f, borderColor())
                .build();

            ui.image(id + ".image")
                .x(14.0f)
                .y(14.0f)
                .size(std::max(0.0f, width - 28.0f), imageHeight)
                .bingDaily(0, "zh-CN")
                .radius(12.0f)
                .transition(pageTransition())
                .build();

            ui.text(id + ".label")
                .x(14.0f)
                .y(labelY)
                .size(std::max(0.0f, width - 28.0f), 22.0f)
                .text("Bing daily")
                .fontSize(15.0f)
                .lineHeight(20.0f)
                .color(textMuted())
                .horizontalAlign(core::HorizontalAlign::Center)
                .build();
        })
        .build();
}

void mediaThumb(core::dsl::Ui& ui, const std::string& id, const std::string& title, const std::string& source, float width, bool bing = false) {
    ui.stack(id)
        .size(width, 88.0f)
        .content([&] {
            ui.rect(id + ".bg")
                .size(width, 88.0f)
                .color(surface())
                .radius(12.0f)
                .border(1.0f, borderColor())
                .build();

            auto image = ui.image(id + ".image")
                .x(8.0f)
                .y(8.0f)
                .size(std::max(0.0f, width - 16.0f), 50.0f)
                .radius(8.0f)
                .cover();
            if (bing) {
                image.bingDaily(0, "zh-CN");
            } else {
                image.source(source);
            }
            image.build();

            ui.text(id + ".label")
                .x(8.0f)
                .y(62.0f)
                .size(std::max(0.0f, width - 16.0f), 18.0f)
                .text(title)
                .fontSize(13.0f)
                .lineHeight(16.0f)
                .color(textMuted())
                .horizontalAlign(core::HorizontalAlign::Center)
                .build();
        })
        .build();
}

std::string jsonStringValue(const std::string& json, const std::string& key) {
    const std::string token = "\"" + key + "\":\"";
    const size_t begin = json.find(token);
    if (begin == std::string::npos) {
        return {};
    }
    const size_t valueBegin = begin + token.size();
    std::string value;
    bool escaping = false;
    for (size_t i = valueBegin; i < json.size(); ++i) {
        const char ch = json[i];
        if (escaping) {
            value.push_back(ch == '/' ? '/' : ch);
            escaping = false;
            continue;
        }
        if (ch == '\\') {
            escaping = true;
            continue;
        }
        if (ch == '"') {
            break;
        }
        value.push_back(ch);
    }
    return value;
}

std::string bingApiText() {
    const std::string key = "gallery.bing.text";
    core::network::requestText(key, "https://www.bing.com/HPImageArchive.aspx?format=js&n=1&idx=0&mkt=zh-CN");
    const core::network::TextResult result = core::network::textResult(key);
    if (!result.ready) {
        return "Loading Bing API text...";
    }
    if (!result.ok) {
        return "Network text request failed.";
    }
    const std::string copyright = jsonStringValue(result.body, "copyright");
    return copyright.empty() ? "Bing API returned text data." : copyright;
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
                .colors(themeColors().primary, buttonHover(themeColors().primary), buttonPressed(themeColors().primary))
                .border(1.0f, withAlpha(themeColors().primary, 0.58f))
                .shadow(14.0f, 0.0f, 5.0f, shadowColor(0.22f, 0.10f))
                .transition(pageTransition())
                .build();

            components::button(ui, "control.soft")
                .size(buttonWidth, 54.0f)
                .icon(0xF0C8)
                .text("Soft")
                .colors(surfaceSoft(), buttonHover(surfaceSoft()), buttonPressed(surfaceSoft()))
                .textColor(textPrimary())
                .iconColor(textPrimary())
                .border(1.0f, borderColor(0.70f))
                .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
                .transition(pageTransition())
                .build();

            components::button(ui, "control.warn")
                .size(buttonWidth, 54.0f)
                .icon(0xF071)
                .text("Warning")
                .colors({0.76f, 0.48f, 0.20f, 1.0f}, {0.92f, 0.62f, 0.30f, 1.0f}, {0.46f, 0.24f, 0.08f, 1.0f})
                .border(1.0f, borderColor(0.70f))
                .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
                .transition(pageTransition())
                .build();
        });

    ui.row("properties.a")
        .size(rowWidth, rowHeight)
        .gap(cardGap)
        .content([&] {
            propertyCard(ui, "prop.color", "Color", "hover + press", {0.22f, 0.48f, 0.82f, 1.0f}, "color", cardWidth);
            propertyCard(ui, "prop.border", "Border", "animated edge", surface(), "border", cardWidth);
            propertyCard(ui, "prop.shadow", "Shadow", "elevation", surfaceSoft(), "shadow", cardWidth);
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
        .transition(textTransition())
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
            textSample(ui, "txt.h1", "Heading 36 - Section Header", 36.0f, 46.0f, textWidth, withAlpha(textPrimary(), 0.92f));
            textSample(ui, "txt.h2", "Heading 28 - Component Name", 28.0f, 38.0f, textWidth, withAlpha(textPrimary(), 0.82f));
            textSample(ui, "txt.body", "Body 20 - Text can wrap, align and use custom colors.", 20.0f, 30.0f, textWidth, bodyText());
            textSample(ui, "txt.small", "Small 15 - Secondary metadata and compact labels.", 15.0f, 24.0f, textWidth, withAlpha(textPrimary(), 0.58f));

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
                                    .transition(textTransition())
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
                .colors(animationMoved ? accent() : surfaceSoft(),
                        buttonHover(animationMoved ? accent() : surfaceSoft()),
                        buttonPressed(animationMoved ? accent() : surfaceSoft()))
                .textColor(animationMoved || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
                .iconColor(animationMoved || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
                .border(1.0f, animationMoved ? withAlpha(accent(), 0.58f) : borderColor(0.70f))
                .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
                .onClick([] { animationMoved = !animationMoved; })
                .transition(pageTransition())
                .build();

            components::button(ui, "anim.rotate")
                .size(buttonWidth, 50.0f)
                .text("Rotate")
                .colors(animationRotated ? core::Color{0.84f, 0.46f, 0.60f, 1.0f} : surfaceSoft(),
                        buttonHover(animationRotated ? core::Color{0.84f, 0.46f, 0.60f, 1.0f} : surfaceSoft()),
                        buttonPressed(animationRotated ? core::Color{0.84f, 0.46f, 0.60f, 1.0f} : surfaceSoft()))
                .textColor(animationRotated || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
                .iconColor(animationRotated || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
                .border(1.0f, animationRotated ? withAlpha(core::Color{0.84f, 0.46f, 0.60f, 1.0f}, 0.58f) : borderColor(0.70f))
                .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
                .onClick([] { animationRotated = !animationRotated; })
                .transition(pageTransition())
                .build();

            components::button(ui, "anim.fade")
                .size(buttonWidth, 50.0f)
                .text("Fade")
                .colors(animationFaded ? core::Color{0.50f, 0.72f, 0.34f, 1.0f} : surfaceSoft(),
                        buttonHover(animationFaded ? core::Color{0.50f, 0.72f, 0.34f, 1.0f} : surfaceSoft()),
                        buttonPressed(animationFaded ? core::Color{0.50f, 0.72f, 0.34f, 1.0f} : surfaceSoft()))
                .textColor(animationFaded || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
                .iconColor(animationFaded || optionNight ? core::Color{0.94f, 0.97f, 1.0f, 1.0f} : textPrimary())
                .border(1.0f, animationFaded ? withAlpha(core::Color{0.50f, 0.72f, 0.34f, 1.0f}, 0.58f) : borderColor(0.70f))
                .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
                .onClick([] { animationFaded = !animationFaded; })
                .transition(pageTransition())
                .build();
        });

    ui.stack("animation.stage")
        .size(stageWidth, stageHeight)
        .content([&] {
            ui.rect("animation.stage.bg")
                .size(stageWidth, stageHeight)
                .color(surface())
                .radius(24.0f)
                .border(1.0f, borderColor())
                .build();

            ui.rect("animation.actor")
                .x(animationMoved ? actorTravel : 46.0f)
                .y(animationMoved ? 58.0f : 74.0f)
                .size(actorWidth, actorHeight)
                .color(animationMoved ? accent() : core::Color{0.32f, 0.62f, 0.92f, 1.0f})
                .radius(animationRotated ? 30.0f : 18.0f)
                .rotate(animationRotated ? 0.42f : 0.0f)
                .opacity(animationFaded ? 0.36f : 1.0f)
                .shadow(26.0f, 0.0f, 12.0f, shadowColor(0.32f, 0.16f))
                .transition(motionTransition())
                .animate(core::AnimProperty::Frame | core::AnimProperty::Color | core::AnimProperty::Opacity |
                         core::AnimProperty::Radius | core::AnimProperty::Shadow | core::AnimProperty::Transform)
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
                .states(surfaceSoft(), buttonHover(surfaceSoft()), buttonPressed(surfaceSoft()))
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
                .color(enabled ? accent() : surfaceActive())
                .radius(13.0f)
                .transition(pageTransition())
                .build();

            ui.rect(id + ".knob")
                .x(enabled ? toggleX + 22.0f : toggleX + 4.0f)
                .y(26.0f)
                .size(18.0f, 18.0f)
                .color(optionNight ? core::Color{0.96f, 0.98f, 1.0f, 1.0f} : core::Color{1.0f, 1.0f, 1.0f, 1.0f})
                .radius(9.0f)
                .transition(pageTransition())
                .build();
        })
        .build();
}

void composeSettingsPage(core::dsl::Ui& ui, float width, float height) {
    const float rowWidth = std::max(0.0f, std::min(width, 720.0f));

    ui.column("settings.list")
        .size(rowWidth, std::min(height, 430.0f))
        .gap(14.0f)
        .content([&] {
            settingRow(ui, "setting.dense", "Dense layout", "Use tighter spacing for gallery pages.", optionDense, rowWidth, [] { optionDense = !optionDense; });
            settingRow(ui, "setting.glass", "Glass surfaces", "Show transparent panel examples in controls.", optionGlass, rowWidth, [] { optionGlass = !optionGlass; });
            settingRow(ui, "setting.motion", "Animated transitions", "Keep page and property transitions enabled.", optionMotion, rowWidth, [] { optionMotion = !optionMotion; });
            settingRow(ui, "setting.limit60", "Limit to 60 FPS", "Cap animation rendering below the display refresh rate.", optionLimit60, rowWidth, [] { optionLimit60 = !optionLimit60; });
            settingRow(ui, "setting.night", "Night mode", "Switch gallery between light and dark theme tokens.", optionNight, rowWidth, [] { optionNight = !optionNight; });
        });
}

void composeBingPage(core::dsl::Ui& ui, float width, float height) {
    const float contentWidth = std::max(260.0f, std::min(width, 820.0f));
    const float cardGap = 18.0f;
    const float cardWidth = std::max(90.0f, std::min(246.0f, (contentWidth - cardGap * 2.0f) / 3.0f));
    const float rowWidth = cardWidth * 3.0f + cardGap * 2.0f;
    const float mediaHeight = 182.0f;
    const float mediaImageHeight = 140.0f;
    const float apiHeight = std::clamp(height - 250.0f, 72.0f, 112.0f);

    ui.column("bing.body")
        .size(width, height)
        .alignItems(core::Align::CENTER)
        .gap(18.0f)
        .content([&] {
            ui.row("bing.media")
                .size(rowWidth, mediaHeight)
                .gap(cardGap)
                .content([&] {
                    imageCard(ui, "bing.media.png", "Local PNG", "assets/icon.png", cardWidth, mediaHeight, mediaImageHeight);
                    imageCard(ui, "bing.media.svg", "Local SVG", "assets/icon.svg", cardWidth, mediaHeight, mediaImageHeight);
                    bingImageCard(ui, "bing.media.daily", cardWidth, mediaHeight, mediaImageHeight);
                });

            ui.stack("bing.api")
                .size(contentWidth, apiHeight)
                .content([&] {
                    ui.rect("bing.api.bg")
                        .size(contentWidth, apiHeight)
                        .color(surface())
                        .radius(18.0f)
                        .border(1.0f, borderColor())
                        .build();

                    ui.text("bing.api.title")
                        .x(22.0f)
                        .y(12.0f)
                        .size(std::max(0.0f, contentWidth - 44.0f), 28.0f)
                        .text("Bing API text")
                        .fontSize(20.0f)
                        .lineHeight(26.0f)
                        .color(textPrimary())
                        .build();

                    ui.text("bing.api.text")
                        .x(22.0f)
                        .y(42.0f)
                        .size(std::max(0.0f, contentWidth - 44.0f), std::max(0.0f, apiHeight - 52.0f))
                        .text(bingApiText())
                        .fontSize(16.0f)
                        .lineHeight(22.0f)
                        .maxWidth(std::max(0.0f, contentWidth - 44.0f))
                        .wrap(true)
                        .color(textMuted())
                        .build();
                })
                .build();
        });
}

void composeAboutPage(core::dsl::Ui& ui, float width, float height) {
    const float contentWidth = std::max(240.0f, std::min(width, 900.0f));
    const float logoSize = 130.0f;
    const float logoImageSize = 130.0f;
    const float buttonGap = 36.0f;
    const float buttonWidth = std::max(170.0f, std::min(330.0f, (contentWidth - buttonGap) * 0.5f));
    const float buttonRowWidth = buttonWidth * 2.0f + buttonGap;
    const float licenseSpacer = 10.0f;

    ui.column("about.body")
        .size(width, height)
        .alignItems(core::Align::CENTER)
        .gap(8.0f)
        .content([&] {
            ui.stack("about.logo")
                .size(logoSize, logoSize)
                .content([&] {
                    ui.rect("about.logo.frame")
                        .size(logoSize, logoSize)
                        .color(surface())
                        .radius(34.0f)
                        .shadow(20.0f, 0.0f, 10.0f, shadowColor(0.24f, 0.12f))
                        .build();

                    ui.image("about.logo.image")
                        .x((logoSize - logoImageSize) * 0.5f)
                        .y((logoSize - logoImageSize) * 0.5f)
                        .size(logoImageSize, logoImageSize)
                        .source("assets/icon.png")
                        .radius(34.0f)
                        .cover()
                        .build();
                })
                .build();

            ui.row("about.actions")
                .size(buttonRowWidth, 58.0f)
                .gap(buttonGap)
                .content([&] {
                    components::button(ui, "about.github")
                        .size(buttonWidth, 58.0f)
                        .icon(0xF0C1)
                        .iconSize(24.0f)
                        .fontSize(22.0f)
                        .text("GitHub")
                        .colors(accent(), buttonHover(accent()), buttonPressed(accent()))
                        .radius(8.0f)
                        .border(1.0f, withAlpha(accent(), 0.58f))
                        .shadow(14.0f, 0.0f, 5.0f, shadowColor(0.22f, 0.10f))
                        .transition(pageTransition())
                        .onClick([] {
                            core::platform::openUrl("https://github.com/sudoevolve/EUI-NEO");
                        })
                        .build();

                    components::button(ui, "about.group")
                        .size(buttonWidth, 58.0f)
                        .icon(0xF0C0)
                        .iconSize(22.0f)
                        .fontSize(22.0f)
                        .text("Join Group")
                        .colors(surfaceSoft(), buttonHover(surfaceSoft()), buttonPressed(surfaceSoft()))
                        .textColor(textPrimary())
                        .iconColor(textPrimary())
                        .radius(8.0f)
                        .border(1.0f, borderColor())
                        .shadow(12.0f, 0.0f, 4.0f, shadowColor(0.18f, 0.08f))
                        .transition(pageTransition())
                        .onClick([] {
                            core::platform::openUrl("https://qm.qq.com/q/kaPB4paOpa");
                        })
                        .build();
                });

            ui.stack("about.spacer")
                .size(1.0f, licenseSpacer)
                .build();

            ui.column("about.license")
                .size(contentWidth, 116.0f)
                .alignItems(core::Align::CENTER)
                .gap(6.0f)
                .content([&] {
                    ui.text("about.license.title")
                        .size(contentWidth, 38.0f)
                        .text("License")
                        .customFont("YouSheBiaoTiHei")
                        .fontSize(32.0f)
                        .lineHeight(36.0f)
                        .color(textPrimary())
                        .horizontalAlign(core::HorizontalAlign::Center)
                        .build();

                    ui.text("about.license.copy")
                        .size(contentWidth, 28.0f)
                        .text("Copyright @2026 SudoEvolve")
                        .customFont("YouSheBiaoTiHei")
                        .fontSize(22.0f)
                        .lineHeight(26.0f)
                        .color(textMuted())
                        .horizontalAlign(core::HorizontalAlign::Center)
                        .build();

                    ui.text("about.license.type")
                        .size(contentWidth, 28.0f)
                        .text("Licensed under apache2.0")
                        .customFont("YouSheBiaoTiHei")
                        .fontSize(20.0f)
                        .lineHeight(25.0f)
                        .color(textMuted())
                        .horizontalAlign(core::HorizontalAlign::Center)
                        .build();

                })
                .build();
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
    } else if (selectedPage == 4) {
        composeBingPage(ui, width, height);
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
                .border(1.0f, borderColor())
                .shadow(30.0f, 0.0f, 16.0f, shadowColor(0.28f, 0.14f))
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
                        .transition(textTransition())
                        .build();

                    ui.text("page.subtitle")
                        .size(innerWidth, 30.0f)
                        .text(pageSubtitle())
                        .fontSize(20.0f)
                        .lineHeight(28.0f)
                        .color(textMuted())
                        .transition(textTransition())
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
        1100,
        false,
        galleryFrameRateLimit
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
