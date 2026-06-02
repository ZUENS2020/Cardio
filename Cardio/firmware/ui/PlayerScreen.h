#pragma once
#include <Arduino.h>
#include <M5Cardputer.h>
#include "Theme.h"

class PlayerScreen {
public:
    static PlayerScreen& instance();

    void begin();
    void update();

    void setTrackTitle(const char* title);
    void setArtist(const char* artist);
    void setAlbum(const char* album);
    void setFormat(const char* fmt);
    void setProgress(uint32_t posMs, uint32_t durMs);
    void setVolume(uint8_t vol);
    void setMuted(bool muted);
    void setPlaying(bool playing);
    void setOrder(uint8_t order);
    void setPlaylist(const char* name);

    void markDirty() { _dirty = true; }

private:
    PlayerScreen() = default;

    void draw();
    void drawStatusbar();
    void drawBody();

    char     _title[128]    = "No track";
    char     _artist[128]   = "";
    char     _album[128]    = "";
    char     _format[32]    = "";
    char     _playlist[64]  = "";
    uint32_t _posMs         = 0;
    uint32_t _durMs         = 0;
    uint8_t  _vol           = 0;
    bool     _muted         = false;
    uint8_t  _order         = theme::SEQ;
    bool     _playing       = false;
    bool     _dirty         = true;

    static constexpr int W = theme::W;
    static constexpr int H = theme::H;
};