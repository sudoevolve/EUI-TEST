# DSL 设计与当前实现

当前 UI 推荐写法是声明式 DSL：应用只描述页面结构、样式、交互回调和目标状态，`core::dsl::Runtime` 负责布局、状态缓存、动画推进、脏区渲染和 OpenGL primitive 同步。

## 核心元素

`core::dsl` 当前只有 5 种基础元素：

```cpp
enum class ElementKind {
    Row,
    Column,
    Stack,
    Rect,
    Text
};
```

- `Row`：横向布局容器。
- `Column`：纵向布局容器。
- `Stack`：叠放布局容器。
- `Rect`：基础视觉图元，支持颜色、渐变、圆角、边框、阴影、透明度、blur、transform、交互状态。
- `Text`：文本图元，支持字体、字号、颜色、换行、行高和对齐。

组件不进入 core 枚举。组件层应该只是组合 DSL 图元，例如 `components::button(ui, id)` 内部使用 `Stack + Rect + Row + Text`。

## App 入口

DSL app 推荐只实现：

```cpp
#include "app/dsl_app.h"

namespace app {

const DslAppConfig& dslAppConfig();
void compose(core::dsl::Ui& ui, const core::dsl::Screen& screen);

} // namespace app
```

`app/dsl_app.h` 已封装：

- `initialize`
- `update`
- `isAnimating`
- `render`
- `shutdown`

页面示例：

```cpp
void compose(core::dsl::Ui& ui, const core::dsl::Screen& screen) {
    ui.stack("root")
        .size(screen.width, screen.height)
        .align(core::Align::CENTER, core::Align::CENTER)
        .content([&] {
            components::panel(ui, "card")
                .size(360.0f, 260.0f)
                .radius(18.0f)
                .color({0.10f, 0.12f, 0.16f, 1.0f})
                .build();

            components::button(ui, "primary")
                .size(240.0f, 70.0f)
                .text("Click Me")
                .onClick([] {
                    // update app state
                })
                .build();
        });
}
```

## 布局 DSL

容器：

```cpp
ui.row("toolbar")
ui.column("content")
ui.stack("root")
```

通用布局属性：

```cpp
.x(value)
.y(value)
.position(x, y)
.width(value)
.height(value)
.size(width, height)
.fill()
.wrapContent()
.margin(value)
.margin(horizontal, vertical)
.margin(left, top, right, bottom)
.gap(value)
.spacing(value)
.justifyContent(core::Align::CENTER)
.alignItems(core::Align::CENTER)
.align(core::Align::CENTER, core::Align::CENTER)
```

规则：

- `Row` 主轴是 x，`alignItems` 控制 y。
- `Column` 主轴是 y，`alignItems` 控制 x。
- `Stack` 子元素叠放，未指定 x/y 时根据 align 计算位置。

## Rect DSL

```cpp
ui.rect("card")
    .size(360.0f, 260.0f)
    .color({0.10f, 0.12f, 0.16f, 1.0f})
    .radius(18.0f)
    .border(1.0f, {0.23f, 0.29f, 0.38f, 1.0f})
    .shadow(26.0f, 0.0f, 8.0f, {0.0f, 0.0f, 0.0f, 0.26f})
    .transition(0.2f, core::Ease::OutCubic)
    .build();
```

支持：

```cpp
.color(...)
.background(...)
.gradient(...)
.radius(...)
.rounding(...)
.border(...)
.shadow(...)
.blur(...)
.opacity(...)
.translate(...)
.translateX(...)
.translateY(...)
.scale(...)
.rotate(...)
.rotation(...)
.transformOrigin(...)
```

交互也是 `Rect` 能力：

```cpp
ui.rect("hit")
    .size(240.0f, 70.0f)
    .states(normal, hover, pressed)
    .onClick([] {
        // click
    })
    .build();
```

`states(normal, hover, pressed)` 会开启 `interactive`，Runtime 自动维护 hover/press 缓动。

