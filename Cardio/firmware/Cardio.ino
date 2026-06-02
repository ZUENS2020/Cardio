// Cardio — M5Stack Cardputer ADV firmware
// Entry point: setup() + loop()

#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

#include "Logger.h"
#include "DebugConsole.h"
#include "Config.h"
#include "AudioEngine.h"
#include "JackMonitor.h"
#include "PlaybackController.h"
#include "PlayerScreen.h"
#include "BrowserScreen.h"
#include "SplashScreen.h"
#include "Theme.h"

static constexpr int SD_SCK  = 40;
static constexpr int SD_MISO = 39;
static constexpr int SD_MOSI = 14;
static constexpr int SD_CS   = 12;

enum ScreenMode { MODE_PLAYER, MODE_BROWSER };
static ScreenMode mode = MODE_PLAYER;

static void handlePlayerKeys();
static void refreshPlayerUI();
static void syncPlayerUI();
static void handleBrowserKeys();

// Hold-to-repeat timing, shared by the volume keys and list navigation so they
// feel identical: first press steps once instantly, then nothing until
// REPEAT_DELAY_MS (a normal tap = one step), then auto-repeat every
// REPEAT_RATE_MS while held.
static constexpr uint32_t REPEAT_DELAY_MS = 400;
static constexpr uint32_t REPEAT_RATE_MS  = 90;

static bool mountSD() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI, 25000000)) {
        Serial.println("[BOOT] SD mount FAILED");
        return false;
    }
    Serial.printf("[BOOT] SD mounted %lluMB\n", SD.cardSize() / (1024ULL * 1024ULL));
    return true;
}

// ── setup ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && millis() - t0 < 2000) delay(10);

    auto m5cfg = M5.config();
    M5Cardputer.begin(m5cfg);

    bool sdOk = mountSD();

    Logger::instance().begin(sdOk, Logger::INFO);
    LOG_I("BOOT", "Cardio starting, sd=%s", sdOk ? "ok" : "none");

    Config& cfg = Config::instance();
    cfg.begin();
    Logger::instance().setLevel(cfg.logLevel());
    Logger::instance().setSdEnabled(cfg.debugEnabled() && sdOk);
    cfg.registerConsole();

    // Boot splash — code-drawn, no SD/GIF needed; blocks ~1.5s.
    SplashScreen::instance().play();

    AudioEngine& ae = AudioEngine::instance();
    ae.setVolume(cfg.defaultVolume());
    ae.setMuted(cfg.muted());        // boot silent if config says so (classroom)
    ae.begin();
    ae.registerConsole();

    JackMonitor& jack = JackMonitor::instance();
    jack.onInsert([]() { AudioEngine::instance().pause();  });
    jack.onRemove([]() { AudioEngine::instance().resume(); });
    jack.begin();
    jack.registerConsole();

    PlaybackController& pc = PlaybackController::instance();
    pc.begin();
    pc.registerConsole();

    PlayerScreen::instance().begin();
    PlayerScreen::instance().setVolume(cfg.defaultVolume());
    BrowserScreen::instance().begin();

    DebugConsole::instance().begin();
    LOG_I("BOOT", "boot complete, heap=%u", esp_get_free_heap_size());
}

// ── loop ──────────────────────────────────────────────────────────────────────

void loop() {
    M5Cardputer.update();
    JackMonitor::instance().update();

    // Pump audio decoder and auto-advance on track end
    PlaybackController::instance().update();

    if (mode == MODE_PLAYER) {
        handlePlayerKeys();
        refreshPlayerUI();
    } else {
        handleBrowserKeys();
        BrowserScreen::instance().draw();
    }

    DebugConsole::instance().update();
    delay(AudioEngine::instance().isPlaying() ? 1 : 10);
}

// ── Player mode ───────────────────────────────────────────────────────────────

