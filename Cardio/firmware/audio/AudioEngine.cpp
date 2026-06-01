#include "AudioEngine.h"
#include "AudioOutputM5Speaker.h"
#include <M5Cardputer.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorFLAC.h>
#include <AudioGeneratorWAV.h>
#include <SD.h>
#include "Logger.h"
#include "DebugConsole.h"

// ── Pimpl ──────────────────────────────────────────────────────────────────
struct AudioEngine::Impl {
    AudioOutputM5Speaker* out = nullptr;
    AudioFileSourceSD*    src = nullptr;
    AudioGenerator*       gen = nullptr;
    char path[256] = {};

    void cleanup() {
        if (gen) { gen->stop(); delete gen; gen = nullptr; }
        if (src) { delete src;              src = nullptr; }
        path[0] = '\0';
    }
};

// ── Singleton ──────────────────────────────────────────────────────────────
AudioEngine& AudioEngine::instance() {
    static AudioEngine inst;
    return inst;
}

// ── begin ──────────────────────────────────────────────────────────────────
void AudioEngine::begin() {
    // 提高 Speaker 内部 PWM 采样率：96kHz 比默认 64kHz 音质明显更好
    auto spk_cfg = M5Cardputer.Speaker.config();
    spk_cfg.sample_rate = 96000;
    M5Cardputer.Speaker.config(spk_cfg);
    M5Cardputer.Speaker.begin();

    _impl = new Impl();
    _impl->out = new AudioOutputM5Speaker(0);
    _impl->out->begin();
    setVolume(_volume);
    LOG_I("AUDIO", "AudioEngine ready, vol=%u, spk_rate=96kHz", _volume);
}

// ── play ───────────────────────────────────────────────────────────────────
bool AudioEngine::play(const char* path) {
    if (!_impl) return false;

    stop();           // 先停当前播放
    _paused = false;

    const char* ext = strrchr(path, '.');
    if (!ext) { LOG_E("AUDIO", "no extension: %s", path); return false; }
    ext++;

    if (!SD.exists(path)) { LOG_E("AUDIO", "not found: %s", path); return false; }

    _impl->src = new AudioFileSourceSD(path);

    if      (strcasecmp(ext, "mp3")  == 0) _impl->gen = new AudioGeneratorMP3();
    else if (strcasecmp(ext, "flac") == 0) _impl->gen = new AudioGeneratorFLAC();
    else if (strcasecmp(ext, "wav")  == 0) _impl->gen = new AudioGeneratorWAV();
    else {
        LOG_E("AUDIO", "unsupported format: %s", ext);
        _impl->cleanup();
        return false;
    }

    if (!_impl->gen->begin(_impl->src, _impl->out)) {
        LOG_E("AUDIO", "generator begin failed: %s", path);
        _impl->cleanup();
        return false;
    }

    strlcpy(_impl->path, path, sizeof(_impl->path));
    LOG_I("AUDIO", "playing: %s", path);
    return true;
}

// ── transport ──────────────────────────────────────────────────────────────
void AudioEngine::pause() {
    if (!_paused) { _paused = true; LOG_I("AUDIO", "paused"); }
}

void AudioEngine::resume() {
    if (_paused) { _paused = false; LOG_I("AUDIO", "resumed"); }
}

void AudioEngine::stop() {
    if (!_impl) return;
    if (_impl->gen && _impl->gen->isRunning()) {
        _impl->out->stop();
    }
    _impl->cleanup();
    _paused = false;
}

bool AudioEngine::isPlaying() const {
    return _impl && _impl->gen && _impl->gen->isRunning() && !_paused;
}

// ── volume ─────────────────────────────────────────────────────────────────
void AudioEngine::setVolume(uint8_t vol) {
    _volume = (vol > 21) ? 21 : vol;
    // 0-21 → 0-252（M5.Speaker 接受 0-255）
    M5Cardputer.Speaker.setVolume(_volume * 12);
}

// ── update (loop) ──────────────────────────────────────────────────────────
void AudioEngine::update() {
    if (!_impl || !_impl->gen || _paused) return;
    if (_impl->gen->isRunning()) {
        if (!_impl->gen->loop()) {
            LOG_I("AUDIO", "playback ended: %s", _impl->path);
            _impl->cleanup();
        }
    }
}

// ── console commands ───────────────────────────────────────────────────────
void AudioEngine::registerConsole() {
    auto& con = DebugConsole::instance();

    con.registerCommand("play",
        "play <path>  play file from SD (/Cardio/music/ prefix optional)",
        [](int argc, char** argv, Print& out) {
            if (argc < 2) { out.println("usage: play <path>"); return; }
            // 把 argv[1..] 用空格拼回来，支持含空格的文件名
            char arg[256] = {};
            for (int i = 1; i < argc; i++) {
                if (i > 1) strlcat(arg, " ", sizeof(arg));
                strlcat(arg, argv[i], sizeof(arg));
            }
            char path[256];
            if (arg[0] == '/')
                strlcpy(path, arg, sizeof(path));
            else
                snprintf(path, sizeof(path), "/Cardio/music/%s", arg);
            if (!AudioEngine::instance().play(path))
                out.printf("failed: %s\n", path);
        });

    con.registerCommand("pause",  "pause playback",
        [](int, char**, Print&) { AudioEngine::instance().pause(); });

    con.registerCommand("resume", "resume playback",
        [](int, char**, Print&) { AudioEngine::instance().resume(); });

    con.registerCommand("stop",   "stop playback",
        [](int, char**, Print&) { AudioEngine::instance().stop(); });

    con.registerCommand("vol",
        "vol [0-21]  get/set volume",
        [](int argc, char** argv, Print& out) {
            auto& eng = AudioEngine::instance();
            if (argc >= 2) eng.setVolume((uint8_t)atoi(argv[1]));
            out.printf("volume = %u\n", eng.volume());
        });

    con.registerCommand("status", "playback status",
        [](int, char**, Print& out) {
            auto& eng = AudioEngine::instance();
            out.printf("playing: %s\n", eng.isPlaying() ? "yes" :
                                         eng.isPaused() ? "paused" : "no");
            out.printf("volume:  %u\n", eng.volume());
        });

    con.registerCommand("tone",
        "tone  play 1kHz test tone (verify speaker)",
        [](int, char**, Print& out) {
            M5Cardputer.Speaker.tone(1000, 500);  // 1kHz, 500ms
            out.println("1kHz tone 500ms");
        });
}
