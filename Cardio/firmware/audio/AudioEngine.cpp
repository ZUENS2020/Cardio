#include "AudioEngine.h"
#include "AudioOutputM5Speaker.h"
#include <M5Cardputer.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorFLAC.h>
#include <AudioGeneratorWAV.h>
#include <SD.h>
#include "Logger.h"
#include "DebugConsole.h"

// ── Pimpl ──────────────────────────────────────────────────────────────────
struct AudioEngine::Impl {
    AudioOutputM5Speaker* out  = nullptr;
    AudioFileSourceSD*    src  = nullptr;
    AudioFileSourceID3*   id3  = nullptr;
    AudioGenerator*       gen  = nullptr;
    char path[256]        = {};
    char trackTitle[128]  = {};
    char artist[128]      = {};
    char album[128]       = {};
    uint32_t id3DurMs     = 0;

    void cleanup() {
        if (gen) { gen->stop(); delete gen; gen = nullptr; }
        if (id3) { delete id3;              id3 = nullptr; }
        if (src) { delete src;              src = nullptr; }
        path[0] = trackTitle[0] = artist[0] = album[0] = '\0';
        id3DurMs = 0;
    }

    // ID3 元数据回调（定义在 Impl 内部，可访问私有成员）
    static void metadataCB(void* cbData, const char* type, bool /*isUnicode*/, const char* str) {
        auto* impl = (Impl*)cbData;
        if      (strcmp(type, "TIT2") == 0) strlcpy(impl->trackTitle, str, sizeof(impl->trackTitle));
        else if (strcmp(type, "TPE1") == 0) strlcpy(impl->artist,     str, sizeof(impl->artist));
        else if (strcmp(type, "TALB") == 0) strlcpy(impl->album,      str, sizeof(impl->album));
        else if (strcmp(type, "TLEN") == 0) impl->id3DurMs = (uint32_t)atol(str);
    }
};

// ── Singleton ──────────────────────────────────────────────────────────────
AudioEngine& AudioEngine::instance() {
    static AudioEngine inst;
    return inst;
}

// ── Core 0 音频任务 ────────────────────────────────────────────────────────
void AudioEngine::audioTask(void* arg) {
    auto* eng = static_cast<AudioEngine*>(arg);
    for (;;) {
        eng->update();
        vTaskDelay(1);  // 1ms yield，让 Core 0 上的 WiFi/BT 任务也能运行
    }
}