static void handlePlayerKeys() {
    auto& kb = M5Cardputer.Keyboard;
    auto& ae = AudioEngine::instance();
    auto& pc = PlaybackController::instance();

    // ── Volume: hold-to-repeat with instant feedback ───────────────────────
    // Checked every loop (not just on the isChange() edge), so holding
    // ';' / '.' ramps the volume smoothly instead of one step per tap.
    static uint32_t nextVolMs = 0;
    bool volUp = kb.isKeyPressed(';');
    bool volDn = kb.isKeyPressed('.');
    if (volUp != volDn) {                       // exactly one of them held
        uint32_t now = millis();
        if (now >= nextVolMs) {
            uint8_t v = ae.volume();
            if      (volUp && v < 21) ae.setVolume(v + 1);
            else if (volDn && v > 0)  ae.setVolume(v - 1);
            syncPlayerUI();                     // redraw now, skip the throttle
            // long initial delay before auto-repeat, then fast (typematic feel)
            nextVolMs = now + (nextVolMs ? REPEAT_RATE_MS : REPEAT_DELAY_MS);
        }
    } else {
        nextVolMs = 0;                          // released → next tap is instant
    }

    // ── Edge-triggered keys (one action per physical press) ─────────────────
    if (!kb.isChange() || !kb.isPressed()) return;
    Keyboard_Class::KeysState ks = kb.keysState();

    if (ks.enter) {
        BrowserScreen::instance().openFolders();
        mode = MODE_BROWSER;
        return;                                 // leaving player mode
    }

    bool acted = false;
    for (char c : ks.word) {
        switch (c) {
        case ' ':
            if      (ae.isPlaying() && !ae.isPaused()) ae.pause();
            else if (ae.isPaused())                    ae.resume();
            else                                       pc.play();
            acted = true;
            break;
        case ',': pc.prev();        acted = true; break;
        case '/': pc.next();        acted = true; break;
        case 'r': case 'R': pc.cycleOrder();              acted = true; break;
        case 'm': case 'M': ae.setMuted(!ae.isMuted());   acted = true; break;
        }
    }
    if (acted) syncPlayerUI();                   // instant feedback, no throttle
}

// Push all current audio/playback state into the PlayerScreen, then redraw.
// Cheap when nothing moved: PlayerScreen::update() is gated by its own dirty
// flag, so it only repaints the sprite when a value actually changed.
static void syncPlayerUI() {
    auto& ae = AudioEngine::instance();
    auto& pc = PlaybackController::instance();
    auto& ui = PlayerScreen::instance();

    ui.setTrackTitle(ae.currentTitle());
    ui.setArtist(ae.artist());
    ui.setAlbum(ae.album());
    ui.setProgress(ae.positionMs(), ae.durationMs());
    ui.setPlaying(ae.isPlaying());
    ui.setVolume(ae.volume());
    ui.setMuted(ae.isMuted());
    ui.setOrder((uint8_t)pc.order());

    const char* path = ae.currentPath();
    if (path && path[0]) {
        const char* dot = strrchr(path, '.');
        if (dot) {
            char fmt[8];
            snprintf(fmt, sizeof(fmt), "%s", dot + 1);
            if (fmt[0] >= 'a') fmt[0] -= 32; // uppercase
            ui.setFormat(fmt);
        }
    } else {
        ui.setFormat("");
    }
    ui.update();
}

// Periodic refresh for the progress bar.  Throttled because setProgress()
// advances every loop while playing and would otherwise repaint constantly;
// key handlers call syncPlayerUI() directly for instant feedback.
static void refreshPlayerUI() {
    static uint8_t tick = 0;
    if (++tick < 16) return;
    tick = 0;
    syncPlayerUI();
}

// ── Browser mode ──────────────────────────────────────────────────────────────

static void handleBrowserKeys() {
    auto& kb = M5Cardputer.Keyboard;
    auto& bs = BrowserScreen::instance();

    // ── Navigation: hold-to-repeat, same feel as the volume keys ───────────
    // ; / , = up, . / / = down.  Checked every loop (not just on the isChange
    // edge) so holding a key scrolls the list instead of one step per tap.
    static uint32_t nextNavMs = 0;
    bool up   = kb.isKeyPressed(';') || kb.isKeyPressed(',');
    bool down = kb.isKeyPressed('.') || kb.isKeyPressed('/');
    if (up != down) {                       // exactly one direction held
        uint32_t now = millis();
        if (now >= nextNavMs) {
            bs.navigate(up ? -1 : +1);
            // long initial delay before auto-repeat, then fast (typematic feel)
            nextNavMs = now + (nextNavMs ? REPEAT_RATE_MS : REPEAT_DELAY_MS);
        }
    } else {
        nextNavMs = 0;                      // released → next tap is instant
    }

    // ── Edge-triggered: enter (open/play) / del (back/exit) ────────────────
    if (!kb.isChange() || !kb.isPressed()) return;
    Keyboard_Class::KeysState ks = kb.keysState();

    if (ks.del) {
        if (bs.mode() == BROWSER_TRACK) bs.goBack();
        // Shared framebuffer: force the player to repaint over browser content.
        else { mode = MODE_PLAYER; PlayerScreen::instance().markDirty(); }
    }

    if (ks.enter) {
        bs.selectCurrent();
        if (bs.mode() == BROWSER_TRACK && bs.selectedTrack() >= 0) {
            mode = MODE_PLAYER;
            PlayerScreen::instance().markDirty();
        }
    }
}
