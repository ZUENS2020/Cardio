#pragma once
#include <Arduino.h>
#include <M5Cardputer.h>
#include "Theme.h"

#define BROWSER_FOLDER 0
#define BROWSER_TRACK  1

class BrowserScreen {
public:
    static BrowserScreen& instance();

    void begin();

    void openFolders();                   // enter folder browser
    void openFolder(int idx);             // select folder, show tracks
    void navigate(int dir);               // -1=up, +1=down
    void selectCurrent();                 // enter folder or play track
    bool goBack();                        // true=still in browser, false=exited to player
    void exit();                          // force return to player

    int  selectedFolder() const { return _folderIdx; }
    int  selectedTrack()  const { return _trackIdx; }
    int  mode()           const { return _bmode; }

    void draw();
    void markDirty() { _dirty = true; }

private:
    BrowserScreen() = default;

    int  _bmode       = BROWSER_FOLDER;
    int  _folderCount = 0;
    int  _trackCount  = 0;
    int  _sel         = 0;
    int  _folderIdx   = -1;
    int  _trackIdx    = -1;
    int  _scroll      = 0;
    bool _dirty       = true;

    static constexpr int W      = theme::W;
    static constexpr int H      = theme::H;
    static constexpr int HEAD_H = 13;
    static constexpr int FOOT_H = 10;
    static constexpr int ROW_H  = 13;
    static constexpr int VISIBLE = (H - HEAD_H - FOOT_H) / ROW_H;

    int  total() const { return _bmode == BROWSER_FOLDER ? _folderCount : _trackCount; }
    void ensureVisible();
    void drawFolder();

    void drawTrackList();
};