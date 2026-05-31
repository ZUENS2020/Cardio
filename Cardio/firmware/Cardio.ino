// Cardio — M5Stack Cardputer ADV 固件主入口
//
// Week 1 Day 1：Logger + DebugConsole（Serial 通道）
// 后续 setup() 会逐步追加 Config / Audio / Player / Net / UI 初始化。

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "Logger.h"
#include "DebugConsole.h"

// ── SD 卡 SPI 引脚 ────────────────────────────────────────────────────────
// TODO(Day2): 用 ADV 实机确认。下列为原版 Cardputer 引脚，ADV 可能不同。
//             挂载失败时 Logger 自动降级为仅 Serial，不影响控制台调试。
static constexpr int SD_SCK  = 40;
static constexpr int SD_MISO = 39;
static constexpr int SD_MOSI = 14;
static constexpr int SD_CS   = 12;

// Day1 暂用编译期默认值，Day2 接入 Config 后改为读 config.txt
static constexpr bool          DEBUG_ENABLED = true;
static constexpr Logger::Level LOG_LEVEL     = Logger::DEBUG;

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

    Logger::instance().begin(DEBUG_ENABLED && sdOk, LOG_LEVEL);
    LOG_I("BOOT", "Cardio firmware boot, sd=%s", sdOk ? "ok" : "none");

    DebugConsole::instance().begin();
}

void loop() {
    DebugConsole::instance().update();
    delay(2);
}
