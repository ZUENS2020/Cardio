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

// ── Impl ─────────────────────────────────────────────────────────────────────

struct AudioEngine::Impl {
    AudioOutputM5Speaker* out = nullptr;
    AudioFileSourceSD*    src = nullptr;
    AudioFileSourceID3*   id3 = nullptr;
    AudioGenerator*       gen = nullptr;

    char     path[256]   = {};
    char     title[128]  = {};
    char     artist[128] = {};
    char     album[128]  = {};
    uint32_t id3DurMs    = 0;
    uint32_t estDurMs    = 0;

    // Tear down only the decode chain; 'out' persists across tracks.
    void teardown() {
        if (gen) { gen->stop(); delete gen; gen = nullptr; }
        if (id3) {              delete id3; id3 = nullptr; }
        if (src) {              delete src; src = nullptr; }
        path[0] = title[0] = artist[0] = album[0] = '\0';
        id3DurMs = estDurMs = 0;
    }

    static void onMeta(void* cbData, const char* type, bool, const char* str) {
        auto* s = static_cast<Impl*>(cbData);
        if      (!strcmp(type, "TIT2")) strlcpy(s->title,  str, sizeof(s->title));
        else if (!strcmp(type, "TPE1")) strlcpy(s->artist, str, sizeof(s->artist));
        else if (!strcmp(type, "TALB")) strlcpy(s->album,  str, sizeof(s->album));
        else if (!strcmp(type, "TLEN")) s->id3DurMs = (uint32_t)atol(str);
    }
};

// ── Singleton ─────────────────────────────────────────────────────────────────

AudioEngine& AudioEngine::instance() {
    static AudioEngine inst;
    return inst;
}

// ── begin ────────────────────────────────────────────────────────────────────

void AudioEngine::begin() {
    _impl = new Impl();
    // Pass raw Speaker pointer — same pattern as BrokenSignal
    _impl->out = new AudioOutputM5Speaker(&M5Cardputer.Speaker, 0);

    // Stereo + custom sample rate: configure once, then begin
    auto spk = M5Cardputer.Speaker.config();
    spk.stereo = true;
    M5Cardputer.Speaker.config(spk);
    M5Cardputer.Speaker.begin();

    setVolume(_volume);
    LOG_I("AUDIO", "ready vol=%u heap=%u", _volume, esp_get_free_heap_size());
}

// Read the exact track length from a FLAC STREAMINFO block.
// Layout after the "fLaC" marker: a 4-byte metadata-block header (the first
// block is always STREAMINFO) then the 34-byte STREAMINFO body.  At body
// offset 10 sits a packed 64-bit field: sample_rate(20) | channels(3) |
// bits_per_sample(5) | total_samples(36) — i.e. file bytes 18..25.
// Returns 0 if the header can't be parsed (caller falls back to an estimate).
static uint32_t flacDurationMs(const char* path) {
    File f = SD.open(path);
    if (!f) return 0;
    uint8_t b[26];
    int n = f.read(b, sizeof(b));
    f.close();
    if (n < 26) return 0;
    if (memcmp(b, "fLaC", 4) != 0) return 0;
    if ((b[4] & 0x7F) != 0)        return 0; // first block must be STREAMINFO

    uint32_t sampleRate   = ((uint32_t)b[18] << 12) | ((uint32_t)b[19] << 4) | (b[20] >> 4);
    uint64_t totalSamples = ((uint64_t)(b[21] & 0x0F) << 32)
                          | ((uint64_t)b[22] << 24)
                          | ((uint32_t)b[23] << 16)
                          | ((uint32_t)b[24] << 8)
                          |  (uint32_t)b[25];
    if (sampleRate == 0 || totalSamples == 0) return 0;
    return (uint32_t)(totalSamples * 1000ULL / sampleRate);
}

// Exact WAV length from the fmt + data chunk headers.
// duration = data_chunk_bytes / byte_rate.  Handles arbitrary chunk ordering
// and the word-alignment padding between chunks.  0 if it can't be parsed.
static uint32_t wavDurationMs(const char* path) {
    File f = SD.open(path);
    if (!f) return 0;
    uint8_t h[12];
    if (f.read(h, 12) != 12 || memcmp(h, "RIFF", 4) != 0 || memcmp(h + 8, "WAVE", 4) != 0) {
        f.close();
        return 0;
    }
    uint32_t byteRate = 0, dataSize = 0;
    uint8_t c[8];
    while (f.read(c, 8) == 8) {
        uint32_t csize = (uint32_t)c[4] | ((uint32_t)c[5] << 8) | ((uint32_t)c[6] << 16) | ((uint32_t)c[7] << 24);
        if (memcmp(c, "fmt ", 4) == 0) {
            uint8_t fmt[16];
            uint32_t want = csize < 16 ? csize : 16;
            if (f.read(fmt, want) != (int)want) break;
            byteRate = (uint32_t)fmt[8] | ((uint32_t)fmt[9] << 8) | ((uint32_t)fmt[10] << 16) | ((uint32_t)fmt[11] << 24);
            uint32_t rest = csize - want + (csize & 1);
            if (rest) f.seek(f.position() + rest);
        } else if (memcmp(c, "data", 4) == 0) {
            dataSize = csize;
            break; // have byteRate (fmt always precedes data) + dataSize
        } else {
            f.seek(f.position() + csize + (csize & 1)); // skip unknown chunk + pad
        }
    }
    f.close();
    if (byteRate == 0 || dataSize == 0) return 0;
    return (uint32_t)((uint64_t)dataSize * 1000ULL / byteRate);
}

