#include "app/dsl_app.h"

#include "core/dsl.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace app {
namespace {

std::string entry = "0";
std::string expression;
double stored = 0.0;
char pendingOp = 0;
bool freshEntry = true;

constexpr core::Color kBg{0.07f, 0.075f, 0.085f, 1.0f};
constexpr core::Color kButtonTop{0.20f, 0.215f, 0.225f, 1.0f};
constexpr core::Color kButtonBottom{0.075f, 0.08f, 0.085f, 1.0f};
constexpr core::Color kText{0.94f, 0.95f, 0.95f, 1.0f};
constexpr core::Color kMuted{0.55f, 0.56f, 0.58f, 1.0f};
constexpr core::Color kClear{0.0f, 0.0f, 0.0f, 0.0f};

std::string trimNumber(double value) {
    if (!std::isfinite(value)) {
        return "Error";
    }
    if (std::fabs(value) < 0.00000001) {
        value = 0.0;
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(8) << value;
    std::string text = out.str();
    while (text.size() > 1 && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text;
}

std::string groupNumber(std::string text) {
    if (text == "Error") {
        return text;
    }

    const bool negative = !text.empty() && text[0] == '-';
    if (negative) {
        text.erase(text.begin());
    }

    const size_t dot = text.find('.');
    std::string whole = dot == std::string::npos ? text : text.substr(0, dot);
    const std::string decimal = dot == std::string::npos ? std::string{} : text.substr(dot);
    for (int i = static_cast<int>(whole.size()) - 3; i > 0; i -= 3) {
        whole.insert(static_cast<size_t>(i), ",");
    }
    return (negative ? "-" : "") + whole + decimal;
}

double value() {
    if (entry == "Error") {
        return 0.0;
    }

    char* end = nullptr;
    const double parsed = std::strtod(entry.c_str(), &end);
    return end && *end == '\0' ? parsed : 0.0;
}

std::string opText(char op) {
    if (op == '*') {
        return core::dsl::utf8(0x00D7);
    }
    if (op == '/') {
        return core::dsl::utf8(0x00F7);
    }
    if (op == '-') {
        return core::dsl::utf8(0x2212);
    }
    return "+";
}

double apply(double left, double right, char op) {
    if (op == '+') {
        return left + right;
    }
    if (op == '-') {
        return left - right;
    }
    if (op == '*') {
        return left * right;
    }
    return std::fabs(right) < 0.00000001 ? std::numeric_limits<double>::quiet_NaN() : left / right;
}

void clear() {
    entry = "0";
    expression.clear();
    stored = 0.0;
    pendingOp = 0;
    freshEntry = true;
}

void digit(const std::string& text) {
    if (freshEntry || entry == "Error") {
        entry = text == "00" ? "0" : text;
        freshEntry = false;
    } else if (entry.size() < 12) {
        entry = entry == "0" && text != "00" ? text : entry + text;
    }
}

void dot() {
    if (freshEntry || entry == "Error") {
        entry = "0.";
        freshEntry = false;
    } else if (entry.find('.') == std::string::npos) {
        entry += ".";
    }
}

void backspace() {
    if (freshEntry || entry == "Error" || entry.size() <= 1) {
        entry = "0";
        freshEntry = true;
        return;
    }
    entry.pop_back();
    if (entry == "-" || entry.empty()) {
        entry = "0";
        freshEntry = true;
    }
}

void percent() {
    entry = trimNumber(value() / 100.0);
    freshEntry = true;
}

void setOperator(char op) {
    if (pendingOp && !freshEntry) {
        entry = trimNumber(apply(stored, value(), pendingOp));
    }
    stored = value();
    pendingOp = op;
    expression = groupNumber(trimNumber(stored)) + " " + opText(op);
    freshEntry = true;
}

void equals() {
    if (!pendingOp) {
        return;
    }
    const double right = value();
    const double left = stored;
    entry = trimNumber(apply(left, right, pendingOp));
    expression = groupNumber(trimNumber(left)) + " " + opText(pendingOp) + " " + groupNumber(trimNumber(right));
    stored = value();
    pendingOp = 0;
    freshEntry = true;
}

unsigned int keyIcon(const std::string& key) {
    if (key == "back") {
        return 0xF55A;
    }
    if (key == "div") {
        return 0xF529;
    }
    if (key == "mul") {
        return 0xF00D;
    }
    if (key == "sub") {
        return 0xF068;
    }
    if (key == "+") {
        return 0xF067;
    }
    if (key == "%") {
        return 0xF295;
    }
    if (key == "=") {
        return 0xF52C;
    }
    return 0;
}

void press(const std::string& key) {
    if (key == "AC") {
        clear();
    } else if (key == "back") {
        backspace();
    } else if (key == "%") {
        percent();
    } else if (key == ".") {
        dot();
    } else if (key == "=") {
        equals();
    } else if (key == "+" || key == "sub" || key == "mul" || key == "div") {
        setOperator(key == "mul" ? '*' : key == "div" ? '/' : key == "sub" ? '-' : '+');
    } else {
        digit(key);
    }
}

float displayFont(float width, const std::string& text) {
    return std::clamp(width / std::max(4.0f, static_cast<float>(text.size()) * 0.56f), 52.0f, 96.0f);
}

void key(core::dsl::Ui& ui, const std::string& id, const std::string& label,
         float x, float y, float size, bool accent = false) {
    const core::Color top = accent ? core::Color{0.78f, 0.86f, 0.82f, 1.0f} : kButtonTop;
    const core::Color bottom = accent ? core::Color{0.58f, 0.72f, 0.66f, 1.0f} : kButtonBottom;
    const unsigned int icon = keyIcon(label);

    ui.stack(id)
        .x(x)
        .y(y)
        .size(size, size)
        .visualStateFrom(id + ".hit", 0.935f)
        .transition(0.13f, core::Ease::OutCubic)
        .content([&] {
            ui.rect(id + ".face")
                .size(size, size)
                .gradient(top, bottom)
                .radius(size * 0.5f)
                .border(1.0f, accent ? core::Color{0.74f, 0.88f, 0.80f, 0.55f} : core::Color{0.36f, 0.38f, 0.39f, 0.46f})
                .build();

            auto text = ui.text(id + ".text")
                .size(size, size)
                .fontSize(icon != 0 ? size * 0.34f : (label == "AC" || label == "00" ? size * 0.31f : size * 0.42f))
                .lineHeight(size * 0.44f)
                .color(accent ? core::Color{0.96f, 0.98f, 0.96f, 1.0f} : kText)
                .horizontalAlign(core::HorizontalAlign::Center)
                .verticalAlign(core::VerticalAlign::Center);
            if (icon != 0) {
                text.icon(icon);
            } else {
                text.text(label);
            }
            text.build();

            ui.rect(id + ".hit")
                .size(size, size)
                .states(kClear, core::Color{1.0f, 1.0f, 1.0f, 0.04f}, core::Color{0.0f, 0.0f, 0.0f, 0.10f})
                .radius(size * 0.5f)
                .onClick([label] { press(label); })
                .build();
        })
        .build();
}

} // namespace

const DslAppConfig& dslAppConfig() {
    static const DslAppConfig config = {
        "Calculator",
        "calculator",
        kBg,
        430,
        760,
        false,
        90.0
    };
    return config;
}

void compose(core::dsl::Ui& ui, const core::dsl::Screen& screen) {
    const float margin = std::clamp(screen.width * 0.045f, 16.0f, 24.0f);
    const float w = std::max(260.0f, std::min(screen.width - margin * 2.0f, 460.0f));
    const float h = std::max(520.0f, screen.height - margin * 2.0f);
    const float gap = std::clamp(w * 0.052f, 14.0f, 22.0f);
    const float buttonByWidth = (w - gap * 3.0f) / 4.0f;
    const float buttonByHeight = (h - 170.0f - gap * 4.0f) / 5.0f;
    const float button = std::clamp(std::min(buttonByWidth, buttonByHeight), 54.0f, 96.0f);
    const float gridW = button * 4.0f + gap * 3.0f;
    const float gridH = button * 5.0f + gap * 4.0f;
    const float gridX = (w - gridW) * 0.5f;
    const float gridY = h - gridH - 18.0f;
    const float headerH = std::max(140.0f, gridY - 8.0f);
    const std::string shown = groupNumber(entry);
    const std::string exp = expression.empty() ? " " : expression;

    ui.stack("root")
        .size(screen.width, screen.height)
        .align(core::Align::CENTER, core::Align::CENTER)
        .content([&] {
            ui.stack("calc")
                .size(w, h)
                .content([&] {
                    ui.text("expr")
                        .x(22.0f)
                        .y(std::max(20.0f, headerH - 136.0f))
                        .size(w - 44.0f, 48.0f)
                        .text(exp)
                        .fontSize(34.0f)
                        .lineHeight(42.0f)
                        .color(kMuted)
                        .horizontalAlign(core::HorizontalAlign::Right)
                        .verticalAlign(core::VerticalAlign::Center)
                        .build();

                    ui.rect("cursor")
                        .x(w - 18.0f)
                        .y(std::max(26.0f, headerH - 126.0f))
                        .size(2.0f, 42.0f)
                        .color(core::Color{0.68f, 0.86f, 0.80f, 0.78f})
                        .radius(1.0f)
                        .build();

                    ui.text("result")
                        .x(16.0f)
                        .y(std::max(72.0f, headerH - 78.0f))
                        .size(w - 32.0f, 100.0f)
                        .text(shown)
                        .fontSize(displayFont(w - 32.0f, shown))
                        .lineHeight(104.0f)
                        .color(core::Color{1.0f, 1.0f, 1.0f, 1.0f})
                        .horizontalAlign(core::HorizontalAlign::Right)
                        .verticalAlign(core::VerticalAlign::Center)
                        .build();

                    const char* labels[5][4] = {
                        {"AC", "%", "back", "div"},
                        {"7", "8", "9", "mul"},
                        {"4", "5", "6", "sub"},
                        {"1", "2", "3", "+"},
                        {"00", "0", ".", "="}
                    };
                    for (int row = 0; row < 5; ++row) {
                        for (int col = 0; col < 4; ++col) {
                            const std::string label = labels[row][col];
                            key(ui, "key." + std::to_string(row) + "." + std::to_string(col), label,
                                gridX + static_cast<float>(col) * (button + gap),
                                gridY + static_cast<float>(row) * (button + gap),
                                button, label == "=");
                        }
                    }
                })
                .build();
        })
        .build();
}

} // namespace app
