#include "TextRender.h"
#include <string.h>

namespace {

// Decode one UTF-8 codepoint, advance *pp past it, and return its byte length
// (0 at the terminating NUL).  Malformed bytes are consumed one at a time so
// the loop can never stall.
int utf8Next(const char** pp, uint32_t* cp) {
    const uint8_t* p = (const uint8_t*)*pp;
    uint8_t c = p[0];
    if (c == 0) return 0;

    uint32_t u; int len;
    if      (c < 0x80)           { u = c;        len = 1; }
    else if ((c & 0xE0) == 0xC0) { u = c & 0x1F; len = 2; }
    else if ((c & 0xF0) == 0xE0) { u = c & 0x0F; len = 3; }
    else if ((c & 0xF8) == 0xF0) { u = c & 0x07; len = 4; }
    else                         { u = '?';      len = 1; } // stray byte

    for (int i = 1; i < len; ++i) {
        if ((p[i] & 0xC0) != 0x80) { len = i; break; }      // truncated sequence
        u = (u << 6) | (p[i] & 0x3F);
    }
    *cp = u;
    *pp += len;
    return len;
}

} // namespace

namespace theme {

void printUtf8(LGFX_Sprite& spr, const char* s) {
    if (!s) return;
    const char* p = s;
    for (;;) {
        const char* start = p;
        uint32_t cp;
        int len = utf8Next(&p, &cp);
        if (len <= 0) break;

        uint16_t u16 = (cp <= 0xFFFF) ? (uint16_t)cp : 0;
        lgfx::v1::FontMetrics fm;
        if (u16 && fonts::efontCN_10.updateFontMetric(&fm, u16))
            spr.setFont(&fonts::efontCN_10);
        else
            spr.setFont(&fonts::efontJA_10);

        char buf[5];
        memcpy(buf, start, (size_t)len);
        buf[len] = '\0';
        spr.print(buf);
    }
}

} // namespace theme
