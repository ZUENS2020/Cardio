# Cardio 🎵

> 运行在 **M5Stack Cardputer ADV** 上的音乐播放器固件，集成实时均衡器，并正在加入跨平台 BLE 通知推送。
> *A music-player firmware for the M5Stack Cardputer ADV (ESP32-S3), with a real-time equalizer and BLE notification push in progress.*

![platform](https://img.shields.io/badge/platform-ESP32--S3-blue)
![framework](https://img.shields.io/badge/framework-Arduino%20%2F%20PlatformIO-orange)
![audio](https://img.shields.io/badge/audio-MP3%20%7C%20FLAC%20%7C%20WAV-green)
![launcher](https://img.shields.io/badge/bmorcelli%20Launcher-compatible-success)
![license](https://img.shields.io/badge/license-MIT-lightgrey)

---

## 简介

**Cardio** 把一台 Cardputer ADV 变成一个能装进口袋的本地音乐播放器：从 SD 卡播放 MP3/FLAC/WAV，
带一个可实时调的 5 段均衡器、文件浏览器和完整的串口调试控制台。下一步会加入**手机通知/来电直推到设备**
（BLE）。

- 硬件：ESP32-S3FN8（双核 240MHz，8MB Flash，**无 PSRAM**，512KB SRAM）
- 框架：C++ / Arduino（用 PlatformIO 构建）
- 兼容 [bmorcelli Launcher](https://github.com/bmorcelli/Launcher)，可与其他固件共存于一张卡

> ℹ️ **音频是单声道**：Cardputer ADV 的 ES8311 是单 DAC 单声道 codec，喇叭与 3.5mm 耳机共用这一路，
> 硬件无法分离左右声道。想要真立体声需外接 I2S codec —— 见 [外接 WM8960 方案](Cardio/PLAN_WM8960.md)。

---

## ✨ 功能特性

### ✅ 已实现
| 功能 | 说明 |
|---|---|
| 🎵 音乐播放 | MP3 / FLAC / WAV，精确时长解析（Xing/CBR、fmt/data、STREAMINFO）|
| 🎚️ 实时均衡器 | 5 段 peaking IIR（100/300/1k/3k/8k Hz，±12dB），`e` 键进界面实时调，全平时零开销旁路 |
| 🔊 音量曲线 | sqrt 预补偿抵消 M5.Speaker 内部平方，听感线性、位深顶高；`gain` 上限可调 |
| 📂 文件浏览器 | 播放列表 / 曲目导航，长按连发，页码指示 |
| 🔀 播放顺序 | 顺序 / 随机 / 单曲循环 / 列表循环（`r` 键切换）|
| 🈶 中日文字 | 逐字字体回退（efontCN→efontJA），简日混排不出豆腐块 |
| 🎬 开屏动画 | 纯代码绘制（无需 SD 资源）|
| 🔇 静音模式 | 开机静音配置（教室调试用，不丢音量值）|
| 🛠️ 调试控制台 | USB 串口，20+ 命令（播放/音量/EQ/配置/日志…）|
| 📝 分级日志 | Serial + SD 卡文件，超 512KB 自动轮转 |
| 🚀 Launcher 兼容 | 可被 bmorcelli Launcher 安装，`` ` `` 键 / `launcher` 命令返回 |

### 🚧 规划中（详见 [开发计划](#-开发状态--路线图)）
- 📲 **BLE 通知直推**：手机通知 / 来电 → 设备显示（NotifyOverlay / CallScreen）
- ⚙️ UI 收尾：SettingsScreen、PairingScreen、NoticeScreen、补全图标
- 📡 WiFi + RSS 拉取
- 📱 Android 客户端（通知监听 + BLE 推送）
- 🎧 外接 **WM8960** 立体声 codec（真立体声 + 耳放 + 硬件音量 + 喇叭/耳机切换）
- ☁️ 服务端（MQTT + FastAPI）—— 后续迭代

---

## 🔧 硬件

| 部件 | 规格 |
|---|---|
| 主控 | ESP32-S3FN8，双核 240MHz，8MB Flash，**无 PSRAM**，512KB SRAM |
| 音频 | ES8311 **单声道** codec → NS4150B 单声道功放 → 1W 喇叭 / 3.5mm 耳机（共用一路）|
| 显示 | 1.14" LCD，240×135 |
| 输入 | 56 键键盘（TCA8418 I2C）|
| 存储 | microSD 卡槽 |
| 无线 | WiFi 802.11 b/g/n + BLE 5.0（**无** Classic 蓝牙，不支持 A2DP 耳机）|

---

## 🚀 快速开始

### 1. 安装工具链
```bash
# macOS（用 pipx，不要用 pip）
brew install pipx && pipx install platformio
```

### 2. 编译 & 烧录
```bash
cd Cardio/firmware

pio run                            # 编译主固件（huge_app 3MB，直接 USB 烧录用）
pio run --target upload            # 编译并烧录到设备
pio device monitor --baud 115200   # 打开调试控制台

# 可选：编译可被 bmorcelli Launcher 安装的版本（app 上限 1.31MB）
pio run -e cardputer-adv-launcher
#   产物：.pio/build/cardputer-adv-launcher/firmware.bin
```

### 3. 准备 SD 卡
把模板复制到卡根目录，再放入音乐：
```
SD 根目录/
└── Cardio/
    ├── music/
    │   └── 我的歌单/          ← 每个子文件夹 = 一个播放列表
    │       ├── *.mp3 / *.flac / *.wav
    ├── config.txt             ← 主配置
    ├── rss_feeds.txt
    └── notify_filter.txt
```
详见 [SD 卡设置指南](Cardio/sdcard/README.md)。

---

## 🎮 操作

**播放器**
| 键 | 功能 | 键 | 功能 |
|---|---|---|---|
| `空格` | 播放 / 暂停 | `m` | 静音 |
| `,` `/` | 上一首 / 下一首 | `r` | 切换播放顺序 |
| `;` `.` | 音量 +/−（长按连调）| `e` | 均衡器 |
| `Enter` | 进文件浏览器 | `` ` `` | 退出到 Launcher |

**文件浏览器**：`;`/`,` 上移 · `.`/`/` 下移 · `Enter` 打开/播放 · `Del` 返回

**均衡器**：`,`/`/` 选频段 · `;`/`.` 增益 +/− · `0` 当前段归零 · `Del` 存盘返回

---

## 🛠️ 调试控制台

USB 串口（115200）连接后可用，分组如下：

| 类别 | 命令 |
|---|---|
| 播放 | `play <路径>` · `pause` · `resume` · `stop` · `next` · `prev` · `status` |
| 音频 | `vol <0-21>` · `gain <16-160>` · `eq <段 增益>\|flat\|show` · `mute [on\|off]` · `tone` |
| 列表 | `list` · `playlist <n>` · `tracks` · `order <模式>` |
| 系统 | `config get/set/save` · `log level <级别>` · `heap` · `jdet` · `launcher` · `reboot` · `help` |

示例：`eq 0 6`（100Hz +6dB）· `gain 64`（降低音量上限防破音）· `mute on`（教室静音）

---

## 📁 项目结构

```
Cardputer ADV/
├── README.md                 # 本文件
├── CLAUDE.md                 # 开发工作区说明（构建/约束/调试）
├── Cardio/
│   ├── ARCHITECTURE.md       # 系统架构、数据流、状态机
│   ├── PLAN.md               # 开发计划、里程碑、风险
│   ├── PLAN_UI_BLE.md        # UI 收尾 + BLE 通知 详细计划
│   ├── PLAN_WM8960.md        # 外接立体声 codec 计划（可选）
│   ├── CLIENT_PLAN.md        # 三端客户端设计
│   ├── firmware/             # Arduino C++ 固件
│   │   ├── Cardio.ino        # 入口：setup/loop，三屏路由，按键
│   │   ├── audio/            # AudioEngine / Equalizer / 输出桥接 / JackMonitor
│   │   ├── player/           # PlaybackController（内存库索引 + 播放顺序）
│   │   ├── ui/               # Player / Browser / EQ / Splash 屏 + Theme/字体
│   │   ├── config/ debug/    # Config（config.txt）/ Logger / DebugConsole
│   │   ├── platformio.ini    # 两个构建环境（主 / launcher）
│   │   └── partitions_launcher.csv
│   └── sdcard/Cardio/        # SD 卡模板（config.txt 等）
```

---

## 📋 开发状态 & 路线图

播放器内核 + 均衡器 + Launcher 兼容**已完成并实机验证**；通知系统与外接立体声为下一步。

- 详细任务、甘特图、里程碑 → [Cardio/PLAN.md](Cardio/PLAN.md)
- UI 收尾 + BLE 通知直推（每组件接口/数据流/内存预算/测试）→ [Cardio/PLAN_UI_BLE.md](Cardio/PLAN_UI_BLE.md)
- 外接 WM8960 立体声 codec → [Cardio/PLAN_WM8960.md](Cardio/PLAN_WM8960.md)

---

## 📚 文档导航

| 文档 | 内容 |
|---|---|
| [CLAUDE.md](CLAUDE.md) | 构建命令、关键约束（无 PSRAM/单声道/内存预算）、调试、Git 流程 |
| [ARCHITECTURE.md](Cardio/ARCHITECTURE.md) | 系统总览、固件内部架构、音频管线、状态机、SD 结构、依赖库 |
| [PLAN.md](Cardio/PLAN.md) | 代码量、里程碑甘特图、任务清单、风险登记 |
| [PLAN_UI_BLE.md](Cardio/PLAN_UI_BLE.md) | UI 收尾 + BLE 通知直推 详细实施计划 |
| [PLAN_WM8960.md](Cardio/PLAN_WM8960.md) | 外接 WM8960 立体声 codec 实施计划 |
| [CLIENT_PLAN.md](Cardio/CLIENT_PLAN.md) | Android / macOS / Windows 客户端设计 |
| [sdcard/README.md](Cardio/sdcard/README.md) | SD 卡准备与 config.txt 字段说明 |
| [docs/references/REFERENCES.md](Cardio/docs/references/REFERENCES.md) | 数据手册 / 原理图 / 库 离线资料库 + 索引 |

---

## 🙏 致谢

- [M5Stack](https://m5stack.com) — M5Unified / M5Cardputer / M5GFX
- [earlephilhower/ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) — 音频解码（必须 1.9.7）
- [bmorcelli/Launcher](https://github.com/bmorcelli/Launcher) — 多固件启动器
- [efont](https://github.com/tomyhero/efont) — CJK 像素字体
- AudioOutputM5Speaker 三缓冲桥接参考自社区 BrokenSignal 实现

---

## 📄 许可证

[MIT](LICENSE) © ZUENS2020
