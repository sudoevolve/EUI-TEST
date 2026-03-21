# EUI-NEO

EUI-NEO 是一个基于 OpenGL 的轻量级 2D GUI 框架，核心思路是用少量基础图元和统一的组件模型，构建可动画、可扩展、可维护的现代桌面界面。

<p align="center">
  <img src="./1.jpg" alt="EUI-NEO Preview 1" width="49%" />
  <img src="./2.jpg" alt="EUI-NEO Preview 2" width="49%" />
</p>

## 结构

```text
EUI-NEO/
├─ main.cpp
├─ README.md
├─ font/
├─ src/
│  ├─ EUINEO.h
│  ├─ EUINEO.cpp
│  ├─ components/
│  │  ├─ Panel.*
│  │  ├─ Label.*
│  │  ├─ Button.*
│  │  ├─ ProgressBar.*
│  │  ├─ Slider.*
│  │  ├─ SegmentedControl.*
│  │  ├─ InputBox.*
│  │  └─ ComboBox.*
│  └─ pages/
│     └─ MainPage.*
└─ CMakeLists.txt
```

- `main.cpp`：窗口、输入回调、主循环、局部重绘、帧缓存回放。
- `src/EUINEO.h` / `src/EUINEO.cpp`：全局状态、渲染器、基础 `Widget` 能力、文本和矩形绘制。
- `src/components/`：通用组件层。
- `src/pages/`：页面组合层，负责组装组件和页面级逻辑。
- `font/`：运行时加载的字体资源。

## 用法

### 编译

```bash
cmake -B build -G Ninja
cmake --build build --config Release
```

### 运行

- 保证可执行文件同级可访问 `font/` 目录。
- 运行后 `main.cpp` 会创建窗口、初始化渲染器、加载字体、进入事件循环。

### 最基本的页面运行方式

当前项目采用“页面对象 + 主循环”的形式运行：

```cpp
EUINEO::MainPage mainPage;

while (!glfwWindowShouldClose(window)) {
    mainPage.Update();

    if (EUINEO::Renderer::ShouldRepaint()) {
        EUINEO::Renderer::BeginFrame();
        mainPage.Draw();
    }
}
```

## 性能优化

项目性能目标：

- 不降低帧率。
- 不改现有模糊公式。
- 不牺牲动画效果。
- 优先减少无意义重绘和重复 blur。

### 实现优化

- 关闭 MSAA（SDF无需）：
  - 创建窗口时设置 `GLFW_SAMPLES = 0`
  - 运行时关闭 `GL_MULTISAMPLE`
- 局部重绘：
  - 组件交互优先使用 dirty rect
  - 只有脏区区域参与局部清理和重绘
- 帧缓存回放：
  - 全屏绘制完成后缓存结果
  - 局部更新时先回放缓存，再补绘脏区
- 局部区域回写：
  - 脏区重绘后只把该区域写回缓存纹理
- blur 结果缓存：
  - backdrop 未变化时，毛玻璃卡片直接回放缓存结果
  - 普通 hover、输入、下拉不再重复跑整块 blur
- 图元提交裁剪：
  - 局部重绘时，`DrawRect` / `DrawTextStr` 只提交与当前 dirty 区相交的内容
- 事件驱动空闲等待：
  - 没有动画和刷新需求时，主循环进入等待，避免 CPU/GPU 空转

### 性能优化的绘制方式

当前绘制流程不是“每次交互整页重画”，而是：

1. 组件在 `Update()` 中判断自己是否变化。
2. 有变化的组件通过 `MarkDirty(...)` 或 `AddDirtyRect(...)` 上报脏区。
3. 主循环判断本帧是否需要全屏重绘或局部重绘。
4. 局部重绘时，先回放上一帧缓存。
5. 再只清理并重绘 dirty 区域。
6. 重绘完成后，将 dirty 区域回写到缓存。

### 什么时候不该打大脏区

下面这些场景不应该直接全屏失效：

- 按钮 hover
- 输入框 focus
- 光标闪烁
- 下拉项 hover
- 普通文本变化

下面这些场景可以认为必须重新计算大区域内容：

- 窗口尺寸变化
- 背景变化
- 主题切换导致 backdrop 失效
- 直接修改大面积 blur 宿主的模糊参数

## 基础图元 API

当前真正的基础绘制 API 主要有这几类。

### 1. 矩形绘制

```cpp
Renderer::DrawRect(
    float x, float y, float w, float h,
    const Color& color,
    float rounding = 0.0f,
    float blurAmount = 0.0f,
    float shadowBlur = 0.0f,
    float shadowOffsetX = 0.0f,
    float shadowOffsetY = 0.0f,
    const Color& shadowColor = Color(0, 0, 0, 0)
);
```

用途：

- 纯色矩形
- 圆角面板
- 阴影卡片
- 毛玻璃背景
- 进度条轨道
- 滑块手柄

### 2. 文本绘制

```cpp
Renderer::DrawTextStr(
    const std::string& text,
    float x, float y,
    const Color& color,
    float scale = 1.0f
);
```

相关 API：

```cpp
Renderer::MeasureTextWidth(const std::string& text, float scale = 1.0f);
Renderer::LoadFont(const std::string& fontPath, float fontSize = 24.0f,
                   unsigned int startChar = 32, unsigned int endChar = 128);
```

### 3. 重绘控制

