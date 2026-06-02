#pragma once
#include <Arduino.h>

// 3.5mm 耳机插拔检测
// ADV 文档没有独立 jack-detect GPIO，通过实测确认引脚。
// 确认前用 jdet 命令扫描候选引脚；确认后设置 JACK_PIN。
class JackMonitor {
public:
    // 未确认时设 -1（禁用检测，isJackIn 始终返回 false）
    static constexpr int JACK_PIN = -1;

    static JackMonitor& instance();

    void begin();
    void update();          // 在 loop() 中调用，带去抖动

    bool isJackIn() const { return _jackIn; }

    // 注册回调：插入 / 拔出时调用
    using Callback = void (*)();
    void onInsert(Callback cb) { _onInsert = cb; }
    void onRemove(Callback cb) { _onRemove = cb; }

    void registerConsole(); // 注册 jdet 诊断命令

private:
    JackMonitor() = default;

    bool     _jackIn   = false;
    int      _lastRaw  = -1;
    uint32_t _lastMs   = 0;
    Callback _onInsert = nullptr;
    Callback _onRemove = nullptr;

    static constexpr uint32_t DEBOUNCE_MS = 80;
};