// ── begin ──────────────────────────────────────────────────────────────────
void AudioEngine::begin() {
    _mutex = xSemaphoreCreateMutex();

    auto spk_cfg = M5Cardputer.Speaker.config();
    spk_cfg.sample_rate = 96000;
    spk_cfg.stereo = true;
    M5Cardputer.Speaker.config(spk_cfg);
    M5Cardputer.Speaker.begin();

    _impl = new Impl();
    _impl->out = new AudioOutputM5Speaker(0);
    _impl->out->begin();
    setVolume(_volume);

    // 解码任务固定在 Core 0（UI + 控制台跑 Core 1）
    xTaskCreatePinnedToCore(
        audioTask,
        "audioTask",
        12288,   // 12KB stack：FLAC 解码需要较大栈
        this,
        5,       // 优先级 5（高于 loop 的默认 1，低于系统任务的 24）
        &_taskHandle,
        0        // Core 0
    );

    LOG_I("AUDIO", "AudioEngine ready, vol=%u, stereo, task=Core0", _volume);
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

    // 预先从第一帧帧头估算时长（只在文件刚打开、未被解码器读取时 seek）
    if (strcasecmp(ext, "mp3") == 0) {
        static const uint16_t kbpsTable[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
        uint8_t buf[3] = {};
        _impl->src->seek(0, SEEK_SET);
        _impl->src->read(buf, 3);
        if (buf[0]=='I' && buf[1]=='D' && buf[2]=='3') {
            uint8_t h[7]; _impl->src->read(h, 7);
            uint32_t skip = 10u + (((uint32_t)(h[3]&0x7f)<<21)|((uint32_t)(h[4]&0x7f)<<14)
                                  |((uint32_t)(h[5]&0x7f)<<7)|(h[6]&0x7f));
            _impl->src->seek(skip, SEEK_SET);
        } else { _impl->src->seek(0, SEEK_SET); }
        for (int i = 0; i < 4096; i++) {
            uint8_t b; if (!_impl->src->read(&b, 1)) break;
            if (b != 0xFF) continue;
            uint8_t b2; if (!_impl->src->read(&b2, 1)) break;
            if ((b2 & 0xE0) != 0xE0) continue;
            uint8_t b3; _impl->src->read(&b3, 1);
            int idx = (b3 >> 4) & 0x0F;
            if (idx > 0 && idx < 15 && kbpsTable[idx] > 0) {
                uint32_t sz = _impl->src->getSize();
                _impl->id3DurMs = (uint32_t)((uint64_t)sz * 8 / kbpsTable[idx]);
                break;
            }
        }
        _impl->src->seek(0, SEEK_SET);  // 重置给 ID3 包装器
    }

    AudioFileSource* source = _impl->src;

    if (strcasecmp(ext, "mp3") == 0) {
        _impl->id3 = new AudioFileSourceID3(_impl->src);
        _impl->id3->RegisterMetadataCB(Impl::metadataCB, _impl);
        _impl->id3->open(path);
        source = _impl->id3;
        _impl->gen = new AudioGeneratorMP3();
    } else if (strcasecmp(ext, "flac") == 0) {
        _impl->gen = new AudioGeneratorFLAC();
    } else if (strcasecmp(ext, "wav") == 0) {
        _impl->gen = new AudioGeneratorWAV();
    } else {
        LOG_E("AUDIO", "unsupported format: %s", ext);
        _impl->cleanup();
        return false;
    }

    if (!_impl->gen->begin(source, _impl->out)) {
        LOG_E("AUDIO", "generator begin failed: %s", path);
        _impl->cleanup();
        return false;
    }

    // 加锁后再挂入 generator（stop() 已经清理过了）
    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    strlcpy(_impl->path, path, sizeof(_impl->path));
    _startMs  = millis();
    _pausedMs = 0;
    if (_mutex) xSemaphoreGive(_mutex);

    LOG_I("AUDIO", "playing: %s", path);
    return true;
}

// ── transport ──────────────────────────────────────────────────────────────
void AudioEngine::pause() {
    if (!_paused) {
        _paused = true;
        _pausedMs = millis();  // 记录暂停开始时间
        LOG_I("AUDIO", "paused");
    }
}

void AudioEngine::resume() {
    if (_paused) {
        _startMs += (millis() - _pausedMs);  // 补偿暂停时长
        _paused = false;
        LOG_I("AUDIO", "resumed");
    }
}

void AudioEngine::stop() {
    if (!_impl) return;
    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    if (_impl->gen && _impl->gen->isRunning()) {
        _impl->out->stop();
    }
    _impl->cleanup();
    _paused = false;
    if (_mutex) xSemaphoreGive(_mutex);
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

// ── update（Core 0 任务调用，mutex 保护）─────────────────────────────────
void AudioEngine::update() {
    if (!_impl || !_impl->gen || _paused) return;
    if (_mutex && xSemaphoreTake(_mutex, 0) != pdTRUE) return;  // 非阻塞取锁
    if (_impl->gen && _impl->gen->isRunning()) {
        if (!_impl->gen->loop()) {
            LOG_I("AUDIO", "playback ended: %s", _impl->path);
            _impl->cleanup();
        }
    }
    if (_mutex) xSemaphoreGive(_mutex);
}

// ── console commands ───────────────────────────────────────────────────────
// ── progress getters ───────────────────────────────────────────────────────
uint32_t AudioEngine::positionMs() const {
    if (!_impl || !_impl->gen || !_impl->gen->isRunning()) return 0;
    if (_paused) return _pausedMs - _startMs;
    return millis() - _startMs;
}

// 检查字符串是否为有效 UTF-8（GBK 编码的 ID3 标签会被拒绝）
static bool isValidUTF8(const char* s) {
    while (*s) {
        uint8_t c = (uint8_t)*s++;
        if (c < 0x80) continue;
        int extra = (c < 0xE0) ? 1 : (c < 0xF0) ? 2 : 3;
        for (int i = 0; i < extra; i++)
            if (((uint8_t)*s++ & 0xC0) != 0x80) return false;
    }
    return true;
}

uint32_t AudioEngine::durationMs() const {
    if (!_impl) return 0;
    if (_impl->id3DurMs > 0) return _impl->id3DurMs;
    // 无 TLEN：从第一帧帧头读取实际码率，再用文件大小估算
    if (_impl->src) {
        uint32_t sz = _impl->src->getSize();
        if (sz == 0) return 0;
        static const uint16_t kbpsTable[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
        uint8_t buf[4] = {};
        _impl->src->seek(0, SEEK_SET);
        // 跳过可能的 ID3 头（"ID3" + 4 字节大小）
        _impl->src->read(buf, 3);
        if (buf[0]=='I' && buf[1]=='D' && buf[2]=='3') {
            uint8_t h[7]; _impl->src->read(h, 7);
            uint32_t skip = 10 + (((uint32_t)(h[3]&0x7f)<<21)|((uint32_t)(h[4]&0x7f)<<14)
                                  |((uint32_t)(h[5]&0x7f)<<7)|(h[6]&0x7f));
            _impl->src->seek(skip, SEEK_SET);
        } else { _impl->src->seek(0, SEEK_SET); }
        // 找 MP3 帧同步字 0xFF 0xEx
        for (int i = 0; i < 4096; i++) {
            uint8_t b; if (!_impl->src->read(&b, 1)) break;
            if (b != 0xFF) continue;
            uint8_t b2; if (!_impl->src->read(&b2, 1)) break;
            if ((b2 & 0xE0) != 0xE0) continue;
            uint8_t b3; _impl->src->read(&b3, 1);
            int idx = (b3 >> 4) & 0x0F;
            if (idx > 0 && idx < 15 && kbpsTable[idx] > 0) {
                _impl->src->seek(0, SEEK_SET);
                return (uint32_t)((uint64_t)sz * 8 / kbpsTable[idx]);
            }
        }
        _impl->src->seek(0, SEEK_SET);
        return (uint32_t)((uint64_t)sz * 8 / 128);  // 最终回退
    }
    return 0;
}

const char* AudioEngine::currentTitle() const {
    if (!_impl) return "";
    // 只在 UTF-8 有效时使用 ID3 标签（拒绝 GBK 编码的标签）
    if (_impl->trackTitle[0] && isValidUTF8(_impl->trackTitle))
        return _impl->trackTitle;
    // 回退：文件名（FAT32 路径始终是 UTF-8）
    if (!_impl->path[0]) return "";
    const char* slash = strrchr(_impl->path, '/');
    return slash ? slash + 1 : _impl->path;
}

const char* AudioEngine::artist() const {
    if (!_impl || !_impl->artist[0]) return "";
    return isValidUTF8(_impl->artist) ? _impl->artist : "";
}

const char* AudioEngine::album() const {
    if (!_impl || !_impl->album[0]) return "";
    return isValidUTF8(_impl->album) ? _impl->album : "";
}

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
