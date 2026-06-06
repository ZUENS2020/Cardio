# GY-PCM5102A 外接立体声 DAC 接线方案

用 **GY-PCM5102（PCM5102A）I2S DAC 小板**给 Cardputer ADV 加一路**真立体声**线路输出
（从 DAC 板自带的 3.5mm 口出声）。

> 为什么换掉 WM8960：PCM5102A **没有 I2C 控制口**（配置全靠引脚电平，板上跳线已默认拉好），
> 不用写一行寄存器初始化代码，飞线只需 **3 根 I2S + 2 根电源（+SCK 接地）**，最省事。
> 代价：它是**纯 DAC（只放音，无录音/麦克风）**、**无硬件音量寄存器**（音量在固件里数字处理）、
> 线路电平输出（推耳机偏小、推有源音箱正合适）。

> ⚠️ **核实后更正（重要）**：这块 **GY 紫板自带 3.3V 稳压器（板载 LDO）**，VIN 可直接吃 5V。
> 所以**你买的 AMS1117 这次用不上**——EXT 的 5V 直接进 VIN 即可（这正是树莓派的标准接法）。
> 拿到板先看 VIN 旁有没有一颗 SOT-23 小稳压（多标 `662`/`AMS1117`/`LG33`）确认；有就直喂 5V。

---

## 0. 物料

| 件 | 说明 |
|---|---|
| GY-PCM5102 板 | 紫色小板，丝印 `SCK/BCK/DIN/LCK/GND/VIN` + 板载 3.5mm 座 + **板载 3.3V LDO** |
| 杜邦线 | 母对母若干（接 Cardputer EXT 排针 ↔ DAC 板）|
| ~~AMS1117 模块~~ | **本方案不需要**（板载已有稳压）。若你坚持用，见 §1 备选接法 |

---

## 1. 接线表（飞线，5 根 + SCK 接地）

```
Cardputer EXT(P3)                         GY-PCM5102 板
─────────────────                         ─────────────
 12  5VOUT(+5V) ────────────────────────► VIN   (板载 LDO 自己降到 3.3V)
 13  GND ───────────────────────┬───────► GND
                                └───────► SCK   (接地→启用内部 PLL，见 §2)
  2  INT  / G4 ──────────────────────────► BCK   (位时钟 BCLK)
  3  BUSY / G6 ──────────────────────────► LCK   (帧时钟 LRCK/WS)
  7  CS   / G5 ──────────────────────────► DIN   (I2S 数据)
```

| 信号 | Cardputer EXT 针 → GPIO | PCM5102 丝印 | 备注 |
|---|---|---|---|
| 电源正 | 12 → 5VOUT(**+5V**) | VIN | 板载 LDO 降 3.3V，**直接喂 5V** |
| 地 | 13 → GND | GND | 共地 |
| 系统时钟 | 13 → GND | SCK → **接 GND** | PCM5102A 内部 PLL 自生时钟（多数板背面 SCK 焊桥已接地，则此线可省）|
| I2S 位时钟 | 2 → G4 | BCK | |
| I2S 帧时钟 | 3 → G6 | LCK | |
| I2S 数据 | 7 → G5 | DIN | |

> **备选（一定要用 AMS1117）**：EXT 5V → AMS1117 → 3.3V → VIN 也行，但 GY 板载 LDO 会再降一道、
> 有压差余量问题（实测能用），**不如直喂 5V 干净**。两者不可同时供（板上 5V 与 3.3V 轨别一起灌）。