## Text DSL

```cpp
ui.text("title")
    .size(420.0f, 48.0f)
    .text("EUI Gallery")
    .customFont("YouSheBiaoTiHei")
    .fontSize(38.0f)
    .lineHeight(44.0f)
    .color({0.94f, 0.97f, 1.0f, 1.0f})
    .horizontalAlign(core::HorizontalAlign::Center)
    .verticalAlign(core::VerticalAlign::Center)
    .build();
```

支持：

```cpp
.text(...)
.icon(codepoint)
.fontFamily(...)
.font(...)
.customFont(...)
.fontSize(...)
.fontWeight(...)
.color(...)
.opacity(...)
.maxWidth(...)
.wrap(...)
.horizontalAlign(...)
.verticalAlign(...)
.lineHeight(...)
```

`ui.label(id)` 是 `ui.text(id)` 的别名。

## 动画 DSL

动画目标写在元素属性上，Runtime 负责从当前值插值到目标值：

```cpp
ui.rect("actor")
    .x(active ? 420.0f : 40.0f)
    .opacity(active ? 0.4f : 1.0f)
    .rotate(active ? 0.4f : 0.0f)
    .transition(0.42f, core::Ease::OutBack)
    .build();
```

也可以限制动画属性：

```cpp
ui.rect("panel")
    .color(nextColor)
    .transition(core::Transition::make(0.2f).animate(core::AnimProperty::Color))
    .build();
```

当前动画框架位于 `core/animation.h`，包含：

- `core::Ease`
- `core::Transition`
- `core::AnimatedValue<T>`
- `core::SmoothedValue<T>`

`Rect` 当前可动画属性：

- frame
- color
- opacity
- radius
- border
- shadow
- blur
- transform

`Text` 当前可动画属性：

- frame
- text color
- opacity

## 组件写法

组件层在 `components/`，不要直接持有 primitive，也不要绕过 Runtime。

当前组件：

- `components::panel(ui, id)`：直接返回 `ui.rect(id)`。
- `components::text(ui, id)`：直接返回 `ui.text(id)`。
- `components::label(ui, id)`：直接返回 `ui.label(id)`。
- `components::button(ui, id)`：薄 builder，内部组合 `Stack + Rect + Row + Text`。

按钮示例：

```cpp
components::button(ui, "save")
    .size(180.0f, 54.0f)
    .icon(0xF0C7)
    .text("Save")
    .colors(normal, hover, pressed)
    .transition(0.18f)
    .onClick([] {
        // state change
    })
    .build();
```

写新组件时优先这样做：

- 简单组件：直接返回 core builder。
- 复杂组件：提供一个薄 builder，内部只声明 DSL 树。
- 不要在组件里 new / initialize / render / destroy primitive。
- 不要在组件里做自己的事件循环或动画循环。

## Runtime 行为

`core::dsl::Runtime` 负责：

- 持有 `Ui`
- 每帧按需 compose 页面声明
- 调用 `ui.layout()` 计算逻辑坐标
- 按 id 缓存 `Rect` / `Text` primitive 实例
- 维护 hover / press / click
- 推进 transition 动画
- 维护 dirty rect
- 使用离屏 framebuffer cache + scissor 做脏区渲染
- 处理 DPI scale
- render / shutdown

纯 hover/press/transition 视觉变化不会重新 compose 页面。只有 click 回调导致状态变化时，`app/dsl_app.h` 会请求重新 compose，并保守触发 full redraw。

## 当前限制

- 还没有 z-index，绘制顺序就是声明顺序。
- 还没有 clip / scroll。
- 还没有键盘 focus。
- hit-test 当前只对 `Rect` 生效。
- transform 后的 hit-test 仍按原始布局矩形计算。
- id 移除后的实例缓存目前不会主动回收，只是不再绘制。
- 脏区渲染是保守矩形，复杂重叠场景可能扩大重绘区域。
