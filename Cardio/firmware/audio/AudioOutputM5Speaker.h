#pragma once
#include <AudioOutput.h>
#include <M5Cardputer.h>

// Direct port of BrokenSignal's AudioOutputM5Speaker pattern.
// Key design: heap-allocated triple buffers; flush() retries with
// taskYIELD() until playRaw() accepts the buffer (non-blocking);
// begin() only resets write indices; stop() flushes then halts.
class AudioOutputM5Speaker : public AudioOutput {
public:
    static constexpr size_t BUF_VALS = 1024 * 2; // int16 values per half-buffer

    AudioOutputM5Speaker(m5::Speaker_Class* spk, uint8_t ch = 0);
    ~AudioOutputM5Speaker();

    bool begin()                     override; // reset write indices
    bool ConsumeSample(int16_t s[2]) override;
    void flush()                     override; // submit current buffer, rotate
    bool stop()                      override; // flush + Speaker::stop

    bool SetRate(int hz)     override; // also retunes the equalizer to this rate
    bool SetChannels(int ch) override { channels = ch; return true; }
    bool SetBitsPerSample(int)       { return true; }

private:
    m5::Speaker_Class* _spk;
    uint8_t  _ch;
    int16_t* _buf[3]; // three heap-allocated half-buffers
    int      _wi;     // buffer index being written (0/1/2)
    size_t   _wv;     // values written so far in current buffer
};
