#pragma once
#include <Arduino.h>

class AudioEngine {
public:
    static AudioEngine& instance();

    void begin();

    // Start playing a file.  Accepts absolute paths like /Cardio/music/foo/bar.mp3
    bool play(const char* path);
    void pause();
    void resume();
    void stop(); // user-initiated stop; also stops hardware channel

    bool isPlaying() const;
    bool isPaused()  const { return _paused; }

    void    setVolume(uint8_t vol); // 0–21
    uint8_t volume()  const { return _volume; }

    void    setMuted(bool m);       // force hardware output to 0 regardless of volume
    bool    isMuted() const { return _muted; }

    // Master-volume ceiling at the top user step (loudness/headroom trim).
    // Lower it if the loudest setting distorts; raise it if it's too quiet.
    void    setGainCeiling(uint8_t c);
    uint8_t gainCeiling() const { return _volCeiling; }

    // Pump the decoder — call every loop iteration.
    // Returns true when the current track just finished (auto-advance cue).
    bool update();

    uint32_t    positionMs()   const;
    uint32_t    durationMs()   const;
    const char* currentTitle() const;
    const char* artist()       const;
    const char* album()        const;
    const char* currentPath()  const;

    void registerConsole();

private:
    AudioEngine() = default;

    void applyHwVolume();   // push _volume (or 0 if muted) to the speaker

    struct Impl;
    Impl*    _impl       = nullptr;
    uint8_t  _volume     = 15;
    uint8_t  _volCeiling = 80;   // master_volume cap at user-max; tunable via `gain` console cmd
    bool     _muted      = false;
    bool     _paused  = false;
    uint32_t _startMs  = 0;
    uint32_t _pausedMs = 0;
};
