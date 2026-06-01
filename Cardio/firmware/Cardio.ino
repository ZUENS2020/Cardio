// Cardio — M5Stack Cardputer ADV 固件主入口
//
// Week 1 Day 1: Logger + DebugConsole
// Week 1 Day 2: AudioEngine（M5Cardputer.begin → PSRAM + ES8311 + 音频解码）

#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

#include "Logger.h"
#include "DebugConsole.h"
#include "Config.h"
#include "AudioEngine.h"
#include "JackMonitor.h"
#include "PlayerScreen.h"

// ── SD 卡 SPI 引脚（ADV 官方 docs 确认）────────────────────────────────────
static constexpr int SD_SCK  = 40;
static constexpr int SD_MISO = 39;
static constexpr int SD_MOSI = 14;
static constexpr int SD_CS   = 12;

static bool mountSD() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI, 25000000)) {
        Serial.println("[BOOT] SD mount FAILED");
        return false;
    }
    Serial.printf("[BOOT] SD mounted, size=%lluMB\n",
                  SD.cardSize() / (1024ULL * 1024ULL));
    return true;
}

void setup() {
    // ── 1. Serial 最先启动，这样 M5 初始化过程中也能看到诊断输出 ──────────
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 2000) { delay(10); }

    Serial.println();
    Serial.println("[BOOT] Cardio starting...");

    // ── 2. M5Cardputer 硬件初始化（LCD / ES8311 / TCA8418 / OPI PSRAM）────
    auto m5cfg = M5.config();
    M5Cardputer.begin(m5cfg);
    Serial.println("[BOOT] M5Cardputer initialized");

    // ── 3. SD 卡 ────────────────────────────────────────────────────────────
    bool sdOk = mountSD();

    // ── 4. Logger ───────────────────────────────────────────────────────────
    Logger::instance().begin(sdOk, Logger::INFO);
    LOG_I("BOOT", "Cardio firmware boot, sd=%s", sdOk ? "ok" : "none");

    // ── 5. Config ───────────────────────────────────────────────────────────
    Config& cfg = Config::instance();
    cfg.begin();
    Logger::instance().setLevel(cfg.logLevel());
    Logger::instance().setSdEnabled(cfg.debugEnabled() && sdOk);
    cfg.registerConsole();

    // ── 6. AudioEngine ──────────────────────────────────────────────────────
    AudioEngine& audio = AudioEngine::instance();
    audio.setVolume(cfg.defaultVolume());
    audio.begin();
    audio.registerConsole();

    // ── 7. JackMonitor ──────────────────────────────────────────────────────
    JackMonitor& jack = JackMonitor::instance();
    jack.onInsert([]() { AudioEngine::instance().pause(); });
    jack.onRemove([]() { AudioEngine::instance().resume(); });
    jack.begin();
    jack.registerConsole();

    // ── 8. PlayerScreen ─────────────────────────────────────────────────────
    PlayerScreen::instance().begin();
    PlayerScreen::instance().setVolume(cfg.defaultVolume());

    // ── 9. DebugConsole 最后启动（打印欢迎语 + help）───────────────────────
    DebugConsole::instance().begin();
}

void loop() {
    M5Cardputer.update();                 // 键盘 / 按钮轮询
    AudioEngine::instance().update();     // 解码器驱动（必须高频调用）
    JackMonitor::instance().update();     // 耳机插拔检测

    // UI 每 16 次 loop 更新一次（约 16×2ms = 32ms ≈ 30fps），
    // 不影响 AudioEngine::update() 的调用频率
    static uint8_t uiTick = 0;
    if (++uiTick >= 16) {
        uiTick = 0;
        auto& audio = AudioEngine::instance();
        auto& ui    = PlayerScreen::instance();
        ui.setTrackTitle(audio.currentTitle());
        ui.setArtist(audio.artist());
        ui.setAlbum(audio.album());
        ui.setProgress(audio.positionMs(), audio.durationMs());
        ui.setPlaying(audio.isPlaying());
        ui.setVolume(audio.volume());
        ui.update();
    }

    DebugConsole::instance().update();    // 串口命令
    delay(2);  // 解码器需要高频调用，保持 2ms delay
}
