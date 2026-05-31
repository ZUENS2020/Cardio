# Cardio — 系统架构

## 硬件平台

M5Stack Cardputer ADV
- 主控：ESP32-S3FN8（双核 240MHz，8MB PSRAM）
- 音频：ES8311 编解码器 → NS4150B 功放 → 1W 扬声器 / 3.5mm 耳机
- 显示：1.14" LCD 240×135
- 输入：56 键键盘
- 存储：MicroSD 卡槽

---

## 系统总览

```mermaid
graph TB
    subgraph 通知来源
        A1[Android / Tasker]
        A2[Mac 脚本]
        A3[Windows 脚本]
    end

    subgraph 内网服务端
        B1[FastAPI\n通知转发 API]
        B2[Mosquitto\nMQTT Broker]
        B3[CF Tunnel\nWebSocket 穿透]
    end

    subgraph Cardputer ADV
        C1[WiFiManager]
        C2[MqttClient\nPubSubClient over WSS]
        C3[PlaybackController]
        C4[AudioEngine\nCore 0]
        C5[UI Layer]
        C6[NotifyManager]
    end

    A1 & A2 & A3 -->|HTTP POST| B1
    B1 -->|publish| B2
    B2 --- B3
    B3 -->|MQTT over WSS 443| C2
    C2 --> C6
    C6 --> C5
    C3 --> C4
    C3 --> C5
    C1 --> C2
    C1 --> C3
```

---

## 固件内部架构

```mermaid
graph TB
    subgraph Core0["Core 0（音频专用）"]
        AE[AudioEngine\nESP32-audioI2S\nES8311 I2S 输出]
    end

    subgraph Core1["Core 1（主循环）"]
        CFG[Config\n读写 config.txt]
        WIFI[WiFiManager]
        JM[JackMonitor\n3.5mm 插拔中断]

        subgraph Player["播放器"]
            PC[PlaybackController]
            LS[LocalSource\n扫描 /Cardio/music/]
            RS[RssSource\nHTTP + XML 解析]
            PL[Playlist + PlayOrder\n5种顺序]
        end

        subgraph Notify["通知系统（可选）"]
            MC[MqttClient\nWSS over CF Tunnel]
            NM[NotifyManager\n状态机 + 白名单]
        end

        subgraph UI["UI Layer"]
            PS[PlayerScreen]
            BS[BrowserScreen]
            NO[NotifyOverlay]
            CS[CallScreen]
            SS[SettingsScreen]
        end
    end

    CFG -->|开关配置| WIFI & PC & NM
    WIFI -->|连接成功| MC & RS
    JM -->|插拔事件| PC
    LS & RS --> PL --> PC
    PC --> AE
    MC --> NM
    NM -->|NOTIFY| NO
    NM -->|CALL| CS
    PC --> PS
    PL --> BS
    SS -->|写回| CFG
```

---

## 通知状态机

```mermaid
stateDiagram-v2
    [*] --> IDLE

    IDLE --> NOTIFY : 收到普通通知\n(微信/短信/App)
    IDLE --> CALL   : 收到 notify/phone/call

    NOTIFY --> IDLE : 5秒超时 / 用户按键
    CALL   --> IDLE : 用户关闭来电界面

    IDLE   : 正常播放器 UI\n音乐播放中
    NOTIFY : 顶部 Overlay 显示\n音乐继续播放
    CALL   : 全屏来电界面\n音乐暂停
```

---

## MQTT Topic 结构

```mermaid
graph LR
    ROOT["notify/#"]
    ROOT --> PHONE["notify/phone/"]
    ROOT --> PC["notify/pc/"]
    PHONE --> CALL["notify/phone/call\n→ CALL 状态"]
    PHONE --> SMS["notify/phone/sms\n→ NOTIFY 状态"]
    PHONE --> APP["notify/phone/app/{name}\n→ 按白名单决定"]
    PC --> PCAPP["notify/pc/app/{name}\n→ 按白名单决定"]
```

---

## WiFi 网络回退流程（Android 专属）

> 仅在 `notify_enabled=true` 或 `rss_enabled=true` 时生效。
> 两者均关闭时跳过整个流程，设备不尝试联网。

**"连不上"的判定标准：应用层可达性，而非 WiFi 关联状态。**
- notify 开启时：尝试 TCP 连接 MQTT 服务器（5s 超时）
- rss 开启时：尝试 HTTP HEAD 请求第一条 RSS URL（5s 超时）
- 以上任一失败即视为"连不上"

