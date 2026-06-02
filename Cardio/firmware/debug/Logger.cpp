#include "Logger.h"
#include <SD.h>
#include <stdarg.h>

namespace {
constexpr char   LOG_DIR[]    = "/Cardio/logs";
constexpr char   LOG_PATH[]   = "/Cardio/logs/cardio.log";
constexpr size_t MAX_BYTES    = 512 * 1024;   // 单文件上限 512KB
constexpr int    KEEP_FILES   = 3;            // 轮转保留 .1 .2 .3
constexpr size_t LINE_BUF     = 256;          // 单行最大长度（含格式头）
}  // namespace

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::begin(bool sdEnabled, Level level) {
    _level = level;
    if (_mutex == nullptr) {
        _mutex = xSemaphoreCreateMutex();
    }
    setSdEnabled(sdEnabled);
    LOG_I("LOG", "Logger ready, level=%s sd=%s",
          levelName(_level), _sdReady ? "on" : "off");
}

void Logger::setSdEnabled(bool enabled) {
    _sdEnabled = enabled;
    if (!enabled) {
        _sdReady = false;
        return;
    }
    // 假定 Cardio.ino 已完成 SD.begin()。这里仅确保 logs 目录存在。
    if (!SD.exists("/Cardio")) SD.mkdir("/Cardio");
    if (!SD.exists(LOG_DIR))   SD.mkdir(LOG_DIR);
    _sdReady = SD.exists(LOG_DIR);
    if (!_sdReady) {
        // 不能用 LOG_x（会递归），直接走 Serial
        Serial.println("[LOG] SD logs dir unavailable, file logging disabled");
    }
}

const char* Logger::levelName(Level level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO ";
        case WARN:  return "WARN ";
        case ERROR: return "ERROR";
        default:    return "NONE ";
    }
}

bool Logger::parseLevel(const char* s, Level& out) {
    if (!s) return false;
    if (!strcasecmp(s, "debug")) { out = DEBUG; return true; }
    if (!strcasecmp(s, "info"))  { out = INFO;  return true; }
    if (!strcasecmp(s, "warn"))  { out = WARN;  return true; }
    if (!strcasecmp(s, "error")) { out = ERROR; return true; }
    return false;
}

void Logger::logf(Level level, const char* tag, const char* fmt, ...) {
    if (level < _level) return;

    char line[LINE_BUF];
    // 头部：[时间戳][级别][TAG ]（tag 右补空格至 5 字符）
    int n = snprintf(line, sizeof(line), "[%09lu][%s][%-5.5s] ",
                     (unsigned long)millis(), levelName(level), tag ? tag : "?");
    if (n < 0) return;
    if (n > (int)sizeof(line) - 2) n = sizeof(line) - 2;

    va_list ap;
    va_start(ap, fmt);
    int m = vsnprintf(line + n, sizeof(line) - n - 1, fmt, ap);
    va_end(ap);
    if (m < 0) m = 0;

    size_t len = n + m;
    if (len > sizeof(line) - 2) len = sizeof(line) - 2;
    line[len++] = '\n';
    line[len]   = '\0';

    writeLine(line, len);
}

void Logger::writeLine(const char* line, size_t len) {
    bool locked = _mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(50)) == pdTRUE;

    Serial.write((const uint8_t*)line, len);

    if (_sdReady) {
        rotateIfNeeded(len);
        File f = SD.open(LOG_PATH, FILE_APPEND);
        if (f) {
            f.write((const uint8_t*)line, len);
            f.close();
        }
    }

    if (locked) xSemaphoreGive(_mutex);
}

void Logger::rotateIfNeeded(size_t incoming) {
    File f = SD.open(LOG_PATH, FILE_READ);
    size_t sz = f ? f.size() : 0;
    if (f) f.close();
    if (sz + incoming <= MAX_BYTES) return;

    // cardio.log → .1 → .2 → .3，丢弃最旧
    char from[40], to[40];
    snprintf(to, sizeof(to), "%s.%d", LOG_PATH, KEEP_FILES);
    if (SD.exists(to)) SD.remove(to);
    for (int i = KEEP_FILES - 1; i >= 1; --i) {
        snprintf(from, sizeof(from), "%s.%d", LOG_PATH, i);
        snprintf(to,   sizeof(to),   "%s.%d", LOG_PATH, i + 1);
        if (SD.exists(from)) SD.rename(from, to);
    }
    snprintf(to, sizeof(to), "%s.1", LOG_PATH);
    SD.rename(LOG_PATH, to);
}

void Logger::dumpTo(Print& out) {
    if (!_sdReady) {
        out.println("[LOG] SD logging disabled, nothing to dump");
        return;
    }
    File f = SD.open(LOG_PATH, FILE_READ);
    if (!f) {
        out.println("[LOG] no log file");
        return;
    }
    uint8_t buf[256];
    while (f.available()) {
        size_t r = f.read(buf, sizeof(buf));
        out.write(buf, r);
    }
    f.close();
}

void Logger::clearFile() {
    if (!_sdReady) return;
    char path[40];
    if (SD.exists(LOG_PATH)) SD.remove(LOG_PATH);
    for (int i = 1; i <= KEEP_FILES; ++i) {
        snprintf(path, sizeof(path), "%s.%d", LOG_PATH, i);
        if (SD.exists(path)) SD.remove(path);
    }
    LOG_I("LOG", "log files cleared");
}
