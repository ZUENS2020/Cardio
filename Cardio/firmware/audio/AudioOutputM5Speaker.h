#pragma once
#include <AudioOutput.h>
#include <M5Cardputer.h>

// 基于 M5Unified 官方示例 MP3_with_ESP8266Audio 的正确实现
// 三缓冲轮换：buffer 满时 ConsumeSample 返回 false（generator 下帧重试），
// 同时把满的 buffer 交给 Speaker DMA，切到下一个 buffer 继续填充。
// 不等 DMA 完成，三个 buffer 保证 DMA 播一个、填一个、备一个。
class AudioOutputM5Speaker : public AudioOutput {
public:
    static constexpr size_t BUF_SIZE = 1536;  // 每个 buffer 的 int16_t 数（立体声交织）

    explicit AudioOutputM5Speaker(uint8_t channel = 0);

    bool begin() override { return true; }
    bool ConsumeSample(int16_t sample[2]) override;
    void flush() override;
    bool stop() override;

    // 供 FFT / 波形可视化读取最近播完的 buffer（后续 UI 用）
    const int16_t* getBuffer() const {
        return _tri_buf[(_tri_idx + 2) % 3];
    }

private:
    uint8_t  _ch;
    int16_t  _tri_buf[3][BUF_SIZE];
    size_t   _tri_idx   = 0;  // 当前正在填充的 buffer 索引
    size_t   _fill      = 0;  // 当前 buffer 已填充的 int16_t 数
};
