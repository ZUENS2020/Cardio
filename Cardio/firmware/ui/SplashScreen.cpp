#include "SplashScreen.h"
#include <M5Cardputer.h>
#include "Theme.h"

using namespace theme;

// Code-drawn boot splash — no SD asset, no GIF decoder.  Echoes the design
// motif: a green-yellow-green diagonal stripe sweeps right→left and reveals the
// CARDIO wordmark + ZUENS2020 id.  Everything is rendered into a PSRAM sprite.

// W / H come from theme:: (240 × 135)
static constexpr float TAN_A    = 0.4040f;             // tan(22°)
static constexpr int   HALF     = 14;                  // half stripe width (at top)
static constexpr int   GREEN_W  = 5;                   // each green band
static constexpr int   YELLOW_W = (HALF - GREEN_W) * 2;// 18px yellow center

static constexpr uint16_t kWhite    = 0xFFFF;
static constexpr uint16_t STRIPE_G = rgb565(0x51, 0xB6, 0x77); // brand green
static constexpr uint16_t STRIPE_Y = rgb565(0xFD, 0xD8, 0x35); // brand yellow

SplashScreen& SplashScreen::instance() {
    static SplashScreen inst;
    return inst;
}

// White background + CARDIO wordmark + divider + ZUENS2020 id.
static void drawContent(LGFX_Sprite& spr) {
    spr.fillSprite(kWhite);
    spr.setTextDatum(middle_center);

    spr.setFont(&fonts::Orbitron_Light_32);   // same family as the design
    spr.setTextColor(accent, kWhite);
    spr.drawString("CARDIO", W / 2, 56);
    int cw = spr.textWidth("CARDIO");

    spr.drawFastHLine(W / 2 - cw / 2, 78, cw, accentDim);

    spr.setFont(&fonts::FreeSansBold9pt7b);
    spr.setTextColor(accentDim, kWhite);
    spr.drawString("ZUENS2020", W / 2, 96);
}

void SplashScreen::play() {
    LGFX_Sprite spr(&M5Cardputer.Display);
    spr.setColorDepth(16);  // no PSRAM on this board — sprite lives in SRAM
    if (!spr.createSprite(W, H)) {
        // No sprite memory: draw a static wordmark straight to the panel.
        auto& d = M5Cardputer.Display;
        d.fillScreen(kWhite);
        d.setTextDatum(middle_center);
        d.setFont(&fonts::Orbitron_Light_32);
        d.setTextColor(accent, kWhite);
        d.drawString("CARDIO", W / 2, H / 2);
        delay(1200);
        return;
    }

    const float START = W + HALF + 10;            // off the right edge
    const float END   = -(HALF + H * TAN_A + 10); // off the left edge
    const int   SWEEP = 46;

    // Sweep the stripe; content is hidden to the left of its trailing edge,
    // so the wordmark is wiped into view as the stripe travels left.
    for (int f = 0; f <= SWEEP; ++f) {
        float t  = (float)f / SWEEP;
        float te = t * t * (3.0f - 2.0f * t);     // smoothstep ease
        float sx = START + (END - START) * te;    // stripe center at the top row

        drawContent(spr);
        for (int y = 0; y < H; ++y) {
            float shift = y * TAN_A;
            int trail = (int)(sx + HALF + shift);              // right edge this row
            if (trail > 0) spr.drawFastHLine(0, y, trail < W ? trail : W, kWhite);
            int xc = (int)(sx + shift);                        // stripe center this row
            spr.drawFastHLine(xc - HALF,           y, GREEN_W,  STRIPE_G);
            spr.drawFastHLine(xc - HALF + GREEN_W, y, YELLOW_W, STRIPE_Y);
            spr.drawFastHLine(xc + HALF - GREEN_W, y, GREEN_W,  STRIPE_G);
        }
        spr.pushSprite(0, 0);
        delay(15);
    }

    // Hold the clean wordmark briefly, then hand off to the player.
    drawContent(spr);
    spr.pushSprite(0, 0);
    delay(800);

    spr.deleteSprite();
}
