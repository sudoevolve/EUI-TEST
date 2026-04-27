#pragma once

#include "components/theme.h"
#include "core/dsl.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

namespace components {

struct InputStyle {
    InputStyle() : InputStyle(theme::DarkThemeColors()) {}

    explicit InputStyle(const theme::ThemeColorTokens& tokens) {
        background = tokens.surface;
        hover = tokens.surfaceHover;
        focused = theme::resolveFieldFill(tokens, tokens.surface, 0.20f, 0.70f);
        pressed = tokens.surfaceActive;
        border = theme::withOpacity(tokens.border, 0.78f);
        focusBorder = theme::withAlpha(tokens.primary, 0.86f);
        text = tokens.text;
        placeholder = theme::withOpacity(tokens.text, 0.45f);
        cursor = tokens.primary;
        shadow = theme::popupShadow(tokens);
    }

    core::Color background;
    core::Color hover;
    core::Color focused;
    core::Color pressed;
    core::Color border;
    core::Color focusBorder;
    core::Color text;
    core::Color placeholder;
    core::Color cursor;
    core::Shadow shadow;
    float radius = 10.0f;
};

class InputBuilder {
public:
    InputBuilder(core::dsl::Ui& ui, std::string id)
        : ui_(ui), id_(std::move(id)) {}

    InputBuilder& size(float width, float height) { width_ = width; height_ = height; return *this; }
    InputBuilder& text(std::string value) { text_ = std::move(value); return *this; }
    InputBuilder& value(std::string value) { return text(std::move(value)); }
    InputBuilder& placeholder(std::string value) { placeholder_ = std::move(value); return *this; }
    InputBuilder& multiline(bool value = true) { multiline_ = value; return *this; }
    InputBuilder& fontSize(float value) { fontSize_ = std::max(1.0f, value); return *this; }
    InputBuilder& inset(float value) { inset_ = std::max(0.0f, value); return *this; }
    InputBuilder& style(const InputStyle& value) { style_ = value; return *this; }
    InputBuilder& theme(const theme::ThemeColorTokens& tokens) { style_ = InputStyle(tokens); return *this; }
    InputBuilder& transition(const core::Transition& value) { transition_ = value; return *this; }
    InputBuilder& transition(float duration, core::Ease ease = core::Ease::OutCubic) {
        transition_ = core::Transition::make(duration, ease);
        return *this;
    }
    InputBuilder& onChange(std::function<void(const std::string&)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }
    InputBuilder& onEnter(std::function<void()> callback) {
        onEnter_ = std::move(callback);
        return *this;
    }
    InputBuilder& onFocus(std::function<void(bool)> callback) {
        onFocus_ = std::move(callback);
        return *this;
    }

