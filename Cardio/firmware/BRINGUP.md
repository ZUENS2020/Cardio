# Cardputer ADV 实机 Bring-up 计划（Day 2-3）

> 设备 2026-06-01 到。本文是拿到实机后的逐步验证清单，**按风险从高到低排序**：
> 先打通最不确定的音频链路，再补软件层。每步都有明确验收标准。

---

## 0. 确认引脚表（来自 M5Stack 官方 docs + AndyAiCardputer 参考仓库）

| 功能 | 信号 | GPIO | 来源 |
|---|---|---|---|
| **microSD (SPI)** | CS | **12** | docs + 参考仓库逐字一致 |
| | MOSI | **14** | |
| | SCK/CLK | **40** | |
| | MISO | **39** | |
| **ES8311 (I2S)** | BCLK / SCLK | **41** | 官方 docs |
| | LRCK / WS | **43** | |
| | DSDIN（播放，ESP→codec） | **42** | |
| | ASDOUT（录音，codec→ESP） | **46** | |
| **ES8311 (I2C 控制)** | SDA | **8** | 地址 `0x18` |
| | SCL | **9** | |
| **LCD** | BL/RST/RS/DAT/SCK/CS | 38/33/34/35/36/37 | 官方 docs |
| **键盘 TCA8418** | SDA/SCL/INT | 8/9/**11** | I2C 地址 `0x34` |
| **电池 ADC** | — | **10** | |
| **IR 发射** | TX | **44** | |

> ⚠️ **I2C 总线 G8/G9 三设备共用**：TCA8418 键盘(0x34) + ES8311(0x18) + BMI270 IMU(0x68)。
> 初始化 I2C 用同步模式，避免 TCA8418 寄存器 init 期间驱动队列溢出。
> **GPIO 8/9/11 是键盘关键脚，禁止挪用。**

> ✅ Day1 `Cardio.ino` 里的 SD 引脚（12/14/40/39）经此确认**正确**，无需修改。

### 待实测的未知项
- **MCLK**：官方 docs 未列 ES8311 MCLK 脚。ES8311 可用内部时钟/无 MCLK 模式，M5Unified 已处理 → 走 M5.Speaker 即可绕过。
- **耳机检测 / 功放(NS4150B)使能**：docs **无独立 GPIO**，疑似走 ES8311 寄存器或 TCA8418 → 直接影响 JackMonitor，见第 3 步。

---

## 1. 冒烟测试（先证明今天的固件在实机上活着）

```bash
cd Cardio/firmware
pio run --target upload && pio device monitor --baud 115200
```

**验收：**
- [ ] 串口打印 `[BOOT] Cardio starting...` 和 `=== Cardio Debug Console ===`
- [ ] `[BOOT] SD mounted, size=...MB`（若失败说明 SD 卡/引脚问题，优先排查）
- [ ] 控制台输入 `heap` 有输出，`PSRAM total≈8MB`（验证 `qio_opi` 生效）
- [ ] `log level debug` / `log dump` 正常，SD 卡 `/Cardio/logs/cardio.log` 有内容

---

## 2. AudioEngine（★最高风险，最先做）

### 方案决策：不要让 ESP32-audioI2S 直接占 I2S

参考 [AndyAiCardputer/mp3-player-winamp-cardputer-adv](https://github.com/AndyAiCardputer/mp3-player-winamp-cardputer-adv) —— ADV 上**已验证可用**的播放器，结论：

| 路线 | 问题 |
|---|---|
| ESP32-audioI2S 直接驱动 I2S | ① 不会配 ES8311 I2C 寄存器（ADV 上不出声）② 新版依赖 IDF5 `i2s_std.h`，本项目 Arduino core 2.0.17 是 IDF4.4，编译不过（注册表解析失败也与此有关） |
| **✅ M5.Speaker 拥有 ES8311+I2S，解码库只解码** | M5Unified 已封装 ES8311 init，稳。解码器通过 `AudioOutput` 适配类把 PCM 喂给 `M5Cardputer.Speaker.playRaw()` |

### 推荐架构

```
AudioFileSourceSD(file) → AudioGeneratorMP3/FLAC/... → AudioOutputM5Speaker → M5.Speaker.playRaw()
                            (ESP8266Audio 1.9.7 解码)      (桥接适配类)         (M5Unified 驱动 ES8311)
```

桥接类核心（参考仓库 `AudioOutputM5CardputerSpeaker`，三缓冲 3×1536 样本）：
```cpp
class AudioOutputM5Speaker : public AudioOutput {
  bool ConsumeSample(int16_t s[2]) override;  // 累积到缓冲，满了 flush
  void flush() override;                        // _m5sound->playRaw(buf, n, hertz, stereo)
};
```
> 参考实现把立体声降混成单声道、`while(isPlaying())` 忙等，较粗糙。我们可改进为
> 立体声 + 双缓冲乒乓，避免 underrun。先照搬跑通，再优化。

### ⚠️ 库版本雷区
- **ESP8266Audio 必须 1.9.7**（earlephilhower），2.x 需要 IDF5 的 `i2s_std.h`，本项目编译不过。
- platformio.ini 加：`earlephilhower/ESP8266Audio@1.9.7`（替换原计划的 ESP32-audioI2S）。

### ⚠️ 格式覆盖代价（需实测确认）
原 PLAN 承诺 MP3/FLAC/WAV/AAC/M4A/OGG/Opus（基于 ESP32-audioI2S 全内置）。换 ESP8266Audio 后：
- ✅ 有 generator：MP3 / AAC(raw) / FLAC / WAV / Opus / MIDI / MOD / RTTTL
- ❌ 大概率丢失：**OGG Vorbis**（无 generator）、**M4A**（无 MP4 容器解复用）
- → 明天确认后，需回写 PLAN/CLAUDE.md 的"支持格式"和"依赖库"两处。

**验收：**
- [x] M5.Speaker.begin() 后 `tone()` 能出声（证明 ES8311+功放通路活着）
- [x] SD 放一首 MP3，桥接类跑通，**出声音质正常**（三缓冲 + 96kHz PWM）
- [ ] 再测一首 FLAC（待验）
- [ ] `heap` 看 PSRAM 占用（PSRAM 目前=0，待 M5.begin() 后复查）

**关键结论：**
- `AudioOutputM5Speaker` 必须用三缓冲（tri_buffer[3]）+ ConsumeSample 满时返回 false
- Speaker PWM 采样率设 96kHz（默认 64kHz 音质差）
- 不能在 flushBuf 里 while(isPlaying()) 等待，会造成缺口导致破音

---

## 3. JackMonitor（受未知项影响，方案待实测定）

原计划：GPIO 边沿中断检测 3.5mm 插拔 → 立即暂停。
**但 ADV docs 无独立 jack-detect GPIO。** 明天先确认检测源：

1. 用 `i2cdetect` 思路扫总线，读 ES8311 / TCA8418 寄存器，看插拔时哪位变化
2. 若 ES8311 有 GPIO/检测寄存器 → 轮询该寄存器（放弃硬中断）
3. 若都没有 → 降级：本期不做自动暂停，UI 留手动切换（记入后续迭代）

**验收（视检测源而定）：**
- [ ] 找到插拔可观测的寄存器位 / 引脚
- [ ] 插入耳机 → 播放暂停；拔出 → 可恢复

---

## 4. Config（✅ 已写完，明天仅需实机验证）

代码已完成并 `pio run` 通过（commit `0e52a68`）。明天实机确认：
- [ ] SD 卡放好 `config.txt`，启动日志出现 `[CFG] loaded N keys: ...`
- [ ] `config get default_volume` 返回卡上的值
- [ ] `config set log_level debug` → `config save` → `reboot` 后仍生效（验证回写）
- [ ] 改 `debug_enabled=true` 后 SD 出现 `/Cardio/logs/cardio.log`

---

## 当天物料准备清单

- [ ] microSD 卡（FAT32），根目录建 `/Cardio/music/test/`，放 1 首 MP3 + 1 首 FLAC（48kHz 优先）
- [ ] 复制 `sdcard/Cardio/` 模板到卡（config.txt 等）
- [ ] 3.5mm 有线耳机（测 jack 检测）
- [ ] USB-C 数据线（非纯充电线）

---

## 当天结束时应回写的文档
- PLAN.md：勾选 Day2-3，更新"支持格式"
- CLAUDE.md：依赖库表 ESP32-audioI2S → ESP8266Audio 1.9.7；音频约束（M5.Speaker 拥有 I2S）
- 本文：把"待实测未知项"的结论补全
