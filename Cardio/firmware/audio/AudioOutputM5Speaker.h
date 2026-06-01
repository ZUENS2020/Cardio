#pragma once
#include <AudioOutput.h>
#include <M5Cardputer.h>

// 基于 M5Unified 官方示例 MP3_with_ESP8266Audio 的正确实现
// 三缓冲轮换：buffer 满时 ConsumeSample 返回 false（generator 下帧重试），
// 同时把满的 buffer 交给 Speaker DMA，切到下一个 buffer 继续填充。
// 不等 DMA 完成，三个 buffer 保证 DMA 播一个、填一个、备一个。
class AudioOutputM5Speaker : public AudioOutput {
public:
    static constexpr size_t BUF_SIZE = 1536 * 2;  // 立体声交织：1536 对 × 2

    explicit AudioOutputM5Speaker(uint8_t channel = 0);

    bool begin() override;
    bool ConsumeSample(int16_t sample[2]) override;
    void flush() override;
    bool stop() override;

    const int16_t* getBuffer() const { return _tri_buf[(_tri_idx + 2) % 3]; }

private:
    uint8_t  _ch;
    int16_t  _tri_buf[3][BUF_SIZE];  // 三缓冲，单声道
    size_t   _tri_idx = 0;
    size_t   _fill    = 0;
};
