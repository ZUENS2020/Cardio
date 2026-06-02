#include "BrowserScreen.h"
#include "PlaybackController.h"
#include "TextRender.h"
#include "Canvas.h"
#include "Logger.h"
#include <stdio.h>
#include <string.h>

using namespace theme;

BrowserScreen& BrowserScreen::instance() {
    static BrowserScreen inst;
    return inst;
}

void BrowserScreen::begin() {
    theme::canvas();   // shared framebuffer (allocated once)
    _dirty = true;
}

void BrowserScreen::openFolders() {
    _bmode = BROWSER_FOLDER;
    _folderCount = PlaybackController::instance().folderCount();
    _sel = 0;
    _scroll = 0;
    _folderIdx = -1;
    _trackIdx = -1;
    _dirty = true;
}

void BrowserScreen::openFolder(int idx) {
    auto& pc = PlaybackController::instance();
    if (!pc.selectFolder(idx)) return;
    _bmode = BROWSER_TRACK;
    _folderIdx = idx;
    _trackCount = pc.totalTracks();
    _sel = 0;
    _scroll = 0;
    _trackIdx = -1;
    _dirty = true;
}

void BrowserScreen::navigate(int dir) {
    int t = total();
    if (t == 0) return;
    _sel += dir;
    if (_sel < 0) _sel = 0;
    if (_sel >= t) _sel = t - 1;
    ensureVisible();
    _dirty = true;
}

void BrowserScreen::selectCurrent() {
    if (_bmode == BROWSER_FOLDER) {
        openFolder(_sel);
    } else {
        _trackIdx = _sel;
        PlaybackController::instance().selectTrack(_sel);
    }
}

bool BrowserScreen::goBack() {
    if (_bmode == BROWSER_TRACK) {
        openFolders();
        return true;
    }
    return false;
}

void BrowserScreen::exit() {
    _bmode = BROWSER_FOLDER;
}

void BrowserScreen::ensureVisible() {
    if (_sel < _scroll) _scroll = _sel;
    if (_sel >= _scroll + VISIBLE) _scroll = _sel - VISIBLE + 1;
}

void BrowserScreen::draw() {
    auto& spr = theme::canvas();
    spr.fillSprite(bg);

    auto& pc = PlaybackController::instance();

    // ── Header ───────────────────────────────────────────────────
    spr.fillRect(0, 0, W, HEAD_H, surface1);
    spr.setFont(nullptr);
    spr.setTextSize(1);
    spr.setTextColor(fg1);
    spr.setCursor(s, 2);

    if (_bmode == BROWSER_FOLDER) {
        spr.print("Playlists");
    } else {
        const char* fn = pc.folderName(_folderIdx);
        theme::printUtf8(spr, fn ? fn : "Music");
    }

    // Right: count (reset to the ASCII font — printUtf8 above may leave an efont)
    spr.setFont(nullptr);
    spr.setTextColor(fg2);
    char cnt[16];
    int t = total();
    if (t > 0) snprintf(cnt, sizeof(cnt), "%d/%d", _sel + 1, t);
    else strlcpy(cnt, "empty", sizeof(cnt));
    int cw = strlen(cnt) * 6;
    spr.setCursor(W - s - cw, 2);
    spr.print(cnt);

    // ── List ─────────────────────────────────────────────────────
    int y = HEAD_H;
    spr.setFont(&fonts::efontJA_10);  // CJK + Japanese kana + Russian
    spr.setTextSize(1);
    for (int i = 0; i < VISIBLE; i++) {
        int idx = _scroll + i;
        if (idx >= t) break;
        int rowY = y + i * ROW_H;

        if (idx == _sel) {
            spr.fillRect(0, rowY, W, ROW_H, surface3);
            spr.fillRect(0, rowY, 2, ROW_H, accent);
            spr.setTextColor(fg0);
        } else {
            spr.setTextColor(fg1);
        }

        spr.setCursor(s + 4, rowY + 2);

        if (_bmode == BROWSER_FOLDER) {
            // Folder name — no prefix
            const char* name = pc.folderName(idx);
            if (name) theme::printUtf8(spr, name);
            else spr.print("?");
        } else {
            // Track name — single line truncation
            const char* name = pc.trackName(idx);
            if (!name || !name[0]) { spr.print("?"); continue; }
            int maxW = W - s * 2 - 4;
            spr.setClipRect(s + 4, rowY, maxW, ROW_H);
            theme::printUtf8(spr, name);
            spr.clearClipRect();
        }
    }

    // ── Footer ───────────────────────────────────────────────────
    int footY = H - FOOT_H;
    spr.fillRect(0, footY, W, FOOT_H, surface1);
    spr.setTextColor(fg2);
    spr.setTextSize(1);
    spr.setCursor(s, footY + 2);
    if (_bmode == BROWSER_FOLDER) {
        spr.print("Enter=open  Del=back");
    } else {
        spr.print("Enter=play  Del=back");
    }

    // Page dots — first page = leftmost dot, current page lit, cluster right-aligned
    if (t > VISIBLE) {
        int pages = (t + VISIBLE - 1) / VISIBLE;
        int curPage = _sel / VISIBLE;
        int shown = (pages < 8) ? pages : 8;
        int rightX = W - s - 8;                  // rightmost dot (last shown page)
        for (int i = 0; i < shown; i++) {
            int x = rightX - (shown - 1 - i) * 8;  // i=0 → leftmost, fills rightward
            spr.fillRoundRect(x, footY + 3, 5, 5, 2, (i == curPage) ? accent : surface3);
        }
    }

    spr.pushSprite(&M5Cardputer.Display, 0, 0);
}