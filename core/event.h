#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "core/primitive.h"

#include <cmath>

namespace core {

enum class CursorShape {
    Arrow,
    Hand
};

struct PointerEvent {
    double x = 0.0;
    double y = 0.0;
    double deltaX = 0.0;
    double deltaY = 0.0;
    bool down = false;
    bool pressedThisFrame = false;
    bool releasedThisFrame = false;
};

struct InteractionState {
    bool hover = false;
    bool pressed = false;
    bool clicked = false;
    bool pressStarted = false;
    bool released = false;
    bool drag = false;
    bool active = false;
    bool changed = false;
    double dragStartX = 0.0;
    double dragStartY = 0.0;
    double dragDeltaX = 0.0;
    double dragDeltaY = 0.0;

    void update(const Rect& bounds, const PointerEvent& event, bool topmostHover, bool enabled = true) {
        const bool oldHover = hover;
        const bool oldPressed = pressed;
        const bool oldDrag = drag;
        const bool oldActive = active;

        clicked = false;
        pressStarted = false;
        released = false;

        if (!enabled) {
            hover = false;
            pressed = false;
            drag = false;
            active = false;
            dragDeltaX = 0.0;
            dragDeltaY = 0.0;
            changed = oldHover != hover || oldPressed != pressed || oldDrag != drag || oldActive != active;
            return;
        }

        hover = topmostHover && bounds.contains(event.x, event.y);

        if (hover && event.pressedThisFrame) {
            active = true;
            pressStarted = true;
            dragStartX = event.x;
            dragStartY = event.y;
        }

        pressed = active && event.down;

        dragDeltaX = event.x - dragStartX;
        dragDeltaY = event.y - dragStartY;
        drag = pressed && (std::fabs(dragDeltaX) > 2.0 || std::fabs(dragDeltaY) > 2.0);

        if (event.releasedThisFrame) {
            released = active;
            clicked = active && hover;
            active = false;
            pressed = false;
            drag = false;
        }

        changed = oldHover != hover ||
                  oldPressed != pressed ||
                  oldDrag != drag ||
                  oldActive != active ||
                  pressStarted ||
                  released ||
                  clicked;
    }
};

inline PointerEvent readPointerEvent(GLFWwindow* window, float dpiScale = 1.0f) {
    static double lastX = 0.0;
    static double lastY = 0.0;
    static bool lastDown = false;

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);
    x *= dpiScale;
    y *= dpiScale;

    PointerEvent event;
    event.x = x;
    event.y = y;
    event.deltaX = x - lastX;
    event.deltaY = y - lastY;
    event.down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    event.pressedThisFrame = event.down && !lastDown;
    event.releasedThisFrame = !event.down && lastDown;

    lastX = x;
    lastY = y;
    lastDown = event.down;
    return event;
}

} // namespace core
