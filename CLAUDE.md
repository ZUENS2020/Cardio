# Cardio — Claude Code 工作区

## 项目概述

**Cardio** 是运行在 M5Stack Cardputer ADV 上的固件，集成音乐播放器和跨平台通知推送。

- 硬件：ESP32-S3FN8（双核 240MHz，8MB Flash，**无 PSRAM**，仅 512KB 内部 SRAM），ES8311 音频编解码器，240×135 LCD
- 固件语言：C++，Arduino 框架
- 通知服务端：Python FastAPI + Mosquitto MQTT
- 客户端：Android（Kotlin）/ macOS（Swift）/ Windows（C#）

详细设计见：
- [Cardio/ARCHITECTURE.md](Cardio/ARCHITECTURE.md) — 系统架构、数据流、状态机、SD 卡结构
- [Cardio/PLAN.md](Cardio/PLAN.md) — 开发计划、代码量、里程碑、风险点
- [Cardio/CLIENT_PLAN.md](Cardio/CLIENT_PLAN.md) — 三端客户端详细设计

---

## 仓库结构

```
Cardputer ADV/
├── CLAUDE.md                      # 本文件
├── .gitignore
├── Cardio/
│   ├── ARCHITECTURE.md
│   ├── PLAN.md
│   ├── CLIENT_PLAN.md
│   │
│   ├── firmware/                  # Arduino C++ 固件
│   │   ├── Cardio.ino
│   │   ├── config/
│   │   │   └── Config.h/cpp
│   │   ├── debug/
│   │   │   ├── Logger.h/cpp       # 日志系统
│   │   │   └── DebugConsole.h/cpp # 串口/BLE 调试控制台
│   │   ├── audio/
│   │   │   ├── AudioEngine.h/cpp
│   │   │   ├── AudioOutputM5Speaker.h/cpp  # 三缓冲零拷贝桥接 M5.Speaker
│   │   │   └── JackMonitor.h/cpp
│   │   ├── player/
│   │   │   ├── PlaybackController.h/cpp   # 内存音乐库索引 + 播放控制（已并入旧 LocalSource/Playlist）
│   │   │   └── RssSource.h/cpp            # RSS 拉取（后续迭代）
│   │   ├── net/
│   │   │   ├── WiFiManager.h/cpp
│   │   │   ├── MqttClient.h/cpp
│   │   │   └── BleProvisioner.h/cpp
│   │   ├── notify/
│   │   │   └── NotifyManager.h/cpp
│   │   └── ui/
│   │       ├── PlayerScreen.h/cpp
│   │       ├── BrowserScreen.h/cpp
│   │       ├── NotifyOverlay.h/cpp
│   │       ├── CallScreen.h/cpp
│   │       ├── SettingsScreen.h/cpp
│   │       ├── SplashScreen.h/cpp        # 开屏（纯代码绘制，无 GIF）
│   │       ├── Canvas.h/cpp              # Player/Browser 共用的 240×135 sprite（省 63KB）
│   │       ├── TextRender.h/cpp          # CJK 逐字字体回退 printUtf8()
│   │       └── Theme.h                   # 配色 / 尺寸 / Order 枚举
│   │
│   ├── server/                    # 服务端（待创建）
│   │   ├── docker-compose.yml
│   │   ├── mosquitto/mosquitto.conf
│   │   └── api/
│   │       ├── main.py
│   │       └── requirements.txt
│   │
│   ├── client/                    # 客户端（待创建）
│   │   ├── android/
│   │   ├── macos/
│   │   └── windows/
│   │
│   ├── tools/
│   │   └── splash-converter/      # index.html（待创建）
│   │
│   └── sdcard/                    # SD 卡初始文件模板
│       └── Cardio/
│           ├── music/.gitkeep
│           ├── config.txt
│           ├── rss_feeds.txt
│           └── notify_filter.txt
```

---

## 固件开发

### 构建工具

推荐使用 **PlatformIO**（比 Arduino IDE 更适合多文件项目）：

```bash
# 安装（macOS 用 pipx，不要用 pip/pip3）
brew install pipx && pipx install platformio

# 编译
cd Cardio/firmware
pio run

# 编译并烧录
pio run --target upload

# 串口监视器（调试控制台）
pio device monitor --baud 115200

# 清理
pio run --target clean
```

或使用 **Arduino CLI**：

```bash
# 安装 M5Stack 板管理
arduino-cli core install esp32:esp32

# 编译
arduino-cli compile --fqbn esp32:esp32:m5stack_stamps3 Cardio/firmware

# 烧录
arduino-cli upload --fqbn esp32:esp32:m5stack_stamps3 --port /dev/tty.usbmodem* Cardio/firmware
```

