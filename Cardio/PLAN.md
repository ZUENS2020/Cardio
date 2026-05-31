# Cardio — 开发计划

## 代码量估算

### 固件各模块

| 模块 | 文件 | 估算行数 | 说明 |
|---|---|---|---|
| 主入口 | Cardio.ino | 80 | setup/loop，模块初始化 |
| Config | Config.h/cpp | 120 | 读写 config.txt，键值对解析 |
| Logger | Logger.h/cpp | 120 | 分级日志，输出到 Serial / BLE NUS / SD 卡文件 |
| DebugConsole | DebugConsole.h/cpp | 220 | 命令解析分发，Serial + BLE NUS 双通道 |
| AudioEngine | AudioEngine.h/cpp | 150 | ESP32-audioI2S 封装，ES8311 寄存器配置，增益分级 |
| JackMonitor | JackMonitor.h/cpp | 60 | GPIO 中断，插拔事件回调 |
| PlaybackController | PlaybackController.h/cpp | 200 | 核心状态机，衔接音源/顺序/引擎 |
| Playlist + PlayOrder | Playlist.h/cpp | 180 | 列表数据结构，5 种顺序算法（含 Fisher-Yates shuffle） |
| LocalSource | LocalSource.h/cpp | 120 | 扫描 /Cardio/music/，构建文件夹列表 |
| RssSource | RssSource.h/cpp | 180 | HTTP 拉取，轻量 XML 解析（匹配 title/enclosure） |
| WiFiManager | WiFiManager.h/cpp | 80 | 连接、重连、状态回调 |
| BlePushService | BlePushService.h/cpp | 100 | GATT PushRX Characteristic，notify_mode=ble 时接收手机直推 |
| MqttClient | MqttClient.h/cpp | 130 | WSS 连接，订阅 notify/#，心跳重连，notify_mode=wifi 时启用 |
| NotifyManager | NotifyManager.h/cpp | 160 | 状态机，白名单过滤，BLE/WiFi 两路输入统一路由 |
| PlayerScreen | PlayerScreen.h/cpp | 200 | 封面、歌名、进度条、状态图标 |
| BrowserScreen | BrowserScreen.h/cpp | 180 | 列表/文件夹/RSS 统一浏览，光标导航 |
| NotifyOverlay | NotifyOverlay.h/cpp | 80 | 顶部通知条，5s 计时淡出 |
| CallScreen | CallScreen.h/cpp | 100 | 全屏来电，来源/内容显示，关闭按键 |
| SettingsScreen | SettingsScreen.h/cpp | 150 | 运行时开关，写回 config.txt |
| **固件合计** | | **~2610 行** | |

### 服务端

| 模块 | 文件 | 估算行数 | 说明 |
|---|---|---|---|
| 通知转发 API | main.py | 120 | FastAPI，HTTP POST → MQTT publish，优先级过滤 |
| Mosquitto 配置 | mosquitto.conf | 20 | listener 1883/9001，auth |
| Docker Compose | docker-compose.yml | 30 | Mosquitto + FastAPI 服务 |
| **服务端合计** | | **~170 行** | |

### 客户端通知捕获

| 平台 | 文件 | 估算行数 |
|---|---|---|
| Mac | notify_capture.py | 80 |
| Windows | notify_capture.py | 80 |
| Android | tasker_profile.xml | — （Tasker 图形配置） |
| **客户端合计** | | **~160 行** | |

### 总计

| 部分 | 语言 | 行数 |
|---|---|---|
| 固件 | C++ | ~2610 |
| 服务端 Python | ~170 | notify_mode=wifi 时才需要 |
| Android 客户端 | Kotlin | ~765 |
| **总计** | | **~3545 行** |

> macOS / Windows 客户端暂不开发，后续迭代再加。

> 客户端详细计划见 [CLIENT_PLAN.md](CLIENT_PLAN.md)

---

## 开发里程碑（2 周压缩版）

精致化功能（封面、EQ、NVS 续播、省电）移至后续迭代，2 周内交付可用版本。