```mermaid
flowchart TD
    A([启动]) --> B{notify 或\nrss 已开启?}
    B -- 否 --> Z([跳过联网\n离线模式])
    B -- 是 --> C[连接 NVS 中保存的 WiFi]
    C -- 连接失败 --> E
    C -- 连接成功 --> D{服务可达性测试\nMQTT / RSS}
    D -- 通过 --> OK([正常运行])
    D -- 失败 --> E[BLE 广播\ncardio/req-wifi]

    E --> F{Android App\n在附近?}
    F -- 否/超时 30s --> H[BLE 广播\ncardio/req-hotspot]
    F -- 是 --> G[App 通过 BLE GATT\n发送当前连接的 WiFi 凭据]
    G --> G2[切换 WiFi 重连\n再次测试可达性]
    G2 -- 通过 --> OK2([正常运行\n保存新凭据到 NVS])
    G2 -- 失败 --> H

    H --> I{Android App\n在附近?}
    I -- 否/超时 30s --> ERR([屏幕提示无网络\n继续离线播放])
    I -- 是 --> J[App 推系统通知\n跳转热点开关\n用户一键开启]
    J --> K[App 通过 BLE GATT\n发送热点 SSID/密码]
    K --> L[连接热点\n再次测试可达性]
    L -- 通过 --> OK3([正常运行\n记录热点凭据备用])
    L -- 失败 --> ERR
```

**BLE GATT 新增信号（扩展现有配网服务）：**

```
Service 0xFF00（沿用）
  ├── 0xFF03  Status NOTIFY（设备 → 手机）
  │           新增值：
  │           "req-wifi"    → 请求当前 WiFi 凭据
  │           "req-hotspot" → 请求开启热点
  │           "connected"   → 连接成功
  │           "failed"      → 连接失败
  ├── 0xFF01  SSID     WRITE（手机 → 设备，沿用）
  └── 0xFF02  Password WRITE ENCRYPTED（手机 → 设备，沿用）
```

**iOS / macOS / Windows 客户端行为：**
收到 `req-wifi` 或 `req-hotspot` 信号时忽略，不做任何响应。

---

## 网络连接路径

```mermaid
sequenceDiagram
    participant Card as Cardputer ADV
    participant CF as CF Tunnel
    participant MQ as Mosquitto
    participant API as FastAPI
    participant Phone as Android/Tasker

    Card->>CF: WSS 443 连接\nyour-name.trycloudflare.com/mqtt
    CF->>MQ: WS 转发 ws://localhost:9001
    Card->>MQ: MQTT CONNECT（通过 CF Tunnel）
    MQ-->>Card: CONNACK

    Phone->>API: HTTP POST /notify
    API->>MQ: MQTT publish notify/phone/call
    MQ->>CF: 推送
    CF->>Card: MQTT 消息到达
    Card->>Card: 暂停音乐 + 显示来电界面
```

---

## SD 卡目录结构

```
SD 根目录/
└── Cardio/
    ├── music/
    │   ├── {播放列表名}/       ← 每个子文件夹 = 一个播放列表
    │   │   ├── *.flac
    │   │   └── *.mp3
    │   └── *.mp3               ← 根目录散文件归入默认列表
    ├── config.txt
    ├── rss_feeds.txt
    └── notify_filter.txt
```

**config.txt：**
```ini
# WiFi
wifi_ssid=MyNetwork
wifi_pass=password123

# MQTT
mqtt_host=your-name.trycloudflare.com
# mqtt_port=443        # 可选，默认 443；使用 frp 等工具时填写实际端口
mqtt_path=/mqtt
mqtt_user=cardio
mqtt_pass=yourpassword

# 功能开关
notify_enabled=true
rss_enabled=true

# 调试
debug_enabled=false   # true 时启用调试控制台（Serial + BLE NUS）
debug_ble=false       # true 时额外开启 BLE NUS 通道（需 debug_enabled=true）
log_level=info        # debug | info | warn | error

# 播放器
default_volume=15
default_order=sequential
```

**rss_feeds.txt：**
```
硬核节目|https://feeds.example.com/hardcore.xml
英语听力|https://feeds.example.com/english.xml
```

**notify_filter.txt：**
```
微信=show
短信=show
支付宝=show
微博=drop
```

---

## 服务端部署结构

```
server/
├── docker-compose.yml
├── mosquitto/
│   └── mosquitto.conf      # 开启 WebSocket listener 9001
└── api/
    ├── main.py             # FastAPI 接收 HTTP POST → 转发 MQTT
    └── requirements.txt
```