### 关键配置（platformio.ini）

```ini
[env:cardputer-adv]
platform = espressif32
board = m5stack-stamps3
framework = arduino
board_build.partitions = huge_app.csv   ; 必须，默认分区放不下
; 无 PSRAM：不要设 memory_type=qio_opi / BOARD_HAS_PSRAM（芯片没有这颗 PSRAM）
board_build.f_cpu = 240000000L
monitor_speed = 115200
```

### 依赖库

| 库名 | 安装 | 说明 |
|------|------|------|
| M5Cardputer | `m5stack/M5Cardputer` | 含 M5Unified / M5GFX |
| ESP8266Audio | `earlephilhower/ESP8266Audio@1.9.7` | **必须 1.9.7**，2.x 需 IDF5 |
| NimBLE-Arduino | `h2zero/NimBLE-Arduino` | BLE GATT Server |
| ArduinoJson | `bblanchon/ArduinoJson` | 通知 JSON 解析（Week 2） |
| AnimatedGIF | （当前未用）| 开屏曾用 GIF，现已改**纯代码绘制**（`ui/SplashScreen.cpp`）；如要 GIF 开屏再启用 |
| PubSubClient | `knolleary/PubSubClient` | MQTT（后续迭代） |
| arduinoWebSockets | `links2004/WebSockets` | MQTT WSS（后续迭代） |
| TJpgDec | M5GFX 内置 | 开屏 JPG 回退（当前未用，开屏为纯代码绘制）|

### 调试

设备通过 USB 连接后：

```bash
# 串口调试控制台（config.txt 中 debug_enabled=true）
pio device monitor --baud 115200

# 常用调试命令
status          # 播放状态
heap            # 内存使用
wifi status     # 网络状态
log level debug # 切换日志级别
notify 微信 测试消息  # 模拟通知
call 张三       # 模拟来电
```

BLE 调试：设备广播 `Cardio-Debug`，用 nRF Connect 连接 NUS Service 发送同样命令。

### 日志格式

```
[000123456][INFO ][AUDIO] Playing: 稻香.flac
[000125210][ERROR][MQTT ] Connect timeout
[000130000][WARN ][WIFI ] Signal weak, RSSI=-85
```

SD 卡日志路径：`/Cardio/logs/cardio.log`（超 512KB 自动轮转）。

---

## 服务端开发

```bash
cd Cardio/server

# 本地启动（开发）
cd api && pip install -r requirements.txt && uvicorn main:app --reload

# Docker 部署（在服务器ssh nec上使用docker，本机无docker环境）
docker-compose up -d

# 查看日志
docker-compose logs -f
```

测试推送通知：

```bash
curl -X POST http://localhost:8000/notify \
  -H "Content-Type: application/json" \
  -d '{"source":"微信","content":"测试消息","priority":"normal"}'
```

---

## 客户端开发

### Android（本期）
- Android Studio，Kotlin，目标 API 26+
- 需要手动授予通知访问权限（Settings → Notification Access）
- 详细设计见 [CLIENT_PLAN.md](Cardio/CLIENT_PLAN.md)

### macOS / Windows（后续迭代，暂不开发）
- macOS：Swift + NSStatusItem + SQLite 轮询 Notification Center DB
- Windows：C# .NET 8 + WPF + WinRT UserNotificationListener

---

## SD 卡结构

所有固件文件统一放在 `/Cardio/` 下，与其他固件隔离：

```
/Cardio/
├── music/
│   └── {播放列表}/    ← 每个子文件夹是一个播放列表
├── logs/              ← 自动创建，调试日志
├── config.txt         ← 主配置文件
├── rss_feeds.txt      ← RSS 源列表（名称|URL，每行一条）
└── notify_filter.txt  ← 通知白名单（应用名=show|drop）
```

> 开屏动画当前为**纯代码绘制**（`ui/SplashScreen.cpp`，CARDIO + ZUENS2020 + 品牌斜条纹扫光），不读 SD 上的 `splash.gif/jpg`。早期 GIF 方案因 512KB SRAM 装不下全屏 GIF 解码而放弃（详见"关键约束"）。

`sdcard/Cardio/` 目录为模板，将内容复制到 SD 卡根目录即可。

---

## 关键约束与注意事项

