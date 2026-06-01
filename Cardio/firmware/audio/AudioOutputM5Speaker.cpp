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
    if (_fill >= BUF_SIZE) {
        flush();
    }
    _tri_buf[_tri_idx][_fill    ] = sample[0];
    _tri_buf[_tri_idx][_fill + 1] = sample[1];
    _fill += 2;
    return true;
}

void AudioOutputM5Speaker::flush() {
    if (_fill == 0) return;
    M5Cardputer.Speaker.playRaw(
        _tri_buf[_tri_idx],
        _fill,
        (uint32_t)hertz,
        true,
        1,
        _ch
    );
    _tri_idx = (_tri_idx + 1) % 3;
    _fill = 0;
}

bool AudioOutputM5Speaker::stop() {
    _fill = 0;
    M5Cardputer.Speaker.stop(_ch);
    return true;
}