#pragma once
#include <Arduino.h>

// Real-time N-band graphic equalizer.
//
// A cascade of peaking biquad (RBJ cookbook) filters applied to the decoded
// 16-bit stereo stream right before it lands in the speaker buffer — i.e. inside
// AudioOutputM5Speaker::ConsumeSample().  Each band is a bell curve at a fixed
// centre frequency with an adjustable ±MAX_DB gain; 0 dB is a mathematically
// exact pass-through, and when *every* band is 0 dB the whole stage is bypassed
// at zero cost (process() early-returns).
//
// Everything runs on the Arduino loop (core 1): the decoder feeds samples, the
// UI changes gains, all in the same task — so no locking is needed.  The speaker
// DMA task (core 0) only ever sees the already-filtered buffer.
class Equalizer {
public:
    static constexpr int NUM_BANDS = 5;
    static constexpr int MAX_DB    = 12;   // per-band boost/cut clamp

    static Equalizer& instance();

    // Filter one interleaved stereo frame in place (L = s[0], R = s[1]).
    // Hot path — kept inline and branch-light.
    inline void process(int16_t s[2]) {
        if (!_active) return;
        s[0] = filt(0, s[0]);
        s[1] = filt(1, s[1]);
    }

    // Filter a single mono sample (uses the channel-0 biquad state). Used on the
    // Cardputer ADV, whose ES8311 is a mono codec, so the stream is folded to one
    // channel before output — half the filtering work of stereo.
    inline int16_t processMono(int16_t in) {
        return _active ? filt(0, in) : in;
    }

    void setSampleRate(uint32_t hz);   // recompute coeffs when the track rate changes
    void resetState();                 // clear filter history (call on (re)start)

    void setBandGainDb(int band, int db);
    int  bandGainDb(int band) const { return (band >= 0 && band < NUM_BANDS) ? _db[band] : 0; }
    int  bandFreq(int band)   const;
    bool active()  const { return _active; }
    void reset();                      // all bands → 0 dB (flat)

    String gainsCsv() const;           // "0,3,-2,0,5"
    void   loadGainsCsv(const char* csv);

    void registerConsole();

private:
    Equalizer();

    struct Biquad { float b0, b1, b2, a1, a2; };  // a0 normalised to 1

    inline int16_t filt(int ch, int32_t in) {
        float x = (float)in;
        for (int b = 0; b < NUM_BANDS; ++b) {
            // Direct Form II Transposed — good float numerics, 2 state words/band.
            float y      = _bq[b].b0 * x + _z1[b][ch];
            _z1[b][ch]   = _bq[b].b1 * x - _bq[b].a1 * y + _z2[b][ch];
            _z2[b][ch]   = _bq[b].b2 * x - _bq[b].a2 * y;
            x = y;
        }
        if      (x >  32767.0f) x =  32767.0f;   // saturate, never wrap
        else if (x < -32768.0f) x = -32768.0f;
        return (int16_t)x;
    }

    void computeBand(int band);
    void recomputeActive();

    Biquad   _bq[NUM_BANDS];
    float    _z1[NUM_BANDS][2];   // [band][channel]
    float    _z2[NUM_BANDS][2];
    int8_t   _db[NUM_BANDS];
    uint32_t _fs     = 44100;
    bool     _active = false;
};
