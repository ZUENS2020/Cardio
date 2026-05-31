# Cardio UI 实现方案调研

> 目的：在拿到设计系统之前，定好 UI 的技术路线，并明确"设计系统要按什么格式交付"
> 才能在固件里高保真还原。结论基于**已安装的 M5GFX 版本**实测 API（非泛泛网搜）。

---

## 1. 结论先行

| 决策 | 选择 | 理由 |
|---|---|---|
| **渲染框架** | **M5GFX 即时模式 + 自建薄主题/组件层**（不上 LVGL） | 见 §2 |
| **画布** | 全屏 `M5Canvas` 16bpp 建在 PSRAM，`pushSprite(0,0)` | 无闪烁，8MB PSRAM 充裕 |
| **中文** | 内置 `efontCN_12/14/16`，零资源 | 自带 UTF-8 |
| **品牌/拉丁字体** | VLW 抗锯齿字体，`loadFont` 从 SD 加载 | 还原设计字形 |
| **图标** | 核心图标编译成 RGB565+alpha 数组（`pushAlphaImage`）；可选 PNG 放 SD | 快 + 可热更 |

---

## 2. 为什么不用 LVGL

两条路都在 ADV 上可行（8MB PSRAM 让内存不再是约束，网上"Cardputer 内存紧张"说的是标准版 2MB，与我们无关）。但本项目选**即时模式**：

| 维度 | M5GFX 即时模式（选） | LVGL |
|---|---|---|
| 与现有栈集成 | 已是 M5GFX/M5Unified（音频也用 M5.Speaker），单栈无风险 | 需自建 draw buffer→flush 到 M5GFX、tick、输入对接 |
| 本项目 UI 特征 | 6 个固定屏，多为静态布局 + 少量动态区（进度条、列表光标）——即时模式正好 | 优势在复杂控件树/动画，本项目用不满 |
| 代码量 | 薄组件层，几百行 | 引入框架 + 适配层，Flash 涨数百 KB |
| 学习/维护 | 直接画，可控 | 额外抽象层 |
| **何时反选 LVGL** | —— | 若设计系统强依赖**复杂动效/转场/惯性滚动/富交互控件**，再评估 |

> 即时模式 = 每帧在 Sprite 上从背景往上重绘需要的元素，最后整屏推出。参考仓库
> （AndyAiCardputer Winamp 播放器）已用此法在 ADV 跑出完整 UI：全屏 `createSprite(240,135)`
> + 子 Sprite 嵌套 `pushSprite(&parent,x,y)` + `fillRoundRect`/`color565`/自定义字体。

---

## 3. M5GFX 能力清单（已安装版本实测）

设计系统常见元素 → 对应 API，**全部可用**：

| 设计元素 | M5GFX API |
|---|---|
| 抗锯齿圆角矩形 / 圆 | `fillSmoothRoundRect` / `fillSmoothCircle` |
| 普通圆角矩形 | `fillRoundRect` / `drawRoundRect` |
| 逐像素 alpha 合成（透明图标、浮层） | `pushAlphaImage` |
| 位图 / 照片资源 | `drawPng` / `drawJpg` / `drawBmp`（可从 SD 或内存数组） |
| 运行时抗锯齿字体 | `loadFont` / `unloadFont`（VLW 格式） |
| 缩放 / 旋转 | `pushRotateZoom` |
| 渐变 | `drawGradientLine`（线性，逐线绘制） |
| 中文 5 档字号 | `efontCN_10/12/14/16/24` |
| PSRAM 画布 | Sprite `setColorDepth(16)` + `setPsram(true)` + `createSprite` |
| 文本对齐基准 | `setTextDatum`（9 种锚点）、`textWidth/fontHeight` 做布局 |

> ⚠️ **没有**直接的"半透明纯色填充"（如 `fillRectAlpha`）。做背景压暗（CallScreen 弹出时压暗下层）
> 用：① 预先生成一张带统一 alpha 的纯色 Sprite 用 `pushAlphaImage` 叠加，或 ② 直接画不透明浮层。
> **不支持**实时高斯模糊/背景毛玻璃——设计若依赖 backdrop-blur 需改为不透明卡片。

---

## 4. 设计 Token → 固件映射方案

落到代码就是一个 `ui/Theme.h`（设计 token 集中处） + `ui/widgets/`（组件）。

### 4.1 颜色
- 设计给 **hex**，编译期转 RGB565 常量表放进 `Theme.h`：
  ```cpp
  // RGB888 → RGB565
  constexpr uint16_t rgb565(uint8_t r,uint8_t g,uint8_t b){
    return ((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3);
  }
  namespace color {
    constexpr uint16_t bg        = rgb565(0x12,0x14,0x18);
    constexpr uint16_t surface   = rgb565(0x1E,0x21,0x28);
    constexpr uint16_t primary   = rgb565(0x4C,0x9A,0xFF);
    constexpr uint16_t textHi    = rgb565(0xF2,0xF4,0xF8);
    constexpr uint16_t textLow   = rgb565(0x8A,0x92,0xA0);
    // ...语义命名，非具体色值散落各处
  }
  ```
