// Logger — 分级日志，输出到 Serial / SD 卡文件（BLE NUS 通道在 Week 1 Day 6 接入）
//
// 用法：
//   Logger::instance().begin(/*sdEnabled=*/true, Logger::INFO);
//   LOG_I("AUDIO", "Playing: %s", name);
//
// 输出格式：[000123456][INFO ][AUDIO] Playing: 稻香.flac
//   - 时间戳 = millis()，9 位补零
//   - 级别 / tag 右补空格至 5 字符对齐
//
// SD 写入路径 /Cardio/logs/cardio.log，超 512KB 轮转（cardio.log → .1 → .2 → .3）
#pragma once

#include <Arduino.h>

class Logger {
public:
    enum Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, NONE = 4 };

    static Logger& instance();

    // sdEnabled=false 时仅输出 Serial（对应 config.txt debug_enabled=false）
    void begin(bool sdEnabled, Level level);

    void setLevel(Level level) { _level = level; }
    Level level() const { return _level; }

    // 运行时开关 SD 写入（挂载失败时内部会自动置 false）
    void setSdEnabled(bool enabled);
    bool sdEnabled() const { return _sdEnabled; }

    // 核心打印接口，由 LOG_x 宏调用
    void logf(Level level, const char* tag, const char* fmt, ...)
        __attribute__((format(printf, 4, 5)));

    // DebugConsole 命令：log dump / log clear
    void dumpTo(Print& out);
    void clearFile();

    static const char* levelName(Level level);   // "DEBUG".."ERROR"
    static bool parseLevel(const char* s, Level& out);

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeLine(const char* line, size_t len);
    void rotateIfNeeded(size_t incoming);

    Level _level = INFO;
    bool _sdEnabled = false;
    bool _sdReady = false;          // SD 实际可用（挂载成功 + logs 目录存在）
    SemaphoreHandle_t _mutex = nullptr;
};

// 日志宏 —— 后续所有模块统一用这套，禁止裸 Serial.println
#define LOG_D(tag, ...) Logger::instance().logf(Logger::DEBUG, tag, __VA_ARGS__)
#define LOG_I(tag, ...) Logger::instance().logf(Logger::INFO,  tag, __VA_ARGS__)
#define LOG_W(tag, ...) Logger::instance().logf(Logger::WARN,  tag, __VA_ARGS__)
#define LOG_E(tag, ...) Logger::instance().logf(Logger::ERROR, tag, __VA_ARGS__)