### 硬件
- **分区方案必须用 `huge_app.csv`**，默认 1.4MB App 分区不够
- **无 PSRAM（已核实）**：芯片是 `ESP32-S3FN8`（flash-only，无 R 后缀），官方文档/商店均无 PSRAM；运行时报 `PSRAM ID read error: 0x00000000` 即"找不到 PSRAM 芯片"。**不要**配 `qio_opi` / `BOARD_HAS_PSRAM` / `psram_type=opi`，用官方 `m5stack-stamps3` 板子即可。早期"8MB OPI PSRAM 已确认"是错误假设。
- **内存预算只有 512KB SRAM（启动后约 170KB 可用堆）**：两个常驻屏 sprite 各 63KB（共 126KB）+ 音频解码器 ~30-40KB 已很紧；新增大块内存（如全屏 GIF、第三个 sprite）易 OOM 崩溃/重启。`setPsram(true)` 在本板无意义（M5GFX 会回退 SRAM），勿用。
- ESP32-S3 **只有 BLE，没有 Classic Bluetooth**，不支持 A2DP 蓝牙耳机
- 3.5mm 耳机插入时硬件自动切换扬声器/耳机路由（模拟机械开关），固件无法检测插拔事件；自动暂停功能推迟至后续迭代

### 音频
- 音频库：**ESP8266Audio 1.9.7 + M5.Speaker**（M5Unified 拥有 ES8311+I2S，解码器只解码，通过 AudioOutputM5Speaker 三缓冲零拷贝桥接 playRaw）
- **不用** ESP32-audioI2S（不配 ES8311 寄存器，且新版依赖 IDF5）
- I2S 输出 96kHz stereo（M5.Speaker PWM），所有格式都会重采样到 96kHz
- 支持格式：MP3 / FLAC / WAV（AAC / Opus 待实机确认；OGG Vorbis / M4A 无解码器，不支持）
- 不支持：WMA、APE、OGG Vorbis、M4A 容器（无嵌入式解码器）

### 网络
- WiFi 和 BLE 可共存（时分复用），BLE 激活时 WiFi 吞吐略降
- **本期通知仅走 BLE 直推**（`notify_mode=ble`）；WiFi 仅用于 RSS 拉取
- MQTT / 服务端路径为**后续迭代**，本期不实现
- "连不上"判定（RSS 场景）：HTTP 请求超时，不是 WiFi 关联失败

### BLE 直推（本期通知路径）
- 固件广播 `Cardio-XXXX`，Android App 扫描并连接 GATT Service 0xFF00
- 通知内容通过 PushRX Characteristic（0xFF04）写入，设备回 PushACK（0xFF05）
- NUS Service（6E400001...）供 DebugConsole BLE 通道使用

### BLE WiFi 回退（后续迭代）
1. 服务不可达 → BLE 广播 `cardio/req-wifi` → App 发送当前 WiFi 凭据
2. 仍不可达 → BLE 广播 `cardio/req-hotspot` → App 引导开热点
3. 依赖服务端 MQTT，本期不实现

### UI
- 全部使用 Sprite 离屏渲染后 `pushSprite`，避免闪烁
- 进度条每秒更新一次（当前全屏重绘，后续优化为局部 Sprite 只推进度条区域）
- **字体**：CJK 文本走 **逐字回退**（`ui/TextRender.cpp` 的 `theme::printUtf8()`）——每个码点先查 `efontCN`（简体汉字全，如 风/边），CN 没有的（平假名/片假名/西里尔/日文专用汉字如 冴）回退 `efontJA`。单用任一 efont 都会在简日混排时出豆腐方块：`efontJA` 缺简体专用字，`efontCN` 缺假名/西里尔。两个 efont 各 ~158KB 都链接进固件，3MB 分区充裕。所有渲染 CJK 的地方（PlayerScreen 标题/艺术家、BrowserScreen 文件夹/曲目/顶栏）都用 `printUtf8()` 代替 `spr.print()`。拉丁/数字用 JetBrains Mono VLW；像素风标题用 Silkscreen VLW。
- 屏幕数量：6 个主屏（Player/Browser/Notify/Call/Settings/Splash）+ 2 个补充屏（PairingScreen BLE 配对码、NoticeScreen 无 SD/离线错误）
- 图标：24 个像素风 16×16 图标编译进固件（`fillRect` 数组，无需 SD），另需补画 8 个（pause/prev/next/note/heart/phone/bell/list）

---

## Git 工作流

```bash
# 日常提交
git add Cardio/firmware/...
git commit -m "feat(audio): add ES8311 ALC disable"
git push

# 提交格式：type(scope): description
# type: feat | fix | refactor | docs | chore
# scope: audio | player | mqtt | ui | ble | server | android | macos | windows
```