    void build() {
        const std::string hitId = id_ + ".hit";
        const bool focused = ui_.isFocused(hitId);
        const float textWidth = std::max(0.0f, width_ - inset_ * 2.0f);
        const bool allowMultiline = multiline_;
        const std::function<void(const std::string&)> onChange = onChange_;
        const std::function<void()> onEnter = onEnter_;
        const std::function<void(bool)> onFocus = onFocus_;
        const float textLineHeight = fontSize_ * 1.25f;
        const float textY = multiline_ ? inset_ : 0.0f;
        const float textHeight = multiline_ ? std::max(0.0f, height_ - inset_ * 2.0f) : height_;
        const float lineY = multiline_ ? textY : std::max(0.0f, (height_ - textLineHeight) * 0.5f);
        const core::VerticalAlign textVerticalAlign = multiline_ ? core::VerticalAlign::Top : core::VerticalAlign::Center;
        const float width = width_;
        const float inset = inset_;
        const float fontSize = fontSize_;
        InputState& state = stateFor(id_);
        if (state.text != text_) {
            state.text = text_;
            state.cursor = clampUtf8Boundary(state.text, static_cast<int>(state.text.size()));
            state.selectionStart = state.cursor;
            state.selectionEnd = state.cursor;
            state.horizontalScroll = 0.0f;
        }
        state.cursor = clampUtf8Boundary(state.text, state.cursor);
        state.selectionStart = clampUtf8Boundary(state.text, state.selectionStart);
        state.selectionEnd = clampUtf8Boundary(state.text, state.selectionEnd);
        syncScroll(state, textWidth, fontSize_);

        const bool empty = state.text.empty();
        const float cursorX = std::clamp(inset_ + measureWidth(state.text, 0, state.cursor, fontSize_) - state.horizontalScroll,
                                         inset_,
                                         std::max(inset_, width_ - inset_ - 2.0f));
        const auto selection = selectionRange(state);
        const bool hasSelection = selection.first != selection.second;
        const float selectionX = inset_ + measureWidth(state.text, 0, selection.first, fontSize_) - state.horizontalScroll;
        const float selectionW = measureWidth(state.text, selection.first, selection.second, fontSize_);
        const float visibleTextWidth = std::max(textWidth, measureWidth(state.text, 0, static_cast<int>(state.text.size()), fontSize_) + 24.0f);

        ui_.stack(id_)
            .size(width_, height_)
            .clip()
            .content([&] {
                ui_.rect(hitId)
                    .size(width_, height_)
                    .states(focused ? style_.focused : style_.background, style_.hover, style_.pressed)
                    .radius(style_.radius)
                    .border(1.0f, focused ? style_.focusBorder : style_.border)
                    .shadow(focused ? style_.shadow : core::Shadow{})
                    .transition(transition_)
                    .focusable()
                    .onPress([&state, width, inset, fontSize](const core::PointerEvent& event, const core::Rect& bounds) {
                        state.lastBounds = bounds;
                        state.cursor = cursorFromPointer(state, event.x, bounds, width, inset, fontSize);
                        clearSelection(state);
                        state.dragAnchor = state.cursor;
                        state.selecting = true;
                    })
                    .onFocusChanged(onFocus)
                    .onDrag([&state, width, inset, fontSize](const core::dsl::DragEvent& event) {
                        state.cursor = cursorFromPointer(state, event.x, state.lastBounds, width, inset, fontSize);
                        state.selectionStart = state.dragAnchor;
                        state.selectionEnd = state.cursor;
                        syncScroll(state, std::max(0.0f, width - inset * 2.0f), fontSize);
                    })
                    .onTextInput([&state, allowMultiline, onChange, onEnter, width, inset, fontSize](const core::KeyboardEvent& event) {
                        bool changed = false;

                        if (event.selectAll) {
                            state.selectionStart = 0;
                            state.selectionEnd = static_cast<int>(state.text.size());
                            state.cursor = state.selectionEnd;
                        }
                        if (event.copy) {
                            copySelection(state);
                        }
                        if (event.cut && hasTextSelection(state)) {
                            copySelection(state);
                            eraseSelection(state);
                            changed = true;
                        }
                        if (event.left) {
                            moveCursor(state, -1, event.shift);
                        }
                        if (event.right) {
                            moveCursor(state, 1, event.shift);
                        }
                        if (event.home) {
                            moveCursorTo(state, 0, event.shift);
                        }
                        if (event.end) {
                            moveCursorTo(state, static_cast<int>(state.text.size()), event.shift);
                        }
                        if (event.del) {
                            if (hasTextSelection(state)) {
                                eraseSelection(state);
                                changed = true;
                            } else if (state.cursor < static_cast<int>(state.text.size())) {
                                const int next = nextUtf8Index(state.text, state.cursor);
                                state.text.erase(static_cast<std::size_t>(state.cursor), static_cast<std::size_t>(next - state.cursor));
                                changed = true;
                            }
                        }
                        if (event.backspace) {
                            if (hasTextSelection(state)) {
                                eraseSelection(state);
                                changed = true;
                            } else if (state.cursor > 0) {
                                const int previous = prevUtf8Index(state.text, state.cursor);
                                state.text.erase(static_cast<std::size_t>(previous), static_cast<std::size_t>(state.cursor - previous));
                                state.cursor = previous;
                                clearSelection(state);
                                changed = true;
                            }
                        }
                        if (!event.text.empty()) {
                            insertAtCursor(state, filteredText(event.text, allowMultiline));
                            changed = true;
                        }
                        if (!event.pasteText.empty()) {
                            insertAtCursor(state, filteredText(event.pasteText, allowMultiline));
                            changed = true;
                        }
                        if (event.enter) {
                            if (allowMultiline) {
                                insertAtCursor(state, "\n");
                                changed = true;
                            } else if (onEnter) {
                                onEnter();
                            }
                        }
                        if (event.escape && onEnter) {
                            onEnter();
                        }
                        syncScroll(state, std::max(0.0f, width - inset * 2.0f), fontSize);
                        if (changed && onChange) {
                            onChange(state.text);
                        }
                    })
                    .build();

                if (hasSelection) {
                    ui_.rect(id_ + ".selection")
                        .position(std::max(inset_, selectionX), lineY)
                        .size(std::max(1.0f, std::min(selectionW, width_ - inset_ - std::max(inset_, selectionX))), textLineHeight)
                        .color(theme::withAlpha(style_.cursor, 0.24f))
                        .radius(3.0f)
                        .build();
                }

                ui_.text(id_ + ".text")
                    .position(inset_ - state.horizontalScroll, textY)
                    .size(visibleTextWidth, textHeight)
                    .text(empty ? placeholder_ : state.text)
                    .fontSize(fontSize_)
                    .lineHeight(textLineHeight)
                    .color(empty ? style_.placeholder : style_.text)
                    .wrap(multiline_)
                    .verticalAlign(textVerticalAlign)
                    .build();

                if (focused) {
                    ui_.rect(id_ + ".cursor")
                        .position(cursorX, lineY + std::max(0.0f, (textLineHeight - fontSize_ * 1.18f) * 0.5f))
                        .size(1.5f, fontSize_ * 1.18f)
                        .color(style_.cursor)
                        .radius(1.0f)
                        .build();
                }
            })
            .build();
    }

private:
    struct InputState {
        std::string text;
        int cursor = 0;
        int selectionStart = 0;
        int selectionEnd = 0;
        int dragAnchor = 0;
        bool selecting = false;
        float horizontalScroll = 0.0f;
        core::Rect lastBounds;
    };

