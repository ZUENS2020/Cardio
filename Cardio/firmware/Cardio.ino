// Cardio — M5Stack Cardputer ADV 固件主入口
//
// Week 1 Day 1：Logger + DebugConsole（Serial 通道）
// 后续 setup() 会逐步追加 Config / Audio / Player / Net / UI 初始化。

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "Logger.h"
#include "DebugConsole.h"
#include "Config.h"

// ── SD 卡 SPI 引脚 ────────────────────────────────────────────────────────
// 已核对 ADV 官方 docs 与参考仓库（见 BRINGUP.md），与原版 Cardputer 一致。
// 挂载失败时 Logger 自动降级为仅 Serial，不影响控制台调试。
static constexpr int SD_SCK  = 40;
static constexpr int SD_MISO = 39;
static constexpr int SD_MOSI = 14;
static constexpr int SD_CS   = 12;

static bool mountSD() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    // 首次以较低频率握手，稳定后可提速
    if (!SD.begin(SD_CS, SPI, 25000000)) {
        Serial.println("[BOOT] SD mount FAILED (check pins / card)");
        return false;
    }
    Serial.printf("[BOOT] SD mounted, size=%lluMB\n",
                  SD.cardSize() / (1024ULL * 1024ULL));
    return true;
}

void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 1500) { delay(10); }  // 等 USB CDC 枚举

    Serial.println();
    Serial.println("[BOOT] Cardio starting...");

    bool sdOk = mountSD();

    // Logger 先以 INFO + SD 启动，好让 Config 解析过程能落日志
    Logger::instance().begin(sdOk, Logger::INFO);
    LOG_I("BOOT", "Cardio firmware boot, sd=%s", sdOk ? "ok" : "none");

    // 读 config.txt，再把级别 / SD 写入开关应用到 Logger
    Config& cfg = Config::instance();
    cfg.begin();
    Logger::instance().setLevel(cfg.logLevel());
    Logger::instance().setSdEnabled(cfg.debugEnabled() && sdOk);

    cfg.registerConsole();          // 在 begin() 打印命令列表前注册，使其出现在 help 中
    DebugConsole::instance().begin();
}

void loop() {
    DebugConsole::instance().update();
    delay(2);
}