// Exact MP3 length.  Skips any ID3v2 tag, parses the first frame header, then:
//   • VBR — reads the Xing/Info frame count → frames * samples_per_frame / rate
//   • CBR — audio_bytes * 8 / bitrate
// Far more accurate than a flat bytes/bitrate guess for VBR files.  0 on failure.
static uint32_t mp3DurationMs(const char* path) {
    File f = SD.open(path);
    if (!f) return 0;
    uint32_t fileSize = f.size();

    uint8_t hdr[10];
    uint32_t pos = 0;
    if (f.read(hdr, 10) == 10 && memcmp(hdr, "ID3", 3) == 0) {
        // ID3v2 size is 4 syncsafe bytes (7 bits each), excluding the 10B header
        pos = 10 + (((uint32_t)(hdr[6] & 0x7F) << 21) | ((uint32_t)(hdr[7] & 0x7F) << 14)
                  | ((uint32_t)(hdr[8] & 0x7F) << 7)  |  (uint32_t)(hdr[9] & 0x7F));
    }

    f.seek(pos);
    uint8_t b[200];
    int n = f.read(b, sizeof(b));
    f.close();
    if (n < 36) return 0;

    // Find the frame sync (11 set bits): 0xFF 0xEx/0xFx
    int o = -1;
    for (int i = 0; i + 4 < n; ++i)
        if (b[i] == 0xFF && (b[i + 1] & 0xE0) == 0xE0) { o = i; break; }
    if (o < 0) return 0;

    int versionBits = (b[o + 1] >> 3) & 0x03; // 0=2.5 2=2 3=1
    int layerBits   = (b[o + 1] >> 1) & 0x03; // 1=Layer III
    int brIndex     = (b[o + 2] >> 4) & 0x0F;
    int srIndex     = (b[o + 2] >> 2) & 0x03;
    int chanMode    = (b[o + 3] >> 6) & 0x03; // 3=mono
    if (layerBits != 1 || versionBits == 1 || brIndex == 0 || brIndex == 15 || srIndex == 3)
        return 0;

    static const int kSr1[]  = {44100, 48000, 32000};
    static const int kSr2[]  = {22050, 24000, 16000};
    static const int kSr25[] = {11025, 12000,  8000};
    static const int kBr1[]  = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320};
    static const int kBr2[]  = {0, 8,16,24,32,40,48,56,64, 80, 96,112,128,144,160};

    bool isV1 = (versionBits == 3);
    int sampleRate = isV1 ? kSr1[srIndex] : (versionBits == 2 ? kSr2[srIndex] : kSr25[srIndex]);
    int bitrate    = (isV1 ? kBr1[brIndex] : kBr2[brIndex]) * 1000; // bps
    int spf        = isV1 ? 1152 : 576;                              // samples per frame

    // Xing/Info VBR header sits after the side-info block
    int sideInfo = isV1 ? (chanMode == 3 ? 17 : 32) : (chanMode == 3 ? 9 : 17);
    int x = o + 4 + sideInfo;
    if (x + 12 <= n && (memcmp(b + x, "Xing", 4) == 0 || memcmp(b + x, "Info", 4) == 0)) {
        uint32_t flags = ((uint32_t)b[x+4] << 24) | ((uint32_t)b[x+5] << 16) | ((uint32_t)b[x+6] << 8) | b[x+7];
        if (flags & 0x0001) {
            uint32_t frames = ((uint32_t)b[x+8] << 24) | ((uint32_t)b[x+9] << 16) | ((uint32_t)b[x+10] << 8) | b[x+11];
            if (frames > 0 && sampleRate > 0)
                return (uint32_t)((uint64_t)frames * spf * 1000ULL / sampleRate);
        }
    }
    if (bitrate > 0) { // CBR
        uint32_t audioBytes = (fileSize > pos) ? (fileSize - pos) : fileSize;
        return (uint32_t)((uint64_t)audioBytes * 8000ULL / bitrate);
    }
    return 0;
}

// ── play ──────────────────────────────────────────────────────────────────────

