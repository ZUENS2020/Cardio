#include "JackMonitor.h"
#include "DebugConsole.h"
#include "Logger.h"
#include <driver/gpio.h>

// 候选引脚：从 M5Unified 源码和 ADV 原理图推断
// GPIO  0 — ADV 唯一显式配置为 input 的引脚（M5Unified.cpp:1887）
// GPIO  1 — I2C INT1（TCA8418 或 IMU 中断，备选）
// GPIO  2 — I2C INT2（备选）
// GPIO 21 — ADV 另一个 input（M5Unified.cpp:209）
static constexpr int CANDIDATES[] = { 0, 1, 2, 21 };
static constexpr int N_CANDIDATES = sizeof(CANDIDATES) / sizeof(CANDIDATES[0]);

JackMonitor& JackMonitor::instance() {
    static JackMonitor inst;
    return inst;
}

void JackMonitor::begin() {
    if (JACK_PIN < 0) {
        LOG_W("JACK", "jack pin unknown — run 'jdet' to identify, then set JACK_PIN");
        return;
    }
    // 上拉输入：无插头时 jack-detect 引脚浮高，插头短接到 GND 后变低
    pinMode(JACK_PIN, INPUT_PULLUP);
    _lastRaw = digitalRead(JACK_PIN);
    _jackIn  = (_lastRaw == LOW);  // LOW = 插头接地 = 耳机已插入
    LOG_I("JACK", "monitoring GPIO%d, current=%s", JACK_PIN, _jackIn ? "in" : "out");
}

void JackMonitor::update() {
    if (JACK_PIN < 0) return;

    int raw = digitalRead(JACK_PIN);
    if (raw == _lastRaw) { _lastMs = millis(); return; }

    // 状态变化：等去抖时间后确认
    if (millis() - _lastMs < DEBOUNCE_MS) return;

    _lastRaw = raw;
    _lastMs  = millis();
    bool newIn = (raw == LOW);

    if (newIn != _jackIn) {
        _jackIn = newIn;
        LOG_I("JACK", "jack %s", _jackIn ? "inserted" : "removed");
        if (_jackIn  && _onInsert) _onInsert();
        if (!_jackIn && _onRemove) _onRemove();
    }
}

void JackMonitor::registerConsole() {
    DebugConsole::instance().registerCommand("jdet",
        "jdet  scan candidate GPIOs — insert/remove jack while running",
        [](int, char**, Print& out) {
            // 把所有候选引脚设为上拉输入，读 10 次取众数
            out.println("扫描候选引脚（请慢慢插拔耳机）...");
            out.println("GPIO  | val");
            out.println("------+----");

            for (int i = 0; i < N_CANDIDATES; i++) {
                int pin = CANDIDATES[i];
                pinMode(pin, INPUT_PULLUP);
                delay(5);
                int val = digitalRead(pin);
                out.printf("GPIO%2d|  %d\n", pin, val);
            }
            out.println();
            out.println("插入耳机后再跑一次 jdet，对比哪个引脚从 1 变 0");
            out.println("找到后修改 JackMonitor.h 的 JACK_PIN 并重新烧录");
        });
}
