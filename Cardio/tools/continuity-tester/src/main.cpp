// Cardputer ADV — 临时「通断表 / Continuity Tester」
//
// 用 GPIO 当万用表的通断挡：一个脚驱动成 LOW（黑表笔/参考地），另一个脚开
// 内部上拉（红表笔/测点）。两笔之间若导通，测点被拉到 0V → 读 LOW = 通；
// 开路时内部上拉把它拉到 3.3V → 读 HIGH = 断。
//
//   ┌─ 用途 ─────────────────────────────────────────────────────────┐
//   │ 测 GY-PCM5102 板上 SCK 与 GND 是否已（被背面焊桥）接地：       │
//   │   • G6 引线  →  PCM5102 的 GND 焊盘   （黑笔/参考）            │
//   │   • G5 引线  →  PCM5102 的 SCK 焊盘   （红笔/测点）            │
//   │   • PCM5102 **不要供电**（VIN 不接），只测裸板                 │
//   │ 屏显 CONNECTED(绿+响)=SCK 已接地，啥都不用做；OPEN=需自己桥。  │
//   └────────────────────────────────────────────────────────────────┘
//
// 也可当通用通断表：两根引线点任意两处，导通即 CONNECTED。
//
// ⚠️ 被测对象必须断电；表笔别碰 5V/电池，以免烧 GPIO。

#include <M5Cardputer.h>

constexpr gpio_num_t PIN_REF   = GPIO_NUM_6; // 参考地（驱动 LOW）→ 接 PCM5102 GND
constexpr gpio_num_t PIN_SENSE = GPIO_NUM_5; // 测点（INPUT_PULLUP）→ 接 PCM5102 SCK

static bool     conn      = false;
static bool     first     = true;
static uint32_t lastBeep  = 0;

// 采样 100 次取多数，避免抖动；导通(≈0Ω)稳定读 LOW，开路稳定读 HIGH。
static bool measure() {
    int lows = 0;
    for (int i = 0; i < 100; ++i) {
        if (digitalRead(PIN_SENSE) == LOW) ++lows;
        delayMicroseconds(30);
    }
    return lows >= 90; // ≥90% LOW 判为「通」
}

static void render() {
    auto& d = M5Cardputer.Display;
    uint16_t bg = conn ? d.color565(0, 120, 0) : d.color565(70, 70, 70);
    d.fillScreen(bg);
    d.setTextColor(TFT_WHITE, bg);
    d.setTextDatum(middle_center);

    d.setTextSize(2);
    d.drawString("CONTINUITY", 120, 16);

    d.setTextSize(4);
    d.drawString(conn ? "CONNECTED" : "OPEN", 120, 60);

    d.setTextSize(1);
    d.drawString(conn ? "SCK <-> GND : LINKED  (do nothing)"
                      : "SCK <-> GND : OPEN  (bridge it)", 120, 98);
    d.drawString("REF=G6  SENSE=G5   keep DAC board OFF", 120, 118);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    Serial.begin(115200);

    pinMode(PIN_REF, OUTPUT);
    digitalWrite(PIN_REF, LOW);        // 参考地
    pinMode(PIN_SENSE, INPUT_PULLUP);  // 测点上拉

    M5Cardputer.Display.setRotation(1);
    Serial.println("[continuity] REF=G6(LOW)  SENSE=G5(PULLUP)");
}

void loop() {
    bool now = measure();
    if (first || now != conn) {
        conn  = now;
        first = false;
        render();
        Serial.printf("[continuity] %s\n", conn ? "CONNECTED" : "OPEN");
    }
    // 导通时每 ~700ms 滴一声，像真的通断表
    if (conn && millis() - lastBeep > 700) {
        M5Cardputer.Speaker.tone(2300, 60);
        lastBeep = millis();
    }
    delay(40);
}
