# DSL 设计与当前实现

当前推荐写法是声明式 DSL：应用只描述页面结构、样式、交互回调和目标状态，`core::dsl::Runtime` 负责布局、状态缓存、事件、动画、脏区渲染和 OpenGL primitive 同步。

## 核心元素

```cpp
enum class ElementKind {
    Row,
    Column,
    Stack,
    Rect,
    Text,
    Image
};
```

- `Row`：横向布局容器。
- `Column`：纵向布局容器。
- `Stack`：叠放布局容器。
- `Rect`：基础视觉图元，支持颜色、渐变、圆角、边框、阴影、透明度、blur、transform、hover/pressed 状态。
- `Text`：文本图元，支持字体、字号、颜色、换行、行高和对齐。
- `Image`：图片图元，支持本地图片、网络图片、SVG、Bing daily、cover/contain/stretch。

组件不进入 core 枚举。组件层只是组合 DSL 图元，例如 `components::button(ui, id)` 内部使用 `Stack + Rect + Row + Text`。

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

- initialize
- update
- isAnimating
- render
- shutdown
- 按屏幕刷新率主动节流
- 无动画时等待事件休眠

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

## 通用交互 DSL

`Row / Column / Stack / Rect / Text / Image` 都支持通用交互方法：

```cpp
.interactive(true)
.disabled(false)
.enabled(true)
.cursor(core::CursorShape::Hand)
.onClick(callback)
```

`.onClick(...)` 会自动开启 interactive，并把 cursor 设置为手型。Runtime 会做 topmost hit-test、按下捕获、点击判定和回调派发。

示例：

```cpp
ui.text("github.link")
    .size(260.0f, 34.0f)
    .text("GitHub")
    .color(accent)
    .onClick([] {
        core::platform::openUrl("https://github.com/sudoevolve/EUI-NEO");
    })
    .build();
```

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

Rect 支持：

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
.states(normal, hover, pressed)
```

`.states(normal, hover, pressed)` 是 Rect 专用的 hover / pressed 视觉状态，会开启交互并由 Runtime 维护 blend。

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

Text 支持：

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

## Image DSL

```cpp
ui.image("cover")
    .size(320.0f, 180.0f)
    .source("assets/icon.png")
    .cover()
    .radius(16.0f)
    .build();

ui.image("bing")
    .size(420.0f, 220.0f)
    .bingDaily(0, "zh-CN")
    .cover()
    .build();
```

Image 支持：

```cpp
.source(pathOrUrl)
.path(path)
.url(url)
.bingDaily(idx, mkt)
.fit(core::ImageFit::Cover)
.cover()
.contain()
.stretch()
.radius(...)
.opacity(...)
.tint(...)
.color(...)
.translate(...)
.scale(...)
.rotate(...)
```

默认 fit 是 `Cover`，图片会适应裁剪，不会强行压缩变形。

## 动画 DSL

动画目标写在元素属性上，Runtime 负责从当前值插值到目标值：

```cpp
ui.rect("actor")
    .x(active ? 420.0f : 40.0f)
    .opacity(active ? 0.4f : 1.0f)
    .rotate(active ? 0.4f : 0.0f)
    .transition(0.42f, core::Ease::OutBack)
    .animate(core::AnimProperty::Frame |
             core::AnimProperty::Opacity |
             core::AnimProperty::Transform)
    .build();
```

Frame 动画需要显式 `.animate(core::AnimProperty::Frame)`。窗口大小变化、页面切换导致的普通布局尺寸变化不会默认产生长宽动画。

当前可动画属性：

- Rect：frame、color、opacity、radius、border、shadow、blur、transform。
- Text：frame、text color、opacity。
- Image：frame、tint/color、opacity、radius、transform。

## 组件写法

组件层在 `components/`，不要直接持有 primitive，也不要绕过 Runtime。

当前组件：

- `components::panel(ui, id)`：直接返回 `ui.rect(id)`。
- `components::text(ui, id)`：直接返回 `ui.text(id)`。
- `components::label(ui, id)`：直接返回 `ui.label(id)`。
- `components::image(ui, id)`：直接返回 `ui.image(id)`。
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

## Runtime 行为

`core::dsl::Runtime` 负责：

- 持有 `Ui`。
- 调用 `ui.layout()` 计算逻辑坐标。
- 按 id 缓存 Rect / Text / Image primitive 实例。
- 统一处理 pointer event、hit-test、press capture、click。
- 维护 hover / press 动画状态。
- 推进 transition 动画。
- 维护 dirty rect。
- 使用离屏 framebuffer cache + scissor 做脏区渲染。
- 处理 DPI scale。
- render / shutdown。

纯 hover / press / transition 视觉变化不会重新 compose 页面。click 回调通常会修改 app 状态，因此 Runtime 会设置 `needsCompose()`，`app/dsl_app.h` 再重新 compose 并保守触发 full redraw。

## 当前限制

- 还没有 z-index；声明顺序决定绘制顺序，也影响 topmost hit-test。
- 还没有 clip / scroll。
- 还没有键盘 focus。
- 还没有事件冒泡。
- 只有 click 回调，没有公开 hover / drag 回调。
- transform 后的 hit-test 仍按布局矩形计算。
- id 移除后的实例缓存目前不会主动回收，只是不再绘制。
- 脏区渲染是保守矩形，复杂重叠场景可能扩大重绘区域。