> **EXT(P3) 完整定义**（据 `Sch_M5CardputerAdv_v1.0.pdf`）：
> 1=RESET/G3⚠️ · 2=INT/G4 · 3=BUSY/G6 · 4=SCK/**G40⚠️SD** · 5=MOSI/**G14⚠️SD** · 6=MISO/**G39⚠️SD** · 7=CS/G5
> · 8=GPS-TX/G15 · 9=GPS-RX/G13 · 10=SCL/G9 · 11=SDA/G8 · 12=5VOUT · 13=GND · 14=5VIN
> 选 **G4/G6/G5** 做 I2S：避开 G40/G14/G39（SD 卡 SPI，固件在用）和 G3（strapping）。
> **G8/G9（I2C）这次不用了**——PCM5102 没有 I2C，省两根线。

---

## 2. 板背面 4 个跳线（H/L 焊桥）——出声的关键

GY-PCM5102 背面有 **4 组焊桥**，标 `H 1 L / H 2 L / H 3 L / H 4 L`（每组把中间脚焊向 H=3.3V 或 L=GND）。
**正确配置 = `1→L, 2→L, 3→H, 4→L`**（出厂多半已这样，但有的板空焊/焊错 → 没声音，**务必先核对**）：

| 焊桥 | 对应 PCM5102A 引脚 | 设成 | 含义 |
|---|---|---|---|
| 1 | **FLT** 滤波选择 | **L** | 正常延迟 FIR（默认）|
| 2 | **DEMP** 去加重 | **L** | 关（音源基本不带预加重）|
| 3 | **XSMT** 软静音(低有效) | **H** | **拉高=取消静音**。焊成 L 会被静音 → **零输出**，最常见的"没声音"原因 |
| 4 | **FMT** 格式 | **L** | 标准 I2S（与固件一致）|

> 注:不同批次丝印编号可能不同,**认引脚名为准**——核心就一条:**XSMT 必须接 3.3V(H),其余三个接 GND(L)**。

**SCK 焊桥**：板上另有一处把 SCK 接地的焊桥。若已接地，§1 那根 `GND→SCK` 飞线可省；
拿到先用万用表量 **SCK 对 GND 是否已通**，没通就补焊或飞那根线。**SCK 悬空会出失真/重低音怪声。**

**出声口**：声音从 **PCM5102 板自带的 3.5mm 座**出来，插耳机/音箱到**这个口**，
不再走 Cardputer 机身的喇叭/耳机口（那一路是 ES8311 单声道，本方案不用它）。

---

## 3. 固件实现（已完成 ✅）

实现方式不是另起 `AudioOutputI2S`，而是**复用现有 `M5.Speaker` 整条管线**（三缓冲 +
core0 喂数任务 + playRaw + 音量曲线 + EQ 钩子），只把它的 **I2S 引脚从内置 ES8311
（端口1 / G41/43/42）重定向到 EXT 的 G4/G6/G5**，并关掉单声道下混。改动点：

| 项 | 内置（`internal`）| 外接（`pcm5102`）| 代码 |
|---|---|---|---|
| I2S 引脚 | ES8311 端口1 G41/43/42 | **BCK=G4 / LRCK=G6 / DATA=G5，无 MCLK** | `AudioEngine::routeSpeaker()` |
| 声道 | (L+R)/2 下混单声道 | **真立体声**，左右独立 | `AudioOutputM5Speaker::setStereo()` |
| EQ | `processMono()` 滤一路 | `process()` 左右**各一路**（独立 biquad 状态）| `Equalizer` |
| 音量 | sqrt 预补偿 | **照旧 sqrt**——仍走 M5.Speaker，其内部平方还在，曲线不变 | `applyHwVolume()` |
| ES8311 | 工作 | 空闲（仍挂 I2C，无害）| — |

**怎么启用**（不必改代码）：

- SD `config.txt` 设 **`audio_output=pcm5102`**（默认 `internal`，没接 DAC 时照常用内置喇叭）。
- 或控制台**临时切换**不重启：`out pcm5102` / `out internal`（`config save` 才持久化）。

> 设计成开关 + 默认 `internal`：DAC 没接线时设备不会变哑；`out` 命令方便 A/B 对比内置单声道 vs 外接立体声。

---

## 4. 上电自检顺序

1. **核对背面 4 个焊桥**：`1L 2L 3H 4L`（尤其 **3=XSMT=H**，否则被静音零输出）。
2. 量 **SCK 对 GND 是否已短**（决定那根 SCK 飞线要不要补）。
3. 确认板上有板载 LDO（VIN 旁 SOT-23）→ 接好 5 根线，**EXT 5V 直接进 VIN**，插 Cardputer EXT。
4. 烧改好的固件 → 播一首歌 → 耳机/音箱插 **PCM5102 板的 3.5mm 口**。
5. 没声音排查顺序：**XSMT 是否=H**(最常见) → SCK 是否接地 → DIN/BCK/LCK 是否对位（G5/G4/G6）→ VIN 有没有 3.3V。

---

## 5. 与 WM8960 方案的取舍

| | **PCM5102A（本方案）** | WM8960 |
|---|---|---|
| 接线 | 飞线 7 根，无 I2C | 飞线/转接板，含 I2C |
| 固件 | 仅配 I2S，**零寄存器** | 需 I2C 写一堆寄存器 |
| 功能 | 纯放音，立体声，线路电平 | 放音+录音、带耳放、硬件音量、插孔检测 |
| 适合 | **只想要立体声、最省事** ✅ | 想要耳放/录音/硬件音量 |

你的需求是"听个立体声"，PCM5102 这条路最短。WM8960 的 PCB 转接板文档
（[../wm8960-adapter/](../wm8960-adapter/)）保留作为备选，不删。

---

## 6. 资料出处（已存进项目）

- **PCM5102A 数据手册**：`docs/references/datasheets/PCM5102A_datasheet.pdf`（[TI 原文](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)）
  — 第 8 节：SCK 接地→内部 PLL；FLT/DEMP/XSMT/FMT 引脚电平定义；3.3V 三路供电；时钟丢失自动 standby。
- GY-PCM5102 板跳线/接法佐证：
  [mt32-pi wiki（1L 2L 3H 4L）](https://github.com/dwhinham/mt32-pi/wiki/GY-PCM5102-DAC-module) ·
  [todbot 博客（板载稳压、3wire 接法）](https://todbot.com/blog/2023/05/16/cheap-stereo-line-out-i2s-dac-for-circuitpython-arduino-synths/) ·
  [makerguides ESP32+PCM5102A](https://www.makerguides.com/playing-audio-with-esp32-and-pcm5102/)

---

*接线确定后，固件改动见上 §3；全部参考手册索引见 [docs/references/REFERENCES.md](../../references/REFERENCES.md)。*
