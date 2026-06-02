#pragma once
#include <Arduino.h>
#include <M5Cardputer.h>
#include "Theme.h"

// 5-band equalizer screen: vertical sliders the user nudges live while music
// keeps playing.  Shares the global framebuffer (theme::canvas()) like the
// player/browser, and only repaints when something actually changed.
class EqScreen {
public:
    static EqScreen& instance();

    void open();                       // entering the screen (reset selection)
    void update();                     // redraw if dirty
    void markDirty() { _dirty = true; }

    int  selected() const { return _sel; }
    void select(int i);                // clamp to [0, NUM_BANDS) + mark dirty

private:
    EqScreen() = default;
    void draw();

    int  _sel   = 0;
    bool _dirty = true;
};
