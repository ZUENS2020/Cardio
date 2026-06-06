#include "AudioOutputM5Speaker.h"
#include "Equalizer.h"

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
    Equalizer::instance().resetState();   // drop filter history from the last track
    return true;
}

bool AudioOutputM5Speaker::SetRate(int hz) {
    hertz = hz;
    Equalizer::instance().setSampleRate((uint32_t)hz);
    return true;
}

bool AudioOutputM5Speaker::ConsumeSample(int16_t s[2]) {
    if (_wv >= BUF_VALS) {
        flush();
        return false; // generator retries this sample next loop()
    }
    if (_stereo) {
        // External PCM5102A: a true stereo I2S DAC. Keep L/R independent and
        // EQ-filter each channel against its own biquad state (twice the work
        // of mono, fine on the S3). Output is genuine stereo from the DAC jack.
        Equalizer::instance().process(s); // filters s[0]=L, s[1]=R in place
        _buf[_wi][_wv++] = s[0]; // L
        _buf[_wi][_wv++] = s[1]; // R
        return true;
    }
    // The Cardputer ADV's ES8311 is a single-DAC *mono* codec; the speaker and the
    // 3.5mm jack share that one output (mechanical switch), so true stereo is
    // physically impossible. Fold L+R to a single mono sum (>>1 avoids overflow)
    // so nothing panned hard to one side is lost — the codec would otherwise pick
    // only one I2S slot — then EQ-filter that one channel and write it to both.
    int16_t m = Equalizer::instance().processMono(
                    (int16_t)(((int32_t)s[0] + (int32_t)s[1]) >> 1));
    _buf[_wi][_wv++] = m; // L
    _buf[_wi][_wv++] = m; // R (identical → full mix regardless of codec slot pick)
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
