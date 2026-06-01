# Cardio — Claude Code 工作区

## 项目概述

**Cardio** 是运行在 M5Stack Cardputer ADV 上的固件，集成音乐播放器和跨平台通知推送。

- 硬件：ESP32-S3（双核 240MHz，8MB OPI PSRAM），ES8311 音频编解码器，240×135 LCD
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
│   ├── firmware/                  # Arduino C++ 固件（待创建）
│   │   ├── Cardio.ino
│   │   ├── config/
│   │   │   └── Config.h/cpp
│   │   ├── debug/
│   │   │   ├── Logger.h/cpp       # 日志系统
│   │   │   └── DebugConsole.h/cpp # 串口/BLE 调试控制台
│   │   ├── audio/
│   │   │   ├── AudioEngine.h/cpp
│   │   │   └── JackMonitor.h/cpp
│   │   ├── player/
│   │   │   ├── PlaybackController.h/cpp
│   │   │   ├── Playlist.h/cpp
│   │   │   ├── LocalSource.h/cpp
│   │   │   └── RssSource.h/cpp
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
│   │       └── SplashScreen.h/cpp
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
board_build.arduino.memory_type = qio_opi  ; 8MB OPI PSRAM
board_build.f_cpu = 240000000L
monitor_speed = 115200
```

### 依赖库

| 库名 | 安装 |
|------|------|
| M5Cardputer | `lib_deps = m5stack/M5Cardputer` |
| ESP32-audioI2S | `schreibfaul1/ESP32-audioI2S` |
| NimBLE-Arduino | `h2zero/NimBLE-Arduino` |
| PubSubClient | `knolleary/PubSubClient` |
| arduinoWebSockets | `links2004/WebSockets` |
| ArduinoJson | `bblanchon/ArduinoJson` |
| TJpgDec | M5GFX 内置，无需单独安装 |

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
├── notify_filter.txt  ← 通知白名单（应用名=show|drop）
└── splash.jpg         ← 可选，开屏图片 240×135 JPEG
```

`sdcard/Cardio/` 目录为模板，将内容复制到 SD 卡根目录即可。

---

## 关键约束与注意事项

### 硬件
- **分区方案必须用 `huge_app.csv`**，默认 1.4MB App 分区不够
- **PSRAM 已确认**：`board_build.arduino.memory_type = qio_opi`，8MB OPI PSRAM
- ESP32-S3 **只有 BLE，没有 Classic Bluetooth**，不支持 A2DP 蓝牙耳机
- 3.5mm 耳机插入时自动切换 EQ 预设并暂停播放

### 音频
- 音频库：**ESP8266Audio 1.9.7 + M5.Speaker**（M5Unified 拥有 ES8311+I2S，解码器只解码，通过 AudioOutput 桥接 playRaw）
- **不用** ESP32-audioI2S（不配 ES8311 寄存器，且新版依赖 IDF5）
- I2S 输出固定 48kHz，所有格式都会重采样到 48kHz
- 支持格式：MP3 / FLAC / WAV / AAC / Opus（OGG Vorbis / M4A 待实机确认）
- 不支持：WMA、APE（无嵌入式解码器）

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
- 进度条每秒更新一次，用局部 Sprite 只推进度条区域
- **字体**：CJK / 日语 / 俄语统一用 `efontJA`（含汉字 + 平假名 + 片假名 + Cyrillic + Latin-1 Sup），**不用 `efontCN`**（缺日语假名和俄语）。Flash 多 +357KB，3MB 分区充裕。拉丁/数字用 JetBrains Mono VLW；像素风标题用 Silkscreen VLW。
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
