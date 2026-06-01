#pragma once
#include <Arduino.h>
#include <M5Cardputer.h>

// 播放器主界面，全 Sprite 离屏渲染后一次 pushSprite 避免闪烁
class PlayerScreen {
public:
    static PlayerScreen& instance();

    void begin();
    void update();      // 每帧调用，内部按脏标志决定是否重绘

    // 外部状态推入（由 AudioEngine / PlaybackController 调用）
    void setTrackTitle(const char* title);
    void setArtist(const char* artist);
    void setAlbum(const char* album);
    void setProgress(uint32_t posMs, uint32_t durMs);
    void setVolume(uint8_t vol);
    void setPlaying(bool playing);
    void setPlaylist(const char* name);

    void markDirty() { _dirty = true; }

private:
    PlayerScreen() = default;

    void draw();

    LGFX_Sprite _spr;   // 240×135 全屏 Sprite

    // 当前状态
    char     _title[128]    = "No track";
    char     _artist[128]   = "";
    char     _album[128]    = "";
    char     _playlist[64]  = "";
    uint32_t _posMs         = 0;
    uint32_t _durMs         = 0;
    uint8_t  _vol           = 15;
    bool     _playing       = false;
    bool     _dirty         = true;

    // 颜色（来自设计系统）
    static constexpr uint32_t C_BG       = 0x0D0D0D;  // 近黑背景
    static constexpr uint32_t C_PRIMARY   = 0xFF7A1A;  // 橙色 accent
    static constexpr uint32_t C_TEXT      = 0xF0F0F0;  // 主文字白
    static constexpr uint32_t C_MUTED     = 0x888888;  // 次要文字灰
    static constexpr uint32_t C_BAR_BG    = 0x333333;  // 进度条背景
    static constexpr uint32_t C_BAR_FG    = 0xFF7A1A;  // 进度条前景

    // 布局
    static constexpr int W  = 240;
    static constexpr int H  = 135;
    static constexpr int MARGIN = 6;
};