bool AudioEngine::play(const char* path) {
    if (!_impl) return false;

    // Always stop + flush the output before building a new chain.
    // This is safe even before first playback (flush is a no-op when empty).
    if (_impl->gen && _impl->gen->isRunning()) _impl->gen->stop();
    if (_impl->out) _impl->out->stop();
    _impl->teardown();
    _paused = false;

    const char* dot = strrchr(path, '.');
    if (!dot) { LOG_E("AUDIO", "no ext: %s", path); return false; }
    const char* ext = dot + 1;

    if (!SD.exists(path)) { LOG_E("AUDIO", "not found: %s", path); return false; }

    // Log free heap so we can spot OOM risk before allocating the decoder
    uint32_t freeHeap = esp_get_free_heap_size();
    LOG_I("AUDIO", "play: %s  heap=%u", path, freeHeap);

    // Duration estimate from file size
    _impl->src = new AudioFileSourceSD(path);
    if (!_impl->src) { LOG_E("AUDIO", "SD src alloc failed"); return false; }

    const uint32_t sz = _impl->src->getSize();
    if (sz > 0) {
        uint32_t d = 0;
        if      (strcasecmp(ext, "wav")  == 0) { d = wavDurationMs(path);  _impl->estDurMs = d ? d : (uint32_t)((uint64_t)sz * 1000 / 176400); }
        else if (strcasecmp(ext, "flac") == 0) { d = flacDurationMs(path); _impl->estDurMs = d ? d : (sz / 50); }
        else if (strcasecmp(ext, "mp3")  == 0) { d = mp3DurationMs(path);  _impl->estDurMs = d ? d : (sz / 24); }
        else                                     _impl->estDurMs = sz / 24;
    }

    // No AudioFileSourceBuffer — BrokenSignal reads SD directly for local files.
    // This saves 16 KB of contiguous heap that the MP3 decoder needs.
    AudioFileSource* src = _impl->src;

    if (strcasecmp(ext, "mp3") == 0) {
        _impl->id3 = new AudioFileSourceID3(_impl->src);
        if (!_impl->id3) { _impl->teardown(); return false; }
        _impl->id3->RegisterMetadataCB(Impl::onMeta, _impl);
        src = _impl->id3;
        _impl->gen = new AudioGeneratorMP3();
    } else if (strcasecmp(ext, "flac") == 0) {
        _impl->gen = new AudioGeneratorFLAC();
    } else if (strcasecmp(ext, "wav") == 0) {
        _impl->gen = new AudioGeneratorWAV();
    } else {
        LOG_E("AUDIO", "unsupported: %s", ext);
        _impl->teardown();
        return false;
    }

    if (!_impl->gen) {
        LOG_E("AUDIO", "gen alloc failed (OOM?)  heap=%u", esp_get_free_heap_size());
        _impl->teardown();
        return false;
    }

    // BrokenSignal calls output->begin() explicitly here, before gen->begin()
    _impl->out->begin();

    if (!_impl->gen->begin(src, _impl->out)) {
        LOG_E("AUDIO", "gen begin failed: %s", path);
        _impl->teardown();
        return false;
    }

    strlcpy(_impl->path, path, sizeof(_impl->path));
    _startMs  = millis();
    _pausedMs = 0;
    LOG_I("AUDIO", "started, heap=%u", esp_get_free_heap_size());
    return true;
}

// ── Transport ─────────────────────────────────────────────────────────────────

void AudioEngine::pause() {
    if (!_paused) {
        // Stop the speaker hardware immediately (like BrokenSignal pauseAudio())
        M5Cardputer.Speaker.stop(0);
        _paused   = true;
        _pausedMs = millis();
    }
}

void AudioEngine::resume() {
    if (_paused) {
        // Reset output state before resuming decode (like BrokenSignal resumeAudio())
        if (_impl && _impl->out) _impl->out->begin();
        _startMs += millis() - _pausedMs;
        _paused   = false;
    }
}

void AudioEngine::stop() {
    if (!_impl) return;
    if (_impl->gen && _impl->gen->isRunning()) _impl->gen->stop();
    if (_impl->out) _impl->out->stop();
    _impl->teardown();
    _paused = false;
}

bool AudioEngine::isPlaying() const {
    return _impl && _impl->gen && _impl->gen->isRunning() && !_paused;
}

// ── update ────────────────────────────────────────────────────────────────────

bool AudioEngine::update() {
    if (!_impl || !_impl->gen || _paused) return false;
    if (!_impl->gen->isRunning())         return false;
    if (!_impl->gen->loop()) {
        LOG_I("AUDIO", "ended: %s", _impl->path);
        if (_impl->out) _impl->out->stop();
        _impl->teardown();
        return true;
    }
    return false;
}

// ── Volume ───────────────────────────────────────────────────────────────────

