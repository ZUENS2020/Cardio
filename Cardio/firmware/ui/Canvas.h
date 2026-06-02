#pragma once
#include <M5Cardputer.h>

namespace theme {

// Single shared 240×135 16-bpp off-screen framebuffer.  PlayerScreen and
// BrowserScreen are never on screen at the same time, so they share ONE sprite
// instead of each owning a 63 KB one — saving 63 KB of SRAM (this board has no
// PSRAM, so internal RAM is the whole budget).  Allocated on first call.
LGFX_Sprite& canvas();

}
