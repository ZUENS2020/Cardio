#pragma once
#include <Arduino.h>
#include <vector>
#include "AudioEngine.h"
#include "Theme.h"

// Unified music library + transport controller.
//
// The whole /Cardio/music/ tree is scanned ONCE in begin() into an
// in-memory index (_lib).  Each Folder keeps only the relative filenames
// of its tracks; the absolute path is rebuilt on demand by prepending
// MUSIC_ROOT (see fullPath()).  Because the index lives in RAM,
// selectFolder() never touches the SD card again — switching folders or
// tracks is instant, and the indices BrowserScreen displays map 1:1 to
// what actually plays.
//
// Folder = a subfolder of /Cardio/music/ that contains at least one
//          supported audio file.  Loose files sitting directly under the
//          root form a synthetic folder named "*".
class PlaybackController {
public:
    static PlaybackController& instance();

    void begin();

    // Pump audio and handle auto-advance.  Call every loop iteration.
    void update();

    // ── Library ────────────────────────────────────────────────────────────
    int         folderCount()         const;
    const char* folderName(int idx)   const;

    // Point the controller at a folder (no SD access — the index is in RAM).
    // Stops any current playback.
    bool selectFolder(int idx);

    // Track list for the currently selected folder.
    int         totalTracks()         const;
    const char* trackName(int idx)    const; // display name (no extension)
    const char* trackPath(int idx)    const; // absolute path

    // ── Transport ──────────────────────────────────────────────────────────
    bool play();          // start from current position (or first if not set)
    bool next();
    bool prev();
    void stop();
    void selectTrack(int idx); // play track by index immediately

    // ── Order ──────────────────────────────────────────────────────────────
    theme::Order order()  const { return _order; }
    void setOrder(theme::Order o);
    void cycleOrder();

    // ── Info for UI ────────────────────────────────────────────────────────
    const char* currentTrackPath()    const;
    int         currentTrackIndex()   const; // -1 if nothing loaded
    const char* currentFolderName()   const;
    int         currentFolderIndex()  const { return _folderIdx; }

    void onTrackEnd();

    void registerConsole();

private:
    PlaybackController() = default;

    // ── In-memory music library ──────────────────────────────────────────────
    struct Folder {
        String              name;   // display name ("*" = loose root files)
        String              rel;    // dir relative to MUSIC_ROOT ("" = root)
        std::vector<String> files;  // filenames (with extension), sorted
    };
    std::vector<Folder> _lib;       // entire library, scanned once at begin()
    int _folderIdx = -1;            // currently selected folder

    // ── Playback state for the current folder ────────────────────────────────
    std::vector<int>  _map;         // play order → file index within the folder
    int               _pos = -1;    // current position in _map
    theme::Order      _order = theme::SEQ;

    AudioEngine& _ae = AudioEngine::instance();

    // ── Internal ───────────────────────────────────────────────────────────
    void   scanLibrary();               // one-time full scan into _lib
    void   scanFolderFiles(Folder& f);  // fill f.files from SD
    void   buildMap();                  // (re)build _map for the current folder
    bool   advancePos();                // move _pos per order; false at SEQ end
    void   playCurrent();               // play _map[_pos]
    String fullPath(int fileIdx) const; // MUSIC_ROOT[/rel]/filename

    const std::vector<String>& curFiles() const; // current folder's files
    int  mappedIndex() const;           // file index from current _pos

    static bool isMusicFile(const char* name);
    static const char* baseName(const char* path);
};