    static InputState& stateFor(const std::string& id) {
        static std::unordered_map<std::string, InputState> states;
        return states[id];
    }

    static std::string filteredText(const std::string& input, bool multiline) {
        std::string output;
        for (char ch : input) {
            if (!multiline && (ch == '\n' || ch == '\r')) {
                continue;
            }
            output.push_back(ch);
        }
        return output;
    }

    static int prevUtf8Index(const std::string& value, int index) {
        int out = std::clamp(index, 0, static_cast<int>(value.size()));
        if (out <= 0) {
            return 0;
        }
        --out;
        while (out > 0 && (static_cast<unsigned char>(value[static_cast<std::size_t>(out)]) & 0xC0) == 0x80) {
            --out;
        }
        return out;
    }

    static int nextUtf8Index(const std::string& value, int index) {
        int out = std::clamp(index, 0, static_cast<int>(value.size()));
        if (out >= static_cast<int>(value.size())) {
            return static_cast<int>(value.size());
        }
        ++out;
        while (out < static_cast<int>(value.size()) && (static_cast<unsigned char>(value[static_cast<std::size_t>(out)]) & 0xC0) == 0x80) {
            ++out;
        }
        return out;
    }

    static int clampUtf8Boundary(const std::string& value, int index) {
        int out = std::clamp(index, 0, static_cast<int>(value.size()));
        while (out > 0 && out < static_cast<int>(value.size()) &&
               (static_cast<unsigned char>(value[static_cast<std::size_t>(out)]) & 0xC0) == 0x80) {
            --out;
        }
        return out;
    }

    static std::pair<int, int> selectionRange(const InputState& state) {
        return {std::min(state.selectionStart, state.selectionEnd), std::max(state.selectionStart, state.selectionEnd)};
    }

    static bool hasTextSelection(const InputState& state) {
        return state.selectionStart != state.selectionEnd;
    }

    static void clearSelection(InputState& state) {
        state.selectionStart = state.cursor;
        state.selectionEnd = state.cursor;
        state.dragAnchor = state.cursor;
    }

    static void eraseSelection(InputState& state) {
        const auto range = selectionRange(state);
        if (range.first == range.second) {
            return;
        }
        state.text.erase(static_cast<std::size_t>(range.first), static_cast<std::size_t>(range.second - range.first));
        state.cursor = range.first;
        clearSelection(state);
    }

    static void insertAtCursor(InputState& state, const std::string& value) {
        if (value.empty()) {
            return;
        }
        if (hasTextSelection(state)) {
            eraseSelection(state);
        }
        state.text.insert(static_cast<std::size_t>(state.cursor), value);
        state.cursor += static_cast<int>(value.size());
        clearSelection(state);
    }

