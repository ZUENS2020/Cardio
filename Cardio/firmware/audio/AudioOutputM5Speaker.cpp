#include "AudioOutputM5Speaker.h"

AudioOutputM5Speaker::AudioOutputM5Speaker(uint8_t channel)
    : _ch(channel) {}

bool AudioOutputM5Speaker::ConsumeSample(int16_t sample[2]) {
    if (_fill < BUF_SIZE) {
        _tri_buf[_tri_idx][_fill    ] = sample[0];  // L
        _tri_buf[_tri_idx][_fill + 1] = sample[1];  // R
        _fill += 2;
        return true;
    }
    // buffer 满：把当前 buffer 交给 Speaker，切下一个 buffer，返回 false
    // ESP8266Audio generator 收到 false 后会在下帧重试这个 sample
    flush();
    return false;
}

void AudioOutputM5Speaker::flush() {
    if (_fill == 0) return;
    M5Cardputer.Speaker.playRaw(
        _tri_buf[_tri_idx],
        _fill,
        (uint32_t)hertz,
        true,   // stereo
        1,      // repeat
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
