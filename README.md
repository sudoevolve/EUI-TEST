# EUI-NEO

<p align="center">
  <img src="assets/icon.svg" width="104" alt="EUI icon">
</p>

<p align="center">
  <a href="README.zh-CN.md">简体中文</a>
</p>

EUI-NEO is a C++17, OpenGL, and GLFW declarative UI experiment. Pages describe structure, styling, interaction callbacks, and target state through `core::dsl::Ui`; `core::dsl::Runtime` handles layout, animation, events, dirty-region rendering, framebuffer cache, and synchronization with low-level primitives.

## Preview

|  |  |
| --- | --- |
| ![preview 1](docs/pic/1.jpg) | ![preview 2](docs/pic/2.jpg) |
| ![preview 3](docs/pic/3.jpg) | ![preview 4](docs/pic/4.jpg) |

## Quick Start

Requirements:

- CMake 3.14+
- A C++17 compiler
- OpenGL
- Network access for CMake to fetch GLFW and glad

Windows / PowerShell example:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\gallery.exe
```

The project creates one executable for each `app/*.cpp` page source, such as `gallery` and `demo`. After build, `assets/` is copied next to the executable automatically.

## Project Layout

```text
app/          Page entry points and gallery examples
assets/       Runtime assets: fonts, PNG, SVG, and icons
components/   Reusable UI components built on top of the DSL
core/         DSL, Runtime, primitives, text, image, network, and platform code
docs/         Implementation notes and API documentation
3rd/          Third-party single-file dependencies
```

## Docs

- [DSL Design And Current Implementation](docs/DSL.md)
- [Components](docs/组件.md)
- [Primitives And Text](docs/基础图元文本图元.md)
- [Layout](docs/布局.md)
- [Events](docs/事件.md)
- [Animation](docs/动画.md)
- [Rendering Pipeline](docs/渲染流程.md)
- [Images](docs/图片.md)
- [Network](docs/网络.md)
- [Window And Pages](docs/窗口页面.md)

## Current Components

`components/components.h` exports the current component layer:

- Basic wrappers: `panel`, `text` / `label`, `image`, `theme`
- Controls: `button`, `checkbox`, `radio`, `toggleSwitch`, `progress`, `slider`, `input`, `segmented`, `tabs`, `scroll`
- Popups and feedback: `dialog`, `toast`, `contextMenu`, `dropdown`
- Pickers: `datepicker`, `timepicker`, `colorpicker`
- Data display: `dataTable` / `datatable`
- Charts: `linechart` / `lineChart`, `barchart` / `barChart`, `piechart` / `pieChart`

Components only compose DSL trees. They do not own OpenGL primitives directly. Business state stays in the page or application layer: pass the current value into the builder, then write the next value back from callbacks.
