#pragma once
#include <M5Cardputer.h>

namespace theme {

// Print UTF-8 text at the sprite's current cursor, using its current text size
// and color, with per-glyph font fallback: efontCN (simplified Chinese) first,
// then efontJA (kana / Cyrillic / Japanese-only kanji) for any glyph CN lacks.
// Neither efont covers both simplified Chinese *and* Japanese kana, so a single
// font leaves "豆腐" boxes on mixed libraries; checking each codepoint against
// efontCN via updateFontMetric and falling back to efontJA fixes that.
// A caller-set clip rect still bounds the output, so this is a drop-in for
// spr.print() on any CJK string.
void printUtf8(LGFX_Sprite& spr, const char* utf8);

}
