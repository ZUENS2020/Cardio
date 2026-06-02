#include "PlayerScreen.h"
#include "TextRender.h"
#include "Canvas.h"
#include <string.h>
#include <stdio.h>

using namespace theme;

PlayerScreen& PlayerScreen::instance() {
    static PlayerScreen inst;
    return inst;
}

void PlayerScreen::begin() {
    theme::canvas();   // ensure the shared framebuffer is allocated
    _dirty = true;
}

// ── State setters ──────────────────────────────────────────────────────────
void PlayerScreen::setTrackTitle(const char* t) {
    if (strncmp(_title, t, sizeof(_title)) == 0) return;
    strlcpy(_title, t, sizeof(_title));
    _dirty = true;
}
void PlayerScreen::setArtist(const char* a) {
    if (strncmp(_artist, a, sizeof(_artist)) == 0) return;
    strlcpy(_artist, a, sizeof(_artist));
    _dirty = true;
}
void PlayerScreen::setAlbum(const char* a) {
    if (strncmp(_album, a, sizeof(_album)) == 0) return;
    strlcpy(_album, a, sizeof(_album));
    _dirty = true;
}
void PlayerScreen::setFormat(const char* f) {
    if (strncmp(_format, f, sizeof(_format)) == 0) return;
    strlcpy(_format, f, sizeof(_format));
    _dirty = true;
}
void PlayerScreen::setProgress(uint32_t p, uint32_t d) {
    if (_posMs == p && _durMs == d) return;
    _posMs = p; _durMs = d;
    _dirty = true;
}
void PlayerScreen::setVolume(uint8_t v) {
    if (_vol == v) return;
    _vol = v;
    _dirty = true;
}
void PlayerScreen::setMuted(bool m) {
    if (_muted == m) return;
    _muted = m;
    _dirty = true;
}
void PlayerScreen::setPlaying(bool p) {
    if (_playing == p) return;
    _playing = p;
    _dirty = true;
}
void PlayerScreen::setOrder(uint8_t o) {
    if (_order == o) return;
    _order = o;
    _dirty = true;
}
void PlayerScreen::setPlaylist(const char* n) {
    if (strncmp(_playlist, n, sizeof(_playlist)) == 0) return;
    strlcpy(_playlist, n, sizeof(_playlist));
    _dirty = true;
}

void PlayerScreen::update() {
    if (!_dirty) return;
    _dirty = false;
    draw();
}

// ── UTF-8 truncation ──────────────────────────────────────────────────────
static void truncUTF8(char* buf, size_t bufSize, const char* src, int maxChars) {
    int chars = 0;
    const char* p = src;
    while (*p && chars < maxChars) {
        uint8_t c = (uint8_t)*p;
        int bytes = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        if ((size_t)(p - src) + bytes + 3 >= bufSize) break;
        memcpy(buf + (p - src), p, bytes);
        p += bytes;
        chars++;
    }
    size_t len = p - src;
    if (*p) { buf[len] = '.'; buf[len+1] = '.'; buf[len+2] = 0; }
    else      buf[len] = 0;
}

// ── Draw ──────────────────────────────────────────────────────────────────
void PlayerScreen::draw() {
    auto& spr = theme::canvas();
    spr.fillSprite(bg);  // clear whole frame to prevent overlap
    drawStatusbar();
    drawBody();
    spr.pushSprite(&M5Cardputer.Display, 0, 0);
}

// ── Statusbar (16px) ─────────────────────────────────────────────────────
void PlayerScreen::drawStatusbar() {
    auto& spr = theme::canvas();
    spr.fillRect(0, 0, W, statusbarH, surface1);

    // Left: "CARDIO" — efontCN for potential CJK chars
    spr.setFont(&fonts::efontJA_10);
    spr.setTextColor(accent);
    spr.setCursor(s, 2);
    spr.print("CARDIO");

    // Right: battery % (default font — numbers only)
    spr.setFont(nullptr);
    spr.setTextSize(1);
    spr.setTextColor(fg2);
    int bat = M5Cardputer.Power.getBatteryLevel();
    if (bat < 0) bat = 0;
    if (bat > 100) bat = 100;
    char rbuf[12];
    snprintf(rbuf, sizeof(rbuf), "%d%%", bat);
    int rw = strlen(rbuf) * 6;
    spr.setCursor(W - s - rw, 3);
    spr.print(rbuf);

    spr.drawFastHLine(0, statusbarH - 1, W, line);
}