**CF Tunnel 路由：**
```yaml
ingress:
  - hostname: your-name.trycloudflare.com
    path: /mqtt
    service: ws://localhost:9001
  - service: http_status:404
```

---

## 调试控制台与日志系统

### Logger

全局单例，所有模块统一使用宏接口：

```cpp
LOG_D("AUDIO", "Buffer: %d/%d", used, total);   // DEBUG
LOG_I("WIFI",  "Connected: %s", ssid);           // INFO
LOG_W("MQTT",  "Reconnecting, attempt %d", n);   // WARN
LOG_E("RSS",   "Parse failed: %s", url);         // ERROR
```

输出格式：
```
[000123456][INFO ][WIFI ] Connected: MyNetwork
[000125210][ERROR][MQTT ] Connect timeout
```

| 输出目标 | 条件 |
|----------|------|
| Serial（USB CDC 115200） | 始终输出（`debug_enabled` 无关） |
| SD `/Cardio/logs/cardio.log` | `debug_enabled=true` 时写入 |
| BLE NUS TX | `debug_ble=true` 且已连接时推送 |

日志文件超 512KB 自动轮转：`cardio.log` → `cardio.log.1` → `cardio.log.2`（保留 3 个）。

---

### DebugConsole

```mermaid
graph LR
    USB[USB Serial\n115200] --> DC[DebugConsole\n命令解析分发]
    BLE[BLE NUS RX\n6E400002] --> DC
    DC --> AE[AudioEngine]
    DC --> PC[PlaybackController]
    DC --> WM[WiFiManager]
    DC --> MQ[MqttClient]
    DC --> NM[NotifyManager]
    DC --> RS[RssSource]
    DC --> CFG[Config]
    DC --> UI[UI Layer]
    DC --> LOG[Logger]
    DC --> OUT[输出\nSerial + BLE NUS TX]
```

**`debug_enabled=false` 时 DebugConsole 完全不初始化，零开销。**

**完整命令集：**

```
── 播放 ──────────────────────────────────
play <路径>              播放指定文件
pause / resume           暂停 / 恢复
stop                     停止
next / prev              切曲
volume <0-21>            音量
seek <秒>                跳转
status                   当前播放状态

── 播放列表 ──────────────────────────────
list                     列出所有播放列表
playlist <名称>          切换播放列表
tracks                   列出当前列表曲目
order <模式>             切换播放顺序

── 网络 ──────────────────────────────────
wifi status              连接状态
wifi connect             强制重连
mqtt status              MQTT 状态
mqtt pub <topic> <内容>  发布测试消息

── 通知模拟 ──────────────────────────────
notify <来源> <内容>     模拟普通通知
call <姓名>              模拟来电

── RSS ───────────────────────────────────
rss refresh              强制刷新所有源
rss list                 列出已拉取节目

── 日志 ──────────────────────────────────
log level <级别>         设置日志级别
log dump                 输出日志文件全部内容
log clear                清空日志文件

── 系统 ──────────────────────────────────
heap                     SRAM / PSRAM 剩余
battery                  电池电压和百分比
jack                     3.5mm 插拔状态
config get <key>         读取配置项
config set <key> <val>   修改配置项（写回 NVS）
ui <screen>              强制切换界面
screen <on|off>          屏幕开关
reboot                   重启
```

**BLE 连接方式：**
- 设备开启后广播名 `Cardio-Debug`
- 电脑 / 手机用 nRF Connect 或 Serial Bluetooth Terminal 连接
- 找到 NUS Service（`6E400001-...`），打开 TX Notify + RX Write
- 直接发文本命令，回复从 TX 推送过来

---

## 依赖库

| 库 | 用途 | 阶段 |
|---|---|---|
| M5Cardputer | 硬件初始化、键盘、ES8311 | 全程 |
| ESP32-audioI2S | 音频解码 + I2S 输出 + ID3 回调 | 全程 |
| M5GFX | 屏幕绘图 | 全程 |
| SD / FS | 文件系统 | 全程 |
| arduinoWebSockets | WebSocket 传输层 | Week 3 |
| PubSubClient | MQTT 客户端 | Week 3 |
| ArduinoJson | 解析通知 JSON | Week 3 |
| HTTPClient | 拉取 RSS XML（ESP32 内置） | Week 3 |
| TJpgDec | 封面 JPEG 解码 | Week 5 |
| Preferences | NVS 断电续播 | Week 5 |
| Wire | I2C 写 ES8311 寄存器 | Week 1 |