void AudioEngine::setVolume(uint8_t vol) {
    _volume = (vol > 21) ? 21 : vol;
    applyHwVolume();
}

void AudioEngine::setMuted(bool m) {
    _muted = m;
    applyHwVolume();
}

void AudioEngine::applyHwVolume() {
    // The codec/amp clips well below M5.Speaker's full range, so quarter the
    // hardware level (0–21 → 0–52 instead of 0–210) for headroom / no distortion.
    // Muting forces it to 0 while keeping _volume so unmute restores the level.
    M5Cardputer.Speaker.setVolume(_muted ? 0 : (_volume * 10 / 4));
}

// ── Progress ─────────────────────────────────────────────────────────────────

uint32_t AudioEngine::positionMs() const {
    if (!_impl || !_impl->gen || !_impl->gen->isRunning()) return 0;
    if (_paused) return _pausedMs - _startMs;
    return millis() - _startMs;
}

uint32_t AudioEngine::durationMs() const {
    if (!_impl) return 0;
    // estDurMs is now computed exactly from the container header up front, so
    // prefer it; fall back to the ID3 TLEN tag only if parsing failed.
    return (_impl->estDurMs > 0) ? _impl->estDurMs : _impl->id3DurMs;
}

// ── Metadata ─────────────────────────────────────────────────────────────────

static bool isValidUTF8(const char* s) {
    while (*s) {
        uint8_t c = (uint8_t)*s++;
        if (c < 0x80) continue;
        int n = (c < 0xE0) ? 1 : (c < 0xF0) ? 2 : 3;
        for (int i = 0; i < n; ++i)
            if (((uint8_t)*s++ & 0xC0) != 0x80) return false;
    }
    return true;
}

const char* AudioEngine::currentTitle() const {
    if (!_impl) return "";
    if (_impl->title[0] && isValidUTF8(_impl->title)) return _impl->title;
    if (!_impl->path[0]) return "";
    const char* sl = strrchr(_impl->path, '/');
    return sl ? sl + 1 : _impl->path;
}
const char* AudioEngine::artist() const {
    if (!_impl || !_impl->artist[0] || !isValidUTF8(_impl->artist)) return "";
    return _impl->artist;
}
const char* AudioEngine::album() const {
    if (!_impl || !_impl->album[0]  || !isValidUTF8(_impl->album))  return "";
    return _impl->album;
}
const char* AudioEngine::currentPath() const {
    return (_impl && _impl->path[0]) ? _impl->path : "";
}

// ── DebugConsole ─────────────────────────────────────────────────────────────

void AudioEngine::registerConsole() {
    auto& con = DebugConsole::instance();
    con.registerCommand("play", "play <path>", [](int argc, char** argv, Print& out) {
        if (argc < 2) { out.println("usage: play <path>"); return; }
        char arg[256]={}, p[256]={};
        for (int i=1;i<argc;++i) { if(i>1) strlcat(arg," ",sizeof(arg)); strlcat(arg,argv[i],sizeof(arg)); }
        snprintf(p,sizeof(p),"%s%s",arg[0]=='/'?"":"/Cardio/music/",arg);
        AudioEngine::instance().play(p);
    });
    con.registerCommand("pause",  "pause",  [](int,char**,Print&){ AudioEngine::instance().pause(); });
    con.registerCommand("resume", "resume", [](int,char**,Print&){ AudioEngine::instance().resume(); });
    con.registerCommand("stop",   "stop",   [](int,char**,Print&){ AudioEngine::instance().stop(); });
    con.registerCommand("vol", "vol [0-21]", [](int argc, char** argv, Print& out) {
        auto& e = AudioEngine::instance();
        if (argc >= 2) e.setVolume((uint8_t)atoi(argv[1]));
        out.printf("vol=%u\n", e.volume());
    });
    con.registerCommand("mute", "mute [on|off]", [](int argc, char** argv, Print& out) {
        auto& e = AudioEngine::instance();
        if (argc >= 2) {
            const char* a = argv[1];
            e.setMuted(!strcasecmp(a,"on") || !strcasecmp(a,"true") || !strcmp(a,"1"));
        } else e.setMuted(!e.isMuted());
        out.printf("muted=%s\n", e.isMuted() ? "y" : "n");
    });
    con.registerCommand("status", "status", [](int,char**,Print& out) {
        auto& e = AudioEngine::instance();
        out.printf("playing=%s paused=%s vol=%u pos=%lums dur=%lums heap=%u\n",
            e.isPlaying()?"y":"n", e.isPaused()?"y":"n",
            e.volume(),
            (unsigned long)e.positionMs(), (unsigned long)e.durationMs(),
            esp_get_free_heap_size());
    });
    con.registerCommand("tone", "1kHz 500ms", [](int,char**,Print& out) {
        M5Cardputer.Speaker.tone(1000, 500); out.println("1kHz 500ms");
    });
}
