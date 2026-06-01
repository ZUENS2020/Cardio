#pragma once
#include <AudioOutput.h>
#include <M5Cardputer.h>

// 三缓冲无锁流水线：M5.Speaker playRaw() 是零拷贝（只存指针到 wavinfo[2] 双槽），
// 所以必须保证提交给 Speaker 的 buffer 在播放完成前不被覆盖。
// 三缓冲流水线：buf A 被 Speaker 持有 → buf B 已排入 Speaker → buf C 解码器正在填。
// Speaker 波形完成一个槽后自动翻转到下一个，解码器永远不等 DMA 完成。
class AudioOutputM5Speaker : public AudioOutput {
public:
    static constexpr size_t BUF_SIZE = 1536 * 2;

    explicit AudioOutputM5Speaker(uint8_t channel = 0);

    bool begin() override;
    bool ConsumeSample(int16_t sample[2]) override;
    void flush() override;
    bool stop() override;

private:
    uint8_t  _ch;
    int16_t  _tri_buf[3][BUF_SIZE];
    size_t   _tri_idx = 0;
    size_t   _fill    = 0;
};