```mermaid
gantt
    title Cardio 2-Week Sprint
    dateFormat  YYYY-MM-DD
    axisFormat  %m/%d

    section Week 1 固件核心
    Logger + DebugConsole（Serial）      :w1a0, 2026-05-31, 1d
    Config + AudioEngine + JackMonitor   :w1a, after w1a0, 2d
    Playlist + LocalSource + PlayOrder   :w1b, after w1a, 2d
    PlayerScreen + BrowserScreen         :w1c, after w1b, 1d
    WiFiManager + BLE 回退流程           :w1d, after w1c, 2d

    section Week 1 服务端
    服务端部署 Mosquitto + FastAPI        :s1, 2026-05-31, 2d
    CF Tunnel 配置                       :s2, after s1, 1d

    section Week 2 网络功能
    MqttClient WSS + NotifyManager       :w2a, 2026-06-07, 2d
    RssSource + NotifyOverlay + CallScreen :w2b, after w2a, 2d
    BleProvisioner 固件端                :w2c, after w2a, 1d

    section Week 2 客户端
    Android 核心（通知 + 上传 + BleWifiHelper） :a1, 2026-06-07, 4d

    section Week 2 收尾
    集成测试 + SettingsScreen            :t1, 2026-06-12, 2d
```

---

## 任务清单

### Week 1 Day 1 — Logger + DebugConsole（最先做）

**原则：所有后续模块开发直接用 LOG_X 埋点，不再用 Serial.println**

- [ ] `Logger`：全局单例，宏接口 `LOG_D/I/W/E(tag, fmt, ...)`
  - Serial 实时输出（USB CDC 115200）
  - SD 卡写入 `/Cardio/logs/cardio.log`，超 512KB 自动轮转（保留 3 个文件）
  - 日志格式：`[000123456][INFO ][AUDIO] Playing: 稻香.flac`
  - 运行时级别过滤，`debug_enabled=false` 时关闭 SD 写入仅保留 Serial
- [ ] `DebugConsole`（Serial 通道）：主循环轮询 `Serial.readStringUntil('\n')`，启动时打印命令列表

**此阶段可用命令（随模块增加逐步扩充）：**
```
heap          显示 SRAM/PSRAM 剩余
log level <debug|info|warn|error>   设置日志级别
log dump      输出当前日志文件全部内容
log clear     清空日志文件
reboot        重启设备
```

---

### Week 1 Day 2-3 — 固件基础层

- [ ] 分区方案设为 `huge_app.csv`（默认 1.4MB 放不下所有库，需改为 3MB App 分区）
- [ ] Config：解析 config.txt，读取所有开关和配置项，`mqtt_port` 默认 443
- [ ] AudioEngine：ES8311 I2S 引脚确认，48kHz/24-bit，关 ALC，增益分级，DMA buffer 调优
- [ ] JackMonitor：GPIO 边沿中断，插拔立即暂停
- [ ] 基础播放验收：SD 卡放 FLAC，Space/←/→/音量键能用，拔耳机暂停

**新增调试命令：**
```
play <路径>    volume <0-21>    pause/resume    status
jack           battery          config get/set
```

### Week 1 Day 3-4 — 播放列表

- [ ] LocalSource：扫描 `/Cardio/music/` 一级子文件夹，散文件归入默认列表
- [ ] 支持格式：MP3 / FLAC / WAV / AAC / M4A / OGG / Opus（ESP32-audioI2S 全部内置，无需额外工作）
- [ ] Playlist + PlayOrder：5 种顺序，fn+O 切换
- [ ] BrowserScreen：↑↓ 移动，Enter 播放，长按 fn 进入列表选择
- [ ] PlayerScreen：歌名、艺术家（ID3 回调）、进度条、mm:ss

### Week 1 Day 5 — UI 收尾

- [ ] RssSource 列表与本地列表在 BrowserScreen 混合显示（图标区分 📁 / 📡）
- [ ] SettingsScreen 骨架（开关页，后续扩充）

### Week 1 Day 6-7 — 网络 + BLE 回退

- [ ] WiFiManager：连接 NVS 凭据 → 失败或服务不可达 → 触发 BLE 回退
- [ ] BleProvisioner：NimBLE GATT Server，Service 0xFF00，含 WiFi 配网（0xFF01/02）和 DevStatus（0xFF03）
- [ ] BlePushService：在 0xFF00 追加 PushRX（0xFF04）和 PushACK（0xFF05），`notify_mode=ble` 时激活
- [ ] BleProvisioner 追加 NUS Service（6E400001...）：DebugConsole BLE 通道接入
- [ ] 服务端并行：docker-compose 部署 Mosquitto + FastAPI，CF Tunnel 配置

**新增调试命令：**
```
wifi status/connect    mqtt status    mqtt pub <topic> <内容>
```

---

### Week 2 Day 1-2 — 固件网络功能

