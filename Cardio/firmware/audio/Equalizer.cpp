#include "Equalizer.h"
#include "DebugConsole.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Octave-ish spaced centres tuned for a small full-range speaker: low warmth,
// low-mid body, presence, clarity, air.  All well below Nyquist at 44.1k.
static const int kBandHz[Equalizer::NUM_BANDS] = { 100, 300, 1000, 3000, 8000 };

// Broad, overlapping bells so adjacent bands blend like a real graphic EQ.
static constexpr float kQ = 0.9f;

Equalizer& Equalizer::instance() {
    static Equalizer inst;
    return inst;
}

Equalizer::Equalizer() {
    memset(_db, 0, sizeof(_db));
    resetState();
    for (int b = 0; b < NUM_BANDS; ++b) computeBand(b);   // identity at 0 dB
    _active = false;
}

int Equalizer::bandFreq(int band) const {
    return (band >= 0 && band < NUM_BANDS) ? kBandHz[band] : 0;
}

void Equalizer::resetState() {
    memset(_z1, 0, sizeof(_z1));
    memset(_z2, 0, sizeof(_z2));
}

// RBJ "Audio EQ Cookbook" peaking-EQ coefficients.  At 0 dB (A = 1) the
// numerator equals the denominator, so H(z) = 1 exactly (true bypass).
void Equalizer::computeBand(int band) {
    float f0 = (float)kBandHz[band];

    // Guard against a centre at/above Nyquist on low-rate files → identity.
    if (f0 >= (float)_fs * 0.45f) {
        _bq[band] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        return;
    }

    float A     = powf(10.0f, (float)_db[band] / 40.0f);
    float w0    = 2.0f * (float)M_PI * f0 / (float)_fs;
    float cw    = cosf(w0);
    float sw    = sinf(w0);
    float alpha = sw / (2.0f * kQ);

    float a0 =  1.0f + alpha / A;
    _bq[band].b0 = ( 1.0f + alpha * A) / a0;
    _bq[band].b1 = (-2.0f * cw)        / a0;
    _bq[band].b2 = ( 1.0f - alpha * A) / a0;
    _bq[band].a1 = (-2.0f * cw)        / a0;
    _bq[band].a2 = ( 1.0f - alpha / A) / a0;
}

void Equalizer::recomputeActive() {
    _active = false;
    for (int b = 0; b < NUM_BANDS; ++b)
        if (_db[b] != 0) { _active = true; break; }
}

void Equalizer::setSampleRate(uint32_t hz) {
    if (hz == 0 || hz == _fs) return;
    _fs = hz;
    for (int b = 0; b < NUM_BANDS; ++b) computeBand(b);
    resetState();   // coeffs changed under our feet — drop stale history
}

void Equalizer::setBandGainDb(int band, int db) {
    if (band < 0 || band >= NUM_BANDS) return;
    if (db >  MAX_DB) db =  MAX_DB;
    if (db < -MAX_DB) db = -MAX_DB;
    if (_db[band] == db) return;
    _db[band] = (int8_t)db;
    computeBand(band);
    recomputeActive();
}

void Equalizer::reset() {
    memset(_db, 0, sizeof(_db));
    for (int b = 0; b < NUM_BANDS; ++b) computeBand(b);
    _active = false;
}

String Equalizer::gainsCsv() const {
    String s;
    for (int b = 0; b < NUM_BANDS; ++b) {
        if (b) s += ',';
        s += String((int)_db[b]);
    }
    return s;
}

void Equalizer::loadGainsCsv(const char* csv) {
    if (!csv || !csv[0]) return;
    int b = 0;
    const char* p = csv;
    while (*p && b < NUM_BANDS) {
        setBandGainDb(b, atoi(p));   // atoi stops at the comma; clamps internally
        const char* comma = strchr(p, ',');
        if (!comma) break;
        p = comma + 1;
        ++b;
    }
}

void Equalizer::registerConsole() {
    DebugConsole::instance().registerCommand(
        "eq", "eq <band gain> | flat | show",
        [](int argc, char** argv, Print& out) {
            auto& e = Equalizer::instance();
            if (argc >= 2 && !strcasecmp(argv[1], "flat")) {
                e.reset();
            } else if (argc >= 3) {
                e.setBandGainDb(atoi(argv[1]), atoi(argv[2]));
            }
            out.printf("eq active=%s gains=[%s] Hz=[",
                       e.active() ? "y" : "n", e.gainsCsv().c_str());
            for (int i = 0; i < NUM_BANDS; ++i)
                out.printf("%s%d", i ? "," : "", e.bandFreq(i));
            out.println("]");
        });
}
