#include "Canvas.h"
#include "Theme.h"

namespace theme {

LGFX_Sprite& canvas() {
    static LGFX_Sprite spr(&M5Cardputer.Display);
    static bool created = false;
    if (!created) {
        spr.setColorDepth(16);
        spr.createSprite(W, H);   // theme::W × theme::H = 240 × 135
        created = true;
    }
    return spr;
}

}
