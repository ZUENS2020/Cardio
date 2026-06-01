#include "AudioOutputM5Speaker.h"
#include <M5Cardputer.h>

AudioOutputM5Speaker::AudioOutputM5Speaker(uint8_t channel)
    : _ch(channel) {}

bool AudioOutputM5Speaker::begin() {
    _fill = 0;
    _tri_idx = 0;
    return true;
}

bool AudioOutputM5Speaker::ConsumeSample(int16_t sample[2]) {
    if (_fill < BUF_SIZE) {
        _tri_buf[_tri_idx][_fill    ] = sample[0];  // L
        _tri_buf[_tri_idx][_fill + 1] = sample[1];  // R
        _fill += 2;
        return true;
    }
    flush();
    return false;
}

void AudioOutputM5Speaker::flush() {
    if (_fill == 0) return;
    // 等前一帧播完，加超时防死锁
    uint32_t wait = 0;
    while (M5Cardputer.Speaker.isPlaying(_ch) && wait < 1000) {
        vTaskDelay(1);
        wait++;
    }
    M5Cardputer.Speaker.playRaw(
        _tri_buf[_tri_idx],
        _fill,
        (uint32_t)hertz,
        true,   // stereo
        1,
        _ch
    );
    _tri_idx = (_tri_idx < 2) ? _tri_idx + 1 : 0;
    _fill = 0;
}

bool AudioOutputM5Speaker::stop() {
    flush();
    M5Cardputer.Speaker.stop(_ch);
    return true;
}
