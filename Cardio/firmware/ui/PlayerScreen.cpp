#include "PlayerScreen.h"
#include <string.h>
#include <stdio.h>

PlayerScreen& PlayerScreen::instance() {
    static PlayerScreen inst;
    return inst;
}

void PlayerScreen::begin() {
    _spr.setPsram(false);   // PSRAM=0 时用 SRAM
    _spr.setColorDepth(16);
    _spr.createSprite(W, H);
    _dirty = true;
}

// ── 状态更新（任意一个变化就标脏）────────────────────────────────────────────
void PlayerScreen::setTrackTitle(const char* title) {
    if (strncmp(_title, title, sizeof(_title)) == 0) return;
    strlcpy(_title, title, sizeof(_title));
    _dirty = true;
}

void PlayerScreen::setArtist(const char* artist) {
    if (strncmp(_artist, artist, sizeof(_artist)) == 0) return;
    strlcpy(_artist, artist, sizeof(_artist));
    _dirty = true;
}

void PlayerScreen::setAlbum(const char* album) {
    if (strncmp(_album, album, sizeof(_album)) == 0) return;
    strlcpy(_album, album, sizeof(_album));
    _dirty = true;
}

void PlayerScreen::setProgress(uint32_t posMs, uint32_t durMs) {
    if (_posMs == posMs && _durMs == durMs) return;
    _posMs = posMs; _durMs = durMs;
    _dirty = true;
}

void PlayerScreen::setVolume(uint8_t vol) {
    if (_vol == vol) return;
    _vol = vol; _dirty = true;
}

void PlayerScreen::setPlaying(bool playing) {
    if (_playing == playing) return;
    _playing = playing; _dirty = true;
}

void PlayerScreen::setPlaylist(const char* name) {
    if (strncmp(_playlist, name, sizeof(_playlist)) == 0) return;
    strlcpy(_playlist, name, sizeof(_playlist));
    _dirty = true;
}

// ── 主更新（每帧调用）────────────────────────────────────────────────────────
void PlayerScreen::update() {
    if (!_dirty) return;
    _dirty = false;
    draw();
}

// UTF-8 字符数截断（不在多字节序列中间截断）
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

// ── 渲染（全部画到 Sprite 后一次 pushSprite）─────────────────────────────────
void PlayerScreen::draw() {
    auto& spr = _spr;
    spr.fillScreen(C_BG);

    // ── 行 1：标题 + 时间（y=2..13）─────────────────────────────────────────
    // 时间先算宽度，留出空间给标题
    spr.setFont(nullptr);
    spr.setTextSize(1);
    uint32_t pos = _posMs / 1000;
    char timebuf[16];
    if (_durMs > 0) {
        uint32_t dur = _durMs / 1000;
        snprintf(timebuf, sizeof(timebuf), "%02lu:%02lu/%02lu:%02lu",
                 pos/60, pos%60, dur/60, dur%60);
    } else {
        snprintf(timebuf, sizeof(timebuf), "%02lu:%02lu", pos/60, pos%60);
    }
    int tw = strlen(timebuf) * 6;
    spr.setTextColor(C_MUTED);
    spr.setCursor(W - MARGIN - tw, 3);
    spr.print(timebuf);

    // 标题（限制在时间左侧，最多 16 个 CJK 字符）
    spr.setFont(&fonts::efontCN_10);
    spr.setTextColor(C_TEXT);
    spr.setCursor(MARGIN, 2);
    char title[64];
    truncUTF8(title, sizeof(title), _title, 16);
    // 限制绘制区域，不让文字覆盖时间
    spr.setClipRect(MARGIN, 0, W - MARGIN - tw - 4, 14);
    spr.print(title);
    spr.clearClipRect();

    // ── 分隔线 ───────────────────────────────────────────────────────────
    spr.drawFastHLine(0, 15, W, 0x333333);

    // ── 行 2：进度条（y=17..23）─────────────────────────────────────────
    int barY = 17, barH = 5, barX = MARGIN, barW = W - MARGIN * 2;
    spr.fillRoundRect(barX, barY, barW, barH, 2, C_BAR_BG);
    if (_durMs > 0) {
        int fill = (int)((uint64_t)barW * _posMs / _durMs);
        fill = fill > barW ? barW : fill;
        if (fill > 0) spr.fillRoundRect(barX, barY, fill, barH, 2, C_BAR_FG);
    }

    // ── 分隔线 ───────────────────────────────────────────────────────────
    spr.drawFastHLine(0, 24, W, 0x333333);

    // ── 行 3：艺术家 + VOL + 播放图标（y=26..37）────────────────────────
    spr.setFont(&fonts::efontCN_10);
    spr.setTextColor(C_MUTED);
    spr.setCursor(MARGIN, 26);
    if (_artist[0]) {
        char artist[48];
        truncUTF8(artist, sizeof(artist), _artist, 12);
        spr.setClipRect(MARGIN, 24, W - 60, 14);
        spr.print(artist);
        spr.clearClipRect();
    }

    // VOL + 图标（右侧，ASCII 用默认字体）
    spr.setFont(nullptr);
    spr.setTextSize(1);
    char volbuf[10];
    snprintf(volbuf, sizeof(volbuf), "V%u", _vol);
    spr.setTextColor(C_MUTED);
    spr.setCursor(W - MARGIN - 20 - (int)strlen(volbuf)*6, 28);
    spr.print(volbuf);

    // ▶ / ‖ 图标
    int iconX = W - MARGIN - 10, iconY = 26;
    if (_playing) {
        for (int i = 0; i < 9; i++)
            spr.drawFastVLine(iconX + i/2, iconY + i/2, 9 - i, C_PRIMARY);
    } else {
        spr.fillRect(iconX,   iconY, 3, 9, C_PRIMARY);
        spr.fillRect(iconX+4, iconY, 3, 9, C_PRIMARY);
    }

    // ── 分隔线 ───────────────────────────────────────────────────────────
    spr.drawFastHLine(0, 38, W, 0x333333);

    // ── 下半部：封面占位 + 专辑名（y=40..134）────────────────────────────
    int coverY = 40, coverX = MARGIN;
    int coverS = H - coverY - MARGIN;  // ≈ 89px
    spr.fillRoundRect(coverX, coverY, coverS, coverS, 6, 0x1A1A2E);
    spr.setFont(nullptr);
    spr.setTextColor(0x2A2A4E);
    spr.setTextSize(4);
    spr.setCursor(coverX + coverS/2 - 12, coverY + coverS/2 - 16);
    spr.print("J");

    // 专辑名（封面右侧）
    int infoX = coverX + coverS + MARGIN;
    spr.setFont(&fonts::efontCN_10);
    if (_album[0]) {
        spr.setTextColor(C_MUTED);
        spr.setCursor(infoX, coverY + 4);
        char album[48];
        truncUTF8(album, sizeof(album), _album, 10);
        spr.setClipRect(infoX, coverY, W - infoX - MARGIN, 80);
        spr.print(album);
        spr.clearClipRect();
    }

    spr.pushSprite(&M5Cardputer.Display, 0, 0);
}
