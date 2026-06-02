#pragma once
#include <Arduino.h>

namespace theme {

static constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ── Colors (from design system) ──────────────────────────────────────────
constexpr uint16_t bg         = rgb565(0x0B, 0x0D, 0x10);
constexpr uint16_t surface1   = rgb565(0x15, 0x18, 0x1D);
constexpr uint16_t surface2   = rgb565(0x1F, 0x24, 0x2B);
constexpr uint16_t surface3   = rgb565(0x2A, 0x30, 0x39);
constexpr uint16_t line       = rgb565(0x2C, 0x33, 0x3C);
constexpr uint16_t lineBright= rgb565(0x3A, 0x42, 0x4D);
constexpr uint16_t fg0       = rgb565(0xED, 0xF0, 0xF3);
constexpr uint16_t fg1       = rgb565(0xB9, 0xC0, 0xC9);
constexpr uint16_t fg2       = rgb565(0x81, 0x8A, 0x95);
constexpr uint16_t fg3       = rgb565(0x5A, 0x63, 0x6E);
constexpr uint16_t accent    = rgb565(0xFF, 0x7A, 0x1A);
constexpr uint16_t accentBrt = rgb565(0xFF, 0x98, 0x47);
constexpr uint16_t accentDim = rgb565(0xC2, 0x5A, 0x0F);
constexpr uint16_t accentInk = rgb565(0x16, 0x0A, 0x02);
constexpr uint16_t ok        = rgb565(0x3F, 0xCF, 0x6A);
constexpr uint16_t warn      = rgb565(0xFF, 0xC5, 0x3D);
constexpr uint16_t err       = rgb565(0xFF, 0x52, 0x47);
constexpr uint16_t info      = rgb565(0x3A, 0xC0, 0xE8);
constexpr uint16_t cyan      = rgb565(0x2F, 0xD4, 0xC6);
constexpr uint16_t cyanDim   = rgb565(0x1E, 0x8E, 0x85);

// ── Spacing (px) ─────────────────────────────────────────────────────────
constexpr int xs = 2, s = 4, m = 8, l = 12, xl = 16;

// ── Border radius ───────────────────────────────────────────────────────
constexpr int radiusSm = 3, radiusMd = 6, radiusLg = 10;

// ── Screen dimensions ────────────────────────────────────────────────────
constexpr int W = 240;
constexpr int H = 135;

// ── Layout zones ────────────────────────────────────────────────────────
constexpr int statusbarH = 16;
constexpr int bodyH     = H - statusbarH;  // 119

// ── Playback order ──────────────────────────────────────────────────────
enum Order : uint8_t { SEQ = 0, SHF, RPT, RPT1 };

} // namespace theme