    static void moveCursor(InputState& state, int direction, bool keepSelection) {
        const int previous = state.cursor;
        if (!keepSelection && hasTextSelection(state)) {
            const auto range = selectionRange(state);
            state.cursor = direction < 0 ? range.first : range.second;
            clearSelection(state);
            return;
        }
        state.cursor = direction < 0 ? prevUtf8Index(state.text, state.cursor) : nextUtf8Index(state.text, state.cursor);
        if (keepSelection) {
            if (!hasTextSelection(state)) {
                state.selectionStart = previous;
            }
            state.selectionEnd = state.cursor;
        } else {
            clearSelection(state);
        }
    }

    static void moveCursorTo(InputState& state, int position, bool keepSelection) {
        const int previous = state.cursor;
        state.cursor = clampUtf8Boundary(state.text, position);
        if (keepSelection) {
            if (!hasTextSelection(state)) {
                state.selectionStart = previous;
            }
            state.selectionEnd = state.cursor;
        } else {
            clearSelection(state);
        }
    }

    static void copySelection(const InputState& state) {
        if (!hasTextSelection(state)) {
            return;
        }
        const auto range = selectionRange(state);
        if (GLFWwindow* window = glfwGetCurrentContext()) {
            const std::string selected = state.text.substr(static_cast<std::size_t>(range.first), static_cast<std::size_t>(range.second - range.first));
            glfwSetClipboardString(window, selected.c_str());
        }
    }

    static float measureWidth(const std::string& value, int start, int end, float fontSize) {
        const int clampedStart = std::clamp(start, 0, static_cast<int>(value.size()));
        const int clampedEnd = std::clamp(end, clampedStart, static_cast<int>(value.size()));
        if (clampedEnd <= clampedStart) {
            return 0.0f;
        }
        return core::TextPrimitive::measureTextWidth(
            value.substr(static_cast<std::size_t>(clampedStart), static_cast<std::size_t>(clampedEnd - clampedStart)),
            {},
            fontSize,
            400);
    }

    static int cursorFromPointer(const InputState& state, double pointerX, const core::Rect& bounds, float width, float inset, float fontSize) {
        const float scale = width > 0.0f ? bounds.width / width : 1.0f;
        const float localX = static_cast<float>((pointerX - bounds.x) / std::max(0.001f, scale));
        const float target = localX - inset + state.horizontalScroll;
        float cursorX = 0.0f;
        int index = 0;
        while (index < static_cast<int>(state.text.size())) {
            const int next = nextUtf8Index(state.text, index);
            const float advance = measureWidth(state.text, index, next, fontSize);
            if (target < cursorX + advance * 0.5f) {
                return index;
            }
            cursorX += advance;
            index = next;
        }
        return static_cast<int>(state.text.size());
    }

    static void syncScroll(InputState& state, float viewportWidth, float fontSize) {
        const float textWidth = measureWidth(state.text, 0, static_cast<int>(state.text.size()), fontSize);
        const float cursorPixel = measureWidth(state.text, 0, state.cursor, fontSize);
        if (textWidth <= viewportWidth) {
            state.horizontalScroll = 0.0f;
            return;
        }
        const float rightSafe = std::max(8.0f, viewportWidth - 10.0f);
        if (cursorPixel - state.horizontalScroll < 0.0f) {
            state.horizontalScroll = cursorPixel;
        } else if (cursorPixel - state.horizontalScroll > rightSafe) {
            state.horizontalScroll = cursorPixel - rightSafe;
        }
        state.horizontalScroll = std::clamp(state.horizontalScroll, 0.0f, std::max(0.0f, textWidth - viewportWidth + 10.0f));
    }

    core::dsl::Ui& ui_;
    std::string id_;
    InputStyle style_;
    core::Transition transition_ = core::Transition::make(0.16f, core::Ease::OutCubic);
    std::function<void(const std::string&)> onChange_;
    std::function<void()> onEnter_;
    std::function<void(bool)> onFocus_;
    std::string text_;
    std::string placeholder_ = "Input";
    bool multiline_ = false;
    float width_ = 260.0f;
    float height_ = 40.0f;
    float inset_ = 12.0f;
    float fontSize_ = 17.0f;
};

inline InputBuilder input(core::dsl::Ui& ui, const std::string& id) {
    return InputBuilder(ui, id);
}

} // namespace components
