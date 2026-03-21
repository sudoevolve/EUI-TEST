#include "InputBox.h"
#include <GLFW/glfw3.h>
#include <cmath>

namespace EUINEO {

InputBox::InputBox(std::string placeholderText, float x, float y, float w, float h)
    : placeholder(placeholderText) {
    this->x = x;
    this->y = y;
    this->width = w;
    this->height = h;
}

void InputBox::Update() {
    bool hovered = IsHovered();

    float targetHover = hovered ? 1.0f : 0.0f;
    if (hoverAnim != targetHover) {
        hoverAnim = Lerp(hoverAnim, targetHover, State.deltaTime * 15.0f);
        if (std::abs(hoverAnim - targetHover) < 0.01f) hoverAnim = targetHover;
        MarkDirty(4.0f);
    }

    float targetFocus = isFocused ? 1.0f : 0.0f;
    if (focusAnim != targetFocus) {
        focusAnim = Lerp(focusAnim, targetFocus, State.deltaTime * 15.0f);
        if (std::abs(focusAnim - targetFocus) < 0.01f) focusAnim = targetFocus;
        MarkDirty(4.0f);
    }

    if (State.mouseClicked) {
        bool nextFocus = hovered;
        if (isFocused != nextFocus) {
            isFocused = nextFocus;
            cursorBlinkTime = 0.0f;
            cursorVisible = true;
            MarkDirty(4.0f);
        }
    }

    if (isFocused) {
        cursorBlinkTime += State.deltaTime;
        if (cursorBlinkTime >= 1.0f) {
            cursorBlinkTime = std::fmod(cursorBlinkTime, 1.0f);
        }

        bool nextCursorVisible = cursorBlinkTime < 0.5f;
        if (cursorVisible != nextCursorVisible) {
            cursorVisible = nextCursorVisible;
            MarkDirty(4.0f);
        }

        if (!State.textInput.empty()) {
            text += State.textInput;
            cursorPosition = (int)text.length();
            cursorBlinkTime = 0.0f;
            cursorVisible = true;
            if (onTextChanged) onTextChanged(text);
            MarkDirty(4.0f);
        }

        if (State.keysPressed[GLFW_KEY_BACKSPACE]) {
            if (!text.empty()) {
                while (!text.empty() && (text.back() & 0xC0) == 0x80) {
                    text.pop_back();
                }
                if (!text.empty()) text.pop_back();
                cursorPosition = (int)text.length();
                cursorBlinkTime = 0.0f;
                cursorVisible = true;
                if (onTextChanged) onTextChanged(text);
                MarkDirty(4.0f);
            }
        }

        if (State.keysPressed[GLFW_KEY_ENTER] || State.keysPressed[GLFW_KEY_KP_ENTER]) {
            if (onEnter) onEnter();
            MarkDirty(4.0f);
        }
    } else {
        cursorBlinkTime = 0.0f;
        cursorVisible = true;
    }
}

void InputBox::Draw() {
    float absX = 0.0f;
    float absY = 0.0f;
    GetAbsoluteBounds(absX, absY);

    Color baseColor = Lerp(CurrentTheme->surface, CurrentTheme->surfaceActive, focusAnim);
    Color hoverColor = Lerp(CurrentTheme->surfaceHover, CurrentTheme->surfaceActive, focusAnim);
    Color bgColor = Lerp(baseColor, hoverColor, hoverAnim);

    Renderer::DrawRect(absX, absY, width, height, bgColor, 6.0f);

    if (focusAnim > 0.01f) {
        Renderer::DrawRect(absX, absY, width, 1.0f, CurrentTheme->border, 0.0f);
        Color focusColor = CurrentTheme->primary;
        focusColor.a = focusAnim;
        Renderer::DrawRect(absX, absY, width, 2.0f, focusColor, 0.0f);
    } else {
        Renderer::DrawRect(absX, absY, width, 1.0f, CurrentTheme->border, 0.0f);
    }

    float textScale = fontSize / 24.0f;
    float textY = absY + height / 2.0f + (fontSize / 4.0f);
    float textX = absX + 10.0f;

    if (text.empty()) {
        if (!placeholder.empty()) {
            Color phColor = CurrentTheme->text;
            phColor.a = 0.5f;
            Renderer::DrawTextStr(placeholder, textX, textY, phColor, textScale);
        }
    } else {
        Renderer::DrawTextStr(text, textX, textY, CurrentTheme->text, textScale);
    }

    if (isFocused && cursorVisible) {
        float cursorX = textX + (text.empty() ? 0.0f : Renderer::MeasureTextWidth(text, textScale));
        Renderer::DrawRect(cursorX, absY + height / 2.0f - 10.0f, 2.0f, 20.0f, CurrentTheme->primary, 1.0f);
    }
}

} // namespace EUINEO
