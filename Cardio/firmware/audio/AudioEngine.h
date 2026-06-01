#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

class AudioEngine {
public:
    static AudioEngine& instance();

    void begin();
    bool play(const char* path);
    void pause();
    void resume();
    void stop();
    bool isPlaying() const;
    bool isPaused()  const { return _paused; }

    void    setVolume(uint8_t vol);
    uint8_t volume()  const { return _volume; }

    void update();           // Core 0 task 内部调用；loop() 不再需要调用
    void registerConsole();

    // 进度 / 元数据（供 UI 读取，跨核安全）
    uint32_t    positionMs()   const;
    uint32_t    durationMs()   const;
    const char* currentTitle() const;
    const char* artist()       const;
    const char* album()        const;

private:
    AudioEngine() = default;

    struct Impl;
    Impl*   _impl   = nullptr;
    uint8_t _volume = 15;
    bool    _paused = false;
    uint32_t _startMs  = 0;
    uint32_t _pausedMs = 0;

    SemaphoreHandle_t _mutex      = nullptr;  // 保护 _impl 跨核访问
    TaskHandle_t      _taskHandle = nullptr;

    static void audioTask(void* arg);  // 固定在 Core 0
};
