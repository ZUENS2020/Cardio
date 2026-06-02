#pragma once

// Boot splash — a code-drawn CARDIO / ZUENS2020 wordmark with a brand-stripe
// sweep.  Pure M5GFX drawing into a PSRAM sprite: no SD asset, no GIF decoder.
class SplashScreen {
public:
    static SplashScreen& instance();

    // Blocking: runs the ~1.5s intro animation, then returns.
    void play();

private:
    SplashScreen() = default;
};
