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

private:
    AudioEngine() = default;

    // Pimpl：把 ESP8266Audio 类型隐藏在 .cpp，避免污染头文件
    struct Impl;
    Impl*   _impl   = nullptr;
    uint8_t _volume = 15;
    bool    _paused = false;
};