- [ ] MqttClient：WiFiClientSecure + WebSocketsClient + PubSubClient，WSS 连接（`notify_mode=wifi` 时启用）
- [ ] NotifyManager：JSON 解析，状态机（IDLE/NOTIFY/CALL），统一接收 BLE 和 MQTT 两路输入
- [ ] 白名单：从 `notify_filter.txt` 读取
- [ ] NotifyOverlay：顶部通知条，5s 自动消失，音乐继续
- [ ] CallScreen：全屏来电，音乐暂停，用户关闭后恢复

**新增调试命令：**
```
notify <来源> <内容>    call <姓名>    ui <screen>
rss refresh    rss list    screen <on|off>
```

### Week 2 Day 1-4 — Android 客户端（可与固件并行）

- [ ] NotificationListenerService + CallListener + SmsReceiver
- [ ] FilterTable + Uploader（OkHttp，带重试）
- [ ] BleWifiHelper：扫描 GATT Notify，响应 req-wifi / req-hotspot
- [ ] WifiCredential：SSID→密码 DataStore，热点凭据缓存
- [ ] 设置页：权限引导（一键跳转各权限设置页）、服务地址、WiFi 密码列表

### Week 2 Day 5-7 — 集成测试 + RSS

- [ ] RssSource：HTTPClient 拉取 XML，解析 `<title>` / `<enclosure url=`
- [ ] WiFiManager + RssSource 联调（rss_enabled 控制）
- [ ] 全链路测试：手机通知 → FastAPI → MQTT → Cardputer 显示
- [ ] BLE 回退流程测试：断网 → req-wifi → req-hotspot → 恢复
- [ ] SettingsScreen：运行时切换开关，写回 config.txt

---

## 后续迭代（2 周后）

| 功能 | 工期估算 |
|---|---|
| 封面图（TJpgDec） | 2d |
| 硬件 EQ（ES8311 DSP 寄存器） | 1d |
| NVS 断电续播 | 1d |
| 省电息屏 + 低电警告 | 1d |
| 自定义开屏图片 | 1d |
| Splash Converter 工具 | 1d |
| macOS 客户端 | 3d |
| Windows 客户端 | 3d |
| iOS 客户端（如有需要） | 5d |
| Web 配置页面 | 待定 |

---

## Web 配置页面

ADV 连上 WiFi 后在屏幕显示本机 IP，用户在同一局域网浏览器访问即可配置。

**技术方案：**
- 固件端运行 `ESPAsyncWebServer`（轻量异步 HTTP 服务器，Arduino 库）
- 配置页面 HTML 内嵌进固件 Flash（PROGMEM），不占 SD 卡
- REST API：`GET /api/config` 读取当前配置，`POST /api/config` 写入并保存到 NVS
- 需要 WiFi 已连接，config.txt 中加开关 `webui_enabled=true`

**可配置项：** 待定

---

## 自定义开屏图片

### 固件端（SplashScreen.h/cpp，~50 行）

启动时检测 `/Cardio/splash.jpg`，存在则显示 2 秒（任意键跳过），不存在直接进主界面。使用 M5GFX 内置的 `drawJpgFile`，复用已有的 TJpgDec，零额外依赖。

### Splash Converter 工具（tools/splash-converter/index.html，单文件）

纯 HTML + Canvas，双击浏览器直接打开，无需安装任何东西。

**功能：**
- 拖拽 / 点击上传图片（JPG / PNG / WebP / GIF / BMP）
- 四种模式：自由裁剪 / 填满裁剪 / 信箱适配 / 拉伸填满
- 自由裁剪模式：鼠标拖拽画裁剪框，8 个控制点调整大小，可选锁定 16:9 比例
- 信箱适配：可选背景色（黑 / 白 / 深蓝 / 自定义）
- JPEG 质量滑块（10-100，默认 85）
- 实时预览（240×135 实际尺寸）
- 一键下载 `splash.jpg`，拷贝到 SD 卡 `/Cardio/splash.jpg` 即用

---

## 风险点

| 风险 | 可能性 | 应对 |
|---|---|---|
| ESP32-audioI2S 与 M5Cardputer 库 I2S 冲突 | 中 | Week 1 最先验证，必要时手动接管 ES8311 初始化 |
| ADV 的 ES8311 引脚文档不全 | 中 | 参考 AndyAiCardputer/cardputer-adv-tests 仓库的测试代码 |
| CF Tunnel WSS 延迟导致 MQTT 断线 | 低 | PubSubClient 设置 keepalive=60，断线自动重连 |
| RssSource XML 格式不规范 | 中 | 只匹配最小必要字段，容错处理 |
| PSRAM 不足导致音频 buffer 溢出 | 低 | ADV 有 8MB PSRAM，足够；监控堆使用 |
