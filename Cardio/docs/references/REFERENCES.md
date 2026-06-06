# Cardio — 参考资料库

与本项目相关的数据手册、原理图、库与文档的**离线资料库 + 索引**。

> 📦 **关于 PDF**：`datasheets/` 和 `schematics/` 下的 PDF 是厂商版权文件，**本地保留但不入 Git**
> （见根 `.gitignore`，避免撑大仓库与公开再分发）。要在新机器上重建，运行
> [`download.sh`](download.sh) 一键拉取（链接见下表）。原理图 `WM8960_Audio_Board_Schematic.pdf`
> 来自 Waveshare wiki，对应实际选用的 **WM8960 Audio Board 小板**（非树莓派 HAT）。

---

## 📄 数据手册（`datasheets/`）

| 文件 | 是什么 | 对应项目部分 | 来源 |
|---|---|---|---|
| `ESP32-S3_datasheet.pdf` | 主控芯片数据手册 | 全局（MCU）| [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf) |
| `ESP32-S3_TRM.pdf` | 技术参考手册（I2S/外设寄存器，~15MB）| 音频 I2S、外设 | [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf) |
| `TCA8418_keyboard_controller.pdf` | I2C 键盘矩阵控制器 | 键盘输入（ADV 专有）| [TI](https://www.ti.com/lit/ds/symlink/tca8418.pdf) |
| `ES8311_codec.pdf` | 内置**单声道** codec | 内部音频路径 | [Everest](http://www.everest-semi.com/pdf/ES8311%20PB.pdf) |
| `WM8960_codec.pdf` | 立体声 codec（外接方案-备选）| [PLAN_WM8960](../../PLAN_WM8960.md) | Cirrus/Wolfson（[NXP 镜像](https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/imx-processors/52419/1/WM8960.pdf)）|
| `PCM5102A_datasheet.pdf` | 立体声 I2S **DAC**（外接方案-**当前选用**，无 I2C）| [pcm5102-dac](../hardware/pcm5102-dac/README.md) | [TI](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf) |
| *(链接only)* NS4150B | 内置单声道 D 类功放 | 内部音频路径 | [datasheet4u](https://datasheet4u.com/datasheets/Nsiway/NS4150B/1544751)（自动下载被挡，手动下）|

## 📐 原理图（`schematics/`）

| 文件 | 是什么 | 来源 |
|---|---|---|
| `Sch_M5CardputerAdv_v1.0.pdf` | **Cardputer ADV 主板原理图**（含 ES8311/NS4150B/键盘/Grove/EXT 接线）| [M5 OSS](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1178/Sch_M5CardputerAdv_v1.0_2025_06_20_17_19_58.pdf) |
| `Sch_StampS3_v0.3.3.pdf` | StampS3 核心模组原理图（ADV 用 StampS3A，此为最接近的公开版）| [M5 OSS](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1150/Sch_StampS3_v0.3.3.pdf) |
| `M5_CardputerAdv_K132_overview.pdf` | ADV 产品概览/规格（K132）| [M5 OSS](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1178/K132-Adv-cardputer-ADV.pdf) |
| `WM8960_Audio_Board_Schematic.pdf` | **WM8960 Audio Board 小板**原理图（24MHz 有源晶振、2×8 排针、无插孔检测、3.3V）| [Waveshare wiki](https://www.waveshare.com/wiki/File:WM8960_Audio_Board_Schematic..pdf) |

> 关键结论已写进 [PLAN_WM8960.md](../../PLAN_WM8960.md)（板载 24MHz 自供 MCLK → 无需 ESP32 给 MCLK；
> 仅 3.3V 供电；无插孔检测）。

---

## 🔗 软件库（GitHub，按需引用，不下载）

| 库 | 用途 | 项目部分 |
|---|---|---|
| [earlephilhower/ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) | 音频解码（MP3/FLAC/WAV）**必须 1.9.7** | AudioEngine |
| [m5stack/M5Cardputer](https://github.com/m5stack/M5Cardputer) | 硬件/键盘/扬声器封装 | 全局 |
| [m5stack/M5Unified](https://github.com/m5stack/M5Unified) | M5 统一 HAL（ES8311/I2S/电源）| 音频、电源 |
| [m5stack/M5GFX](https://github.com/m5stack/M5GFX) | 屏幕绘图（Sprite）| UI |
| [sparkfun/SparkFun_WM8960_Arduino_Library](https://github.com/sparkfun/SparkFun_WM8960_Arduino_Library) | **WM8960 I2C 驱动**（移植参考）| 外接立体声（计划）|
| [h2zero/NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) | BLE GATT Server | BLE 通知（计划）|
| [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) | 通知 JSON 解析 | BLE 通知（计划）|
| [bmorcelli/Launcher](https://github.com/bmorcelli/Launcher) | 多固件启动器（兼容目标）| Launcher 兼容 |

## 📖 官方文档 / Wiki

- [Cardputer-Adv 官方文档](https://docs.m5stack.com/en/core/Cardputer-Adv) — 规格、引脚图、Grove/EXT 定义
- [Waveshare **WM8960 Audio Board** wiki](https://www.waveshare.com/wiki/WM8960_Audio_Board) — ⚠️ **Audio Board 版**（非 HAT），引脚表 + 示例代码
- [Waveshare WM8960 Audio Board 商品页](https://www.waveshare.com/wm8960-audio-board.htm)

## 📐 技术参考

- [RBJ Audio EQ Cookbook](https://www.w3.org/TR/audio-eq-cookbook/) — 双二阶 peaking 滤波系数公式（`audio/Equalizer.cpp` 用的就是它）
- [WM8960-Audio-HAT issue #9](https://github.com/waveshareteam/WM8960-Audio-HAT/issues/9) — 佐证"插孔检测未接"（上下文，非 HAT 选型）

---

*重建本地库：`bash download.sh`（在本目录运行）。少数被站点拦截的（ES8311 全本、NS4150B）需按上表链接手动下载。*