```cpp
Renderer::RequestRepaint(float duration = 0.0f);
Renderer::AddDirtyRect(float x, float y, float w, float h);
Renderer::ClearDirtyRect();
Renderer::InvalidateAll();
Renderer::InvalidateBackdrop();
```

用途：

- 触发重绘
- 上报局部脏区
- 全屏失效
- backdrop 失效

### 4. Widget 基础能力

```cpp
GetAbsoluteBounds(float& outX, float& outY);
IsHovered();
MarkDirty(float expand = 20.0f, float duration = 0.0f);
```

用途：

- 将相对坐标转为实际屏幕坐标
- 进行基础 hover 判断
- 为当前组件上报局部脏区并触发刷新

## 绘制组件

当前项目里，组件不是单独维护一套渲染路径，而是组合基础图元实现。

### 组件开发标准

- 必须继承 `Widget`
- `Update()` 只处理状态和动画
- `Draw()` 只处理绘制
- 先算绝对坐标，再绘制
- 有变化才打脏区
- 小变化优先 `MarkDirty(...)`
- 不要在 hover 时直接 `InvalidateAll()`
- 不要在普通交互里滥用 `InvalidateBackdrop()`

### 最小组件模板

```cpp
class MyCard : public Widget {
public:
    float hoverAnim = 0.0f;
    std::string text = "Card";

    MyCard(float x, float y, float w, float h) {
        this->x = x;
        this->y = y;
        this->width = w;
        this->height = h;
    }

    void Update() override {
        bool hovered = IsHovered();
        float target = hovered ? 1.0f : 0.0f;
        if (std::abs(hoverAnim - target) > 0.001f) {
            hoverAnim = Lerp(hoverAnim, target, State.deltaTime * 15.0f);
            if (std::abs(hoverAnim - target) < 0.001f) hoverAnim = target;
            MarkDirty(6.0f);
        }
    }

    void Draw() override {
        float absX = 0.0f;
        float absY = 0.0f;
        GetAbsoluteBounds(absX, absY);

        Color bg = Lerp(CurrentTheme->surface, CurrentTheme->surfaceHover, hoverAnim);
        Renderer::DrawRect(absX, absY, width, height, bg, 12.0f);

        float textScale = fontSize / 24.0f;
        Renderer::DrawTextStr(text, absX + 12.0f, absY + 26.0f, CurrentTheme->text, textScale);
    }
};
```

### 现有组件如何构成

- `Panel`：单层矩形或毛玻璃卡片
- `Button`：背景矩形 + 文字
- `ProgressBar`：轨道矩形 + 进度矩形
- `Slider`：轨道矩形 + 已完成区域 + 手柄
- `SegmentedControl`：底板 + 指示器 + 文本
- `InputBox`：输入框底板 + 边框 + 文本 + 光标
- `ComboBox`：主框 + 箭头图标 + 下拉列表 + 列表项

## 绘制页面

页面的职责不是画基础图元，而是组装组件和安排层级。

### 页面开发标准

- 构造函数中做组件实例化和事件绑定
- `Update()` 中推进页面级状态，并依次调用组件 `Update()`
- `Draw()` 中按层级顺序绘制
- 背景层、玻璃层、内容层、浮层要有明确顺序

### 页面结构示例

```cpp
class MainPage {
public:
    Panel glassCard;
    Button btnPrimary;
    InputBox inputBox;
    ComboBox comboBox;

    MainPage();
    void Update();
    void Draw();
};
```

### 页面绘制顺序示例

```cpp
void MainPage::Draw() {
    backgroundPanel.Draw();
    glassCard.Draw();

    titleLabel.Draw();
    btnPrimary.Draw();
    inputBox.Draw();
    comboBox.Draw();
}
```

建议顺序：

- 背景
- 装饰图元
- 毛玻璃宿主
- 普通交互组件
- 浮层和弹出层

## 布局用法

当前项目使用 `Anchor` 做轻量布局。

### 可用锚点

```cpp
Anchor::TopLeft
Anchor::TopCenter
Anchor::TopRight
Anchor::CenterLeft
Anchor::Center
Anchor::CenterRight
Anchor::BottomLeft
Anchor::BottomCenter
Anchor::BottomRight
```

### 基本用法

```cpp
Button btn("Start", 0, 50, 120, 40);
btn.anchor = Anchor::TopCenter;
```

这表示：

- 组件逻辑位置是 `(0, 50)`
- 这个偏移不是相对左上角
- 而是相对屏幕顶部居中点

### 布局计算入口

所有组件最终都通过：

```cpp
GetAbsoluteBounds(absX, absY);
```

把锚点相对坐标转换成真正的屏幕绘制坐标。

### 布局建议

- 页面大布局优先用 `Anchor`
- 组件内部微调用局部偏移
- 不要在每个组件里手写一套屏幕对齐逻辑
- 窗口尺寸变化后，依赖 `GetAbsoluteBounds()` 自动重算位置

## 组件开发自检清单

新增组件或修改组件后，建议至少检查下面这些点：

- 鼠标静止时是否还在持续重绘
- hover 时是否只刷新自己的局部区域
- 是否误用了 `InvalidateAll()`
- 是否误用了 `InvalidateBackdrop()`
- 文本是否使用 `fontSize / 24.0f` 缩放
- 是否通过 `GetAbsoluteBounds()` 计算绝对坐标
- 动画收敛后是否会停止刷新

## 当前内置组件

- `Panel`
- `Label`
- `Button`
- `ProgressBar`
- `Slider`
- `SegmentedControl`
- `InputBox`
- `ComboBox`
