# SD 卡设置指南

Cardio 的所有运行时文件都放在 SD 卡的 `/Cardio/` 目录下，与其他固件隔离。
本目录（`sdcard/Cardio/`）是**模板**——把它整个复制到卡根目录即可。

## 1. 准备卡

1. 把 SD 卡格式化为 **FAT32**（或 exFAT；推荐 32GB 以内用 FAT32）
2. 把本仓库的 `Cardio/sdcard/Cardio/` 文件夹复制到 **卡根目录**，得到 `/Cardio/...`
3. 放入音乐，编辑 `config.txt`

```
SD 根目录/
└── Cardio/
    ├── music/
    │   └── {播放列表名}/        ← 每个子文件夹 = 一个播放列表
    │       ├── 歌曲1.mp3
    │       └── 歌曲2.flac
    ├── logs/                    ← 自动创建，调试日志
    ├── config.txt              ← 主配置
    ├── rss_feeds.txt           ← RSS 源（规划中）
    └── notify_filter.txt       ← 通知白名单（规划中）
```

## 2. 放音乐

- 在 `/Cardio/music/` 下**每建一个子文件夹就是一个播放列表**；直接放在 `music/` 根下的散文件归入默认列表
- 支持格式：**MP3 / FLAC / WAV**（不支持 WMA / APE / OGG Vorbis / M4A）
- 开机时固件扫描一次建立内存索引；换歌/换列表不再读卡目录

> ⚠️ **macOS 用户**：在 Mac 上拷文件会生成 `._xxx`（AppleDouble）和 `.DS_Store` 隐藏文件。
> 固件会自动跳过点号开头的文件，但建议拷完用 `dot_clean /Volumes/你的卡` 清理，保持干净。

## 3. config.txt 字段

| 键 | 取值 | 状态 | 说明 |
|---|---|---|---|
| `default_volume` | 0–21 | ✅ | 开机音量 |
| `default_order` | `sequential` `shuffle` `repeat_one` `repeat_all` `random` | ✅ | 开机播放顺序 |
| `mute` | `true` `false` | ✅ | 开机静音（教室用，不丢音量值；运行时按 `m` 切换）|
| `eq` | 5 个 dB 值，逗号分隔，如 `4,-2,0,2,3` | ✅ | 均衡器 100/300/1k/3k/8k Hz 增益，范围 ±12（按 `e` 进界面实时调，存盘写回此键）|
| `log_level` | `debug` `info` `warn` `error` | ✅ | 日志级别 |
| `debug_enabled` | `true` `false` | ✅ | 开启后日志写入 SD `/Cardio/logs/cardio.log` |
| `notify_mode` | `off` `ble` `wifi` | 🚧 | 通知模式（BLE/WiFi 通知尚未实现，当前等效 `off`）|
| `rss_enabled` | `true` `false` | 🚧 | RSS 拉取（需 WiFi，未实现）|
| `debug_ble` | `true` `false` | 🚧 | BLE 调试通道（NUS，未实现）|
| `wifi_ssid` / `wifi_pass` | 字符串 | 🚧 | WiFi 凭据（RSS/通知用，未实现）|
| `mqtt_*` | — | 🚧 | 服务端 MQTT（后续迭代）|

> ✅ = 当前生效　🚧 = 已能解析但功能未实现（见 [PLAN_UI_BLE.md](../PLAN_UI_BLE.md)）

也可以不改文件，用串口控制台改：`config set <键> <值>` 然后 `config save`。

## 4. rss_feeds.txt（规划中）

```
# 格式：名称|RSS URL（每行一条，# 开头为注释）
硬核节目|https://feeds.example.com/hardcore.xml
```

## 5. notify_filter.txt（规划中）

```
# 格式：应用名=show|drop（每行一条）
# 未列出的应用默认 show
微信=show
微博=drop
```

---

相关文档：[SD 卡结构总览](../ARCHITECTURE.md#sd-卡目录结构) · [开发计划](../PLAN.md)
