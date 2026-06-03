# Cardio 固件

Cardputer ADV 上的 Arduino C++ 固件。完整项目说明见[根目录 README](../../README.md)。

## 构建

用 **PlatformIO**（目录用扁平结构，`src_dir = .`）：

```bash
pio run                            # 主固件（env: cardputer-adv，huge_app 3MB）
pio run --target upload            # 编译并烧录
pio device monitor --baud 115200   # 调试控制台
pio run -e cardputer-adv-launcher  # bmorcelli Launcher 兼容版（app ≤ 1.31MB）
```

依赖（`platformio.ini` 自动拉取）：`m5stack/M5Cardputer`、`earlephilhower/ESP8266Audio@1.9.7`。

> ⚠️ ESP8266Audio **必须 1.9.7**（2.x 需 IDF5）。分区必须 `huge_app.csv`（默认放不下）。
> 本板 **无 PSRAM**，勿配 OPI PSRAM。详见 [CLAUDE.md 关键约束](../../CLAUDE.md)。

## 模块地图

| 目录 | 模块 | 职责 |
|---|---|---|
| `Cardio.ino` | 入口 | setup/loop，三屏模式路由，按键处理，返回 Launcher |
| `audio/` | AudioEngine | 解码（MP3/FLAC/WAV）+ sqrt 音量曲线 + 精确时长 |
| | AudioOutputM5Speaker | 三缓冲零拷贝桥接 M5.Speaker + (L+R)/2 单声道下混 |
| | Equalizer | 5 段 peaking IIR 均衡器（实时，全平旁路）|
| | JackMonitor | 耳机插拔检测框架（ADV 无 jack-detect GPIO，当前停用）|
| `player/` | PlaybackController | 开机扫描 `/Cardio/music/` 建内存库索引 + 5 种播放顺序 |
| `ui/` | PlayerScreen / BrowserScreen / EqScreen / SplashScreen | 屏幕（共用 `Canvas` sprite）|
| | TextRender / Theme | CJK 逐字字体回退 + 配色/尺寸 |
| `config/` | Config | 读写 `/Cardio/config.txt` |
| `debug/` | Logger / DebugConsole | 分级日志（Serial+SD）+ 串口命令控制台 |

> 规划中的 `net/`（WiFi/MQTT/BLE）和 `notify/`（NotifyManager）尚未实现，见
> [PLAN_UI_BLE.md](../PLAN_UI_BLE.md)。

## 调试

串口（115200）连上后输入 `help` 看全部命令。常用：`status` `heap` `vol 12` `eq 0 6` `mute on` `launcher`。