// ── Body (119px) ──────────────────────────────────────────────────────────
void PlayerScreen::drawBody() {
    auto& spr = theme::canvas();
    int y0 = statusbarH;   // 16
    int y1 = H;            // 135
    int bodyH = y1 - y0;  // 119

// Element heights
    constexpr int ptitleH = 20;
    constexpr int partistH = 20;
    constexpr int infoH    = 10;
    constexpr int progH    = 16;
    constexpr int volH     = 14;
    constexpr int totalElH = ptitleH + partistH + infoH + progH + volH;  // 80
    constexpr int numGaps = 5;
    int gap = (bodyH - totalElH) / numGaps;  // (119-80)/5 = 7

    int y = y0 + gap;

    // ── ptitle (efontJA_10 × textSize(2) = 20px) ─────────────────────────
    spr.setFont(&fonts::efontJA_10);
    spr.setTextSize(2);
    spr.setTextColor(fg0);
    {
        char buf[64];
        // At textSize(2), efontCN char is ~20px wide, so max ~10 chars fit on 240px
        truncUTF8(buf, sizeof(buf), _title, 10);
        spr.setClipRect(s, y, W - s * 2, ptitleH);
        spr.setCursor(s, y);
        theme::printUtf8(spr, buf);
        spr.clearClipRect();
    }
    y += ptitleH + gap;

    // ── partist (efontJA_10 × textSize(2) = 20px) ──────────────────────
    spr.setFont(&fonts::efontJA_10);
    spr.setTextSize(2);
    spr.setTextColor(fg1);
    {
        char buf[48];
        truncUTF8(buf, sizeof(buf), _artist, 10);
        spr.setClipRect(s, y, W - s * 2, partistH);
        spr.setCursor(s, y);
        theme::printUtf8(spr, buf);
        spr.clearClipRect();
    }
    spr.setTextSize(1);
    y += partistH + gap;

    // ── format + order tag (default font, same row) ──────────────────────
    spr.setFont(nullptr);
    spr.setTextSize(1);
    {
        // Format on left
        spr.setTextColor(fg2);
        if (_format[0]) {
            spr.setCursor(s, y + 2);
            spr.print(_format);
        }

        // Order tag on right
        const char* labels[] = {"SEQ", "SHF", "RPT", "R1"};
        const char* label = (_order < 4) ? labels[_order] : "SEQ";
        int tw = strlen(label) * 6 + s * 2;
        int tagX = W - s - tw;
        int tagY = y;
        spr.fillRoundRect(tagX, tagY, tw, 12, radiusSm, surface2);
        spr.drawRoundRect(tagX, tagY, tw, 12, radiusSm, accent);
        spr.fillRect(tagX, tagY + 1, 2, 10, accent);
        spr.setTextColor(accent);
        spr.setCursor(tagX + s + 1, tagY + 2);
        spr.print(label);
    }
    y += infoH + gap;

    // ── progress bar (default font for times) ────────────────────────────
    {
        spr.setFont(nullptr);
        spr.setTextSize(1);
        spr.setTextColor(fg2);
        char tl[8], tr[8];
        unsigned int ps = (unsigned int)(_posMs / 1000);
        unsigned int ds = (unsigned int)(_durMs / 1000);
        snprintf(tl, sizeof(tl), "%02u:%02u", ps / 60, ps % 60);
        snprintf(tr, sizeof(tr), "%02u:%02u", ds / 60, ds % 60);
        int tlW = strlen(tl) * 6;
        int trW = strlen(tr) * 6;
        spr.setCursor(s, y + 4);
        spr.print(tl);
        spr.setCursor(W - s - trW, y + 4);
        spr.print(tr);

        int barX = s + tlW + s;
        int barW = W - s * 2 - tlW - trW - s * 2;
        int barY = y + 5;
        int barH = 6;
        spr.fillRoundRect(barX, barY, barW, barH, 2, surface2);
        if (_durMs > 0) {
            int fill = (int)((uint64_t)barW * _posMs / _durMs);
            if (fill > barW) fill = barW;
            if (fill > 0) spr.fillRoundRect(barX, barY, fill, barH, 2, accent);
            int knobX = barX + fill - 2;
            if (knobX < barX) knobX = barX;
            if (knobX + 4 > barX + barW) knobX = barX + barW - 4;
            spr.fillRect(knobX, barY - 1, 4, barH + 2, fg0);
        }
    }
    y += progH + gap;

    // ── vol bar (10 segments, each representing 2 volume steps) ─────────
    {
        spr.setFont(nullptr);
        spr.setTextSize(1);
        spr.setTextColor(_muted ? err : fg2);
        spr.setCursor(s, y + 3);
        spr.print(_muted ? "MUTE" : "VOL");
        int volX = s + 26;
        int totalSegs = 10;
        int cellW = 10;
        int cellH = 10;
        int segGap = 3;
        // Convert 0-21 volume to 0-10 segments
        // vol=0 → 0 bars; vol>0 → at least 1 bar so non-zero vol never looks muted
        int filled = 0;
        if (_vol > 0) {
            filled = (_vol * totalSegs) / 21;
            if (filled < 1) filled = 1;
        }
        for (int i = 0; i < totalSegs; i++) {
            int cx = volX + i * (cellW + segGap);
            uint16_t col = (!_muted && i < filled) ? accent : surface3;
            spr.fillRoundRect(cx, y + 2, cellW, cellH, 2, col);
        }
    }
}