- ⚠️ **RGB565 只有 5-6-5 位**：每通道精度损失，**渐变/相近色会有色带**。设计渐变面积
  尽量小，或接受轻微 banding。深色 UI 受影响小。

### 4.2 字体（type scale）
角色 → 字体对象，集中在 Theme：
| 角色 | 字体来源 | 备注 |
|---|---|---|
| 中文正文 / 列表 | `efontCN_14` 内置 | UTF-8，无需资源 |
| 中文小字 / 状态 | `efontCN_12` | 12px 是可读下限 |
| 品牌/拉丁标题 | **VLW** 自定义字体（SD `/Cardio/ui/fonts/Brand-20.vlw`） | 抗锯齿；每个字号一个 vlw |
| 数字/时间（等宽风格） | GFX bitmap 字体（squix 转换器）或 VLW | 参考仓库用 DSEG7 七段风 |

- VLW 生成：M5Stack 官方 [vlw_font_creator](https://docs.m5stack.com/en/guide/develop_tools/vlw_font_creator) 或 Processing。
- ⚠️ **CJK 仍走 efont**：自定义品牌字体一般只含拉丁/数字。中文若也要品牌字形，需提供
  CJK 字体且按字号生成 vlw（体积大、加载慢），通常不值得——**建议品牌字体只用于拉丁/数字**。

### 4.3 间距 / 圆角
- 设计给 token（如 4/8/12/16），固件里就是 px 常量（屏幕 240×135 固定，1:1 映射）：
  ```cpp
  namespace space { constexpr int xs=2,s=4,m=8,l=12,xl=16; }
  namespace radius{ constexpr int sm=3, md=6, lg=10; }
  ```

### 4.4 图标
两条流水线，建议核心图标走 A：
- **A. 编译进固件**：图标 PNG → RGB565+alpha C 数组 → `pushAlphaImage`。最快，无 SD 依赖。
- **B. 放 SD 热更**：`/Cardio/ui/icons/*.png` → `drawPng`。改图标不用重新烧录，略慢。
- 统一规格 **16×16**（状态栏）/ 20×20（按钮），透明底。

### 4.5 组件
`ui/widgets/` 下一组无状态绘制函数，签名约定 `draw(M5Canvas& cv, const Theme&, Rect, State)`：
- `StatusBar`（顶部：时间/电量/WiFi/通知图标）
- `ListRow`（BrowserScreen 列表项，含 focused/normal）
- `ProgressBar`（PlayerScreen，支持局部重绘）
- `Button` / `Toast`（NotifyOverlay）/ `Badge`
- 状态枚举：`Normal / Focused / Pressed / Disabled`

### 4.6 合成与局部刷新
- 全屏 `M5Canvas`（16bpp/PSRAM）每帧重绘 → `pushSprite(0,0)`。
- 高频小区域（进度条、频谱）用**子 Sprite** 只推脏矩形（参考仓库 `spr.pushSprite(&sprite,x,y)`），
  避免整屏重绘耗时。

---

## 5. 交付设计系统时，请按这个格式给我

这样能 1:1 还原、避免返工（与给设计师的 brief 对齐）：

1. **画板尺寸**：恰好 **240×135**，或 **480×270 @2x**（我按半数映射）。每个屏一张 + 关键状态。
   - 6 个屏：Player / Browser / NotifyOverlay / Call / Settings / Splash。
2. **颜色 token**：语义命名 + hex（如 `bg #121418`、`primary #4C9AFF`…）。一张色板。
3. **字体**：
   - type scale 表：角色 → 字号(px) / 字重 / 行高。
   - 字体文件（TTF/OTF）+ **可嵌入的授权**。
   - 注明哪些角色是拉丁/数字（用品牌字体），中文用 efont 我已内置。
4. **图标**：**SVG 或 PNG @1x，16×16 网格，透明底**，语义命名（`wifi_on`、`battery_80`…）。
5. **间距 / 圆角 token**：数值表（4/8/12… , radius 3/6/10…）。
6. **组件规格**：各组件的尺寸、内边距、各状态（normal/focused/pressed/disabled）样式。
7. **红线标注**：小屏 px 级别很敏感，关键元素请标尺寸/坐标。

### 设计前请知会设计师的硬约束
- 屏幕 **240×135、RGB565**（每通道精度低，大面积渐变有色带 → 慎用大渐变）。
- **无实时模糊/毛玻璃**；透明仅限图标/浮层的逐像素 alpha，背景压暗用不透明卡片或预生成图层。
- **正文字号 ≥12px** 才清晰；状态栏 10px 仅限极短文本。
- 中文走内置 efont 点阵字体（**非**品牌矢量字形）；品牌字体建议只用于拉丁/数字。
- 深色主题对 565 色带最友好（与现有规划一致）。

---

## 6. 待办（设计系统到位后）
- [ ] 建 `ui/Theme.h`：颜色/字号/间距/圆角 token
- [ ] 字体管线：efontCN 选定字号 + 生成品牌 VLW，放 `/Cardio/ui/fonts/`
- [ ] 图标管线：写个 PNG→RGB565+alpha 数组的转换脚本（`tools/`）
- [ ] `ui/widgets/` 基础组件
- [ ] 6 屏逐个实现（Splash → Player → Browser → Settings → NotifyOverlay → Call）
