#pragma once
#include <Arduino.h>

class AudioEngine {
public:
    static AudioEngine& instance();

    void begin();
    bool play(const char* path);   // 从 SD 播放文件，自动识别格式
    void pause();
    void resume();
    void stop();
    bool isPlaying() const;
    bool isPaused()  const { return _paused; }

    void    setVolume(uint8_t vol);  // 0–21，映射到 ES8311
    uint8_t volume()  const { return _volume; }

    void update();          // 在 loop() 中调用，驱动解码器
    void registerConsole(); // 注册 play/pause/resume/stop/vol/status 命令

    // 进度 / 元数据（供 UI 读取）
    uint32_t    positionMs()   const;
    uint32_t    durationMs()   const;
    const char* currentTitle() const;   // ID3 TIT2，回退到文件名
    const char* artist()       const;   // ID3 TPE1
    const char* album()        const;   // ID3 TALB

private:
    AudioEngine() = default;

    // Pimpl：把 ESP8266Audio 类型隐藏在 .cpp，避免污染头文件
    struct Impl;
    Impl*   _impl      = nullptr;
    uint8_t _volume    = 15;
    bool    _paused    = false;
    uint32_t _startMs  = 0;   // play() 时记录的 millis()
    uint32_t _pausedMs = 0;   // 累计暂停时长（用于修正 elapsed）
};
