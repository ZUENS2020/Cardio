#include "AudioOutputM5Speaker.h"

AudioOutputM5Speaker::AudioOutputM5Speaker(m5::Speaker_Class* spk, uint8_t ch)
    : _spk(spk), _ch(ch), _wi(0), _wv(0) {
    for (int i = 0; i < 3; i++) {
        _buf[i] = new int16_t[BUF_VALS];
        memset(_buf[i], 0, BUF_VALS * sizeof(int16_t));
    }
}

AudioOutputM5Speaker::~AudioOutputM5Speaker() {
    for (int i = 0; i < 3; i++) delete[] _buf[i];
}

bool AudioOutputM5Speaker::begin() {
    _wi = 0;
    _wv = 0;
    return true;
}

bool AudioOutputM5Speaker::ConsumeSample(int16_t s[2]) {
    if (_wv >= BUF_VALS) {
        flush();
        return false; // generator retries this sample next loop()
    }
    _buf[_wi][_wv++] = s[0]; // L
    _buf[_wi][_wv++] = s[1]; // R
    return true;
}

// Submit the current half-buffer to the speaker.
// playRaw() returns false when its internal queue is full; we yield
// and retry rather than blocking with delay() so FreeRTOS can run.
void AudioOutputM5Speaker::flush() {
    if (_wv == 0) return;
    while (!_spk->playRaw(_buf[_wi], _wv, (uint32_t)hertz, true, 1, _ch))
        taskYIELD();
    _wi = (_wi < 2) ? _wi + 1 : 0;
    _wv = 0;
}

bool AudioOutputM5Speaker::stop() {
    flush();
    _spk->stop(_ch);
    return true;
}
