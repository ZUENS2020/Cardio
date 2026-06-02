#include "EqScreen.h"
#include "Canvas.h"
#include "Equalizer.h"
#include <stdio.h>
#include <string.h>

using namespace theme;

EqScreen& EqScreen::instance() {
    static EqScreen inst;
    return inst;
}

void EqScreen::open() {
    theme::canvas();   // ensure shared framebuffer exists
    _sel   = 0;
    _dirty = true;
}

void EqScreen::select(int i) {
    if (i < 0) i = 0;
    if (i >= Equalizer::NUM_BANDS) i = Equalizer::NUM_BANDS - 1;
    if (i == _sel) return;
    _sel   = i;
    _dirty = true;
}

void EqScreen::update() {
    if (!_dirty) return;
    _dirty = false;
    draw();
}

// Layout constants — body runs 16..135 (119px).
static constexpr int kTrackTop = 30;
static constexpr int kTrackBot = 102;
static constexpr int kCenterY  = (kTrackTop + kTrackBot) / 2;          // 0 dB line
static constexpr float kPxPerDb = (float)((kTrackBot - kTrackTop) / 2) // 36 / 12
                                / (float)Equalizer::MAX_DB;            // = 3.0 px/dB

void EqScreen::draw() {
    auto& spr = theme::canvas();
    auto& eq  = Equalizer::instance();
    const int N = Equalizer::NUM_BANDS;

    spr.fillSprite(bg);

    // ── Header ───────────────────────────────────────────────────────────
    spr.fillRect(0, 0, W, statusbarH, surface1);
    spr.setFont(&fonts::efontJA_10);
    spr.setTextColor(accent);
    spr.setCursor(s, 2);
    spr.print("EQUALIZER");

    spr.setFont(nullptr);
    spr.setTextSize(1);
    const char* st = eq.active() ? "ON" : "FLAT";
    spr.setTextColor(eq.active() ? ok : fg3);
    spr.setCursor(W - s - (int)strlen(st) * 6, 3);
    spr.print(st);
    spr.drawFastHLine(0, statusbarH - 1, W, line);

    // ── 0 dB reference line ──────────────────────────────────────────────
    const int margin = 8;
    const int slot   = (W - 2 * margin) / N;     // 44
    spr.drawFastHLine(margin, kCenterY, W - 2 * margin, surface2);

    // ── Bands ────────────────────────────────────────────────────────────
    for (int b = 0; b < N; ++b) {
        int  cx  = margin + slot * b + slot / 2;
        bool sel = (b == _sel);
        int  db  = eq.bandGainDb(b);
        int  hy  = kCenterY - (int)(db * kPxPerDb + (db >= 0 ? 0.5f : -0.5f));

        int      tw      = sel ? 6 : 4;
        uint16_t fillCol = (db >= 0) ? (sel ? accent : accentDim)
                                     : (sel ? info   : cyanDim);

        // track
        spr.fillRoundRect(cx - tw / 2, kTrackTop, tw, kTrackBot - kTrackTop, 2, surface3);
        // fill from the 0 dB line to the handle
        if      (db > 0) spr.fillRoundRect(cx - tw / 2, hy, tw, kCenterY - hy, 2, fillCol);
        else if (db < 0) spr.fillRoundRect(cx - tw / 2, kCenterY, tw, hy - kCenterY, 2, fillCol);

        // handle
        int knobW = sel ? 22 : 16;
        spr.fillRoundRect(cx - knobW / 2, hy - 3, knobW, 6, 2, sel ? accentBrt : fg2);

        // gain value (top)
        char gb[8];
        snprintf(gb, sizeof(gb), "%+d", db);
        spr.setTextColor(sel ? fg0 : fg2);
        spr.setCursor(cx - (int)strlen(gb) * 3, 19);
        spr.print(gb);

        // frequency label (bottom)
        char fb[8];
        int  f = eq.bandFreq(b);
        if (f >= 1000) snprintf(fb, sizeof(fb), "%dk", f / 1000);
        else           snprintf(fb, sizeof(fb), "%d", f);
        spr.setTextColor(sel ? accent : fg3);
        spr.setCursor(cx - (int)strlen(fb) * 3, 108);
        spr.print(fb);
    }

    // ── Footer hint ──────────────────────────────────────────────────────
    spr.setTextColor(fg3);
    spr.setCursor(s, 124);
    spr.print(",/ band   ;. gain   0 zero   Del back");

    spr.pushSprite(&M5Cardputer.Display, 0, 0);
}
