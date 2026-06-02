#include "PlaybackController.h"
#include "Config.h"
#include "Logger.h"
#include "DebugConsole.h"
#include <SD.h>
#include <algorithm>

static constexpr const char* MUSIC_ROOT = "/Cardio/music";

static const char* kExts[] = {
    ".mp3", ".flac", ".wav", ".MP3", ".FLAC", ".WAV", nullptr
};

static theme::Order orderFromString(const char* s) {
    if (!s) return theme::SEQ;
    if (!strcasecmp(s, "shuffle")    || !strcasecmp(s, "random"))     return theme::SHF;
    if (!strcasecmp(s, "repeat_all") || !strcasecmp(s, "repeat"))     return theme::RPT;
    if (!strcasecmp(s, "repeat_one"))                                  return theme::RPT1;
    return theme::SEQ;
}

static void sortFiles(std::vector<String>& v) {
    std::sort(v.begin(), v.end(), [](const String& a, const String& b) {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    });
}

// ── Singleton ─────────────────────────────────────────────────────────────────

PlaybackController& PlaybackController::instance() {
    static PlaybackController inst;
    return inst;
}

// ── begin ────────────────────────────────────────────────────────────────────

void PlaybackController::begin() {
    _order = orderFromString(Config::instance().defaultOrder().c_str());
    scanLibrary();
    if (!_lib.empty()) {
        _folderIdx = 0;
        buildMap();
    }
    LOG_I("PC", "order=%u", (uint8_t)_order);
}

// ── update — pump audio + auto-advance ────────────────────────────────────────

void PlaybackController::update() {
    if (_ae.update()) onTrackEnd();
}

void PlaybackController::onTrackEnd() {
    if (advancePos()) playCurrent();
}

// ── Library scanning (once, at begin) ─────────────────────────────────────────

void PlaybackController::scanLibrary() {
    _lib.clear();

    File root = SD.open(MUSIC_ROOT);
    if (!root || !root.isDirectory()) {
        LOG_W("PC", "%s not found", MUSIC_ROOT);
        return;
    }

    Folder loose;            // synthetic folder for loose files in the root
    loose.name = "*";
    loose.rel  = "";

    std::vector<Folder> subs;

    File entry;
    while ((entry = root.openNextFile())) {
        const char* bname = baseName(entry.name());
        if (entry.isDirectory()) {
            if (bname[0] != '.') {
                Folder fld;
                fld.name = bname;
                fld.rel  = bname;
                scanFolderFiles(fld);
                if (!fld.files.empty()) {
                    sortFiles(fld.files);
                    subs.push_back(std::move(fld));
                }
            }
        } else if (isMusicFile(bname)) {
            loose.files.push_back(String(bname));
        }
        entry.close();
    }
    root.close();

    if (!loose.files.empty()) {
        sortFiles(loose.files);
        _lib.push_back(std::move(loose));
    }
    for (auto& s : subs) _lib.push_back(std::move(s));

    int total = 0;
    for (auto& f : _lib) total += (int)f.files.size();
    LOG_I("PC", "library: %d folders, %d tracks", (int)_lib.size(), total);
}

void PlaybackController::scanFolderFiles(Folder& fld) {
    String path = String(MUSIC_ROOT) + "/" + fld.rel;
    File dir = SD.open(path.c_str());
    if (!dir) { LOG_E("PC", "cannot open %s", path.c_str()); return; }

    File f;
    while ((f = dir.openNextFile())) {
        if (!f.isDirectory()) {
            const char* bn = baseName(f.name());
            if (isMusicFile(bn)) fld.files.push_back(String(bn));
        }
        f.close();
    }
    dir.close();
}

// ── Folder selection (no SD access — index is in RAM) ──────────────────────────

bool PlaybackController::selectFolder(int idx) {
    if (idx < 0 || idx >= (int)_lib.size()) return false;
    _ae.stop();
    _folderIdx = idx;
    _pos = -1;
    buildMap();
    LOG_I("PC", "folder '%s': %d tracks",
          _lib[idx].name.c_str(), (int)_lib[idx].files.size());
    return true;
}

// ── Play-order map ────────────────────────────────────────────────────────────

void PlaybackController::buildMap() {
    size_t n = curFiles().size();
    _map.resize(n);
    for (size_t i = 0; i < n; ++i) _map[i] = (int)i;
    if (_order == theme::SHF) {
        // Fisher-Yates shuffle
        for (size_t i = n; i > 1; --i) {
            size_t j = esp_random() % i;
            std::swap(_map[i-1], _map[j]);
        }
    }
}

void PlaybackController::setOrder(theme::Order o) {
    _order = o;
    buildMap();
    LOG_I("PC", "order=%u", (uint8_t)o);
}

void PlaybackController::cycleOrder() {
    setOrder(static_cast<theme::Order>((_order + 1) % 4));
}

// ── Transport ─────────────────────────────────────────────────────────────────

bool PlaybackController::play() {
    if (curFiles().empty()) return false;
    if (_pos < 0) _pos = 0;
    playCurrent();
    return true;
}

bool PlaybackController::next() {
    if (curFiles().empty()) return false;
    if (!advancePos()) return false;
    playCurrent();
    return true;
}

bool PlaybackController::prev() {
    if (curFiles().empty()) return false;
    if (_pos <= 0) _pos = 0;
    else           --_pos;
    playCurrent();
    return true;
}

void PlaybackController::stop() {
    _ae.stop();
}

void PlaybackController::selectTrack(int idx) {
    if (idx < 0 || idx >= (int)curFiles().size()) return;
    // Sync _pos to this track's slot in the order map so next/prev continue
    // from here instead of jumping back to the top of the playlist.
    for (size_t i = 0; i < _map.size(); ++i)
        if (_map[i] == idx) { _pos = (int)i; break; }
    _ae.play(fullPath(idx).c_str());
}

// ── Internal ──────────────────────────────────────────────────────────────────

void PlaybackController::playCurrent() {
    if (_pos < 0 || _pos >= (int)_map.size()) return;
    _ae.play(fullPath(_map[_pos]).c_str());
}

// Advance _pos according to the play order.  Returns false only when a
// SEQ-mode playlist has run off the end (caller stops); every other order
// always yields a next track.
bool PlaybackController::advancePos() {
    if (_map.empty()) return false;
    if (_pos < 0) { _pos = 0; return true; }

    switch (_order) {
        case theme::RPT1:
            break; // stay at the same position
        case theme::SEQ:
            if (_pos + 1 >= (int)_map.size()) return false; // end of playlist
            ++_pos;
            break;
        case theme::RPT:
            _pos = (_pos + 1) % (int)_map.size();
            break;
        case theme::SHF:
            ++_pos;
            if (_pos >= (int)_map.size()) { buildMap(); _pos = 0; } // reshuffle
            break;
    }
    return true;
}

String PlaybackController::fullPath(int fileIdx) const {
    if (_folderIdx < 0 || _folderIdx >= (int)_lib.size()) return String();
    const Folder& f = _lib[_folderIdx];
    if (fileIdx < 0 || fileIdx >= (int)f.files.size()) return String();
    String p = MUSIC_ROOT;
    if (f.rel.length()) { p += '/'; p += f.rel; }
    p += '/'; p += f.files[fileIdx];
    return p;
}

const std::vector<String>& PlaybackController::curFiles() const {
    static const std::vector<String> kEmpty;
    if (_folderIdx < 0 || _folderIdx >= (int)_lib.size()) return kEmpty;
    return _lib[_folderIdx].files;
}

int PlaybackController::mappedIndex() const {
    if (_pos < 0 || _pos >= (int)_map.size()) return -1;
    return _map[_pos];
}

// ── Info ──────────────────────────────────────────────────────────────────────

int PlaybackController::folderCount() const { return (int)_lib.size(); }

const char* PlaybackController::folderName(int idx) const {
    if (idx < 0 || idx >= (int)_lib.size()) return nullptr;
    return _lib[idx].name.c_str();
}

int PlaybackController::totalTracks() const { return (int)curFiles().size(); }

const char* PlaybackController::trackName(int idx) const {
    const auto& files = curFiles();
    if (idx < 0 || idx >= (int)files.size()) return nullptr;
    static char buf[128];
    strlcpy(buf, files[idx].c_str(), sizeof(buf));
    char* dot = strrchr(buf, '.');
    if (dot) *dot = '\0';
    return buf;
}

const char* PlaybackController::trackPath(int idx) const {
    if (idx < 0 || idx >= (int)curFiles().size()) return nullptr;
    static String buf;
    buf = fullPath(idx);
    return buf.c_str();
}

const char* PlaybackController::currentTrackPath() const {
    int mi = mappedIndex();
    if (mi < 0) return nullptr;
    static String buf;
    buf = fullPath(mi);
    return buf.c_str();
}

int PlaybackController::currentTrackIndex() const { return mappedIndex(); }

const char* PlaybackController::currentFolderName() const {
    if (_folderIdx < 0 || _folderIdx >= (int)_lib.size()) return nullptr;
    return _lib[_folderIdx].name.c_str();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

bool PlaybackController::isMusicFile(const char* name) {
    if (!name) return false;
    // Skip dotfiles: macOS writes a "._<name>" AppleDouble companion for every
    // real file on FAT/exFAT cards (plus .DS_Store, .Spotlight-V100, …).  They
    // share the audio extension but are not decodable — including them doubles
    // the track count and decode-fails instantly, cascading the auto-advance.
    if (name[0] == '.') return false;
    const char* dot = strrchr(name, '.');
    if (!dot) return false;
    for (int i = 0; kExts[i]; ++i)
        if (strcasecmp(dot, kExts[i]) == 0) return true;
    return false;
}

const char* PlaybackController::baseName(const char* path) {
    if (!path) return "";
    const char* sl = strrchr(path, '/');
    return sl ? sl + 1 : path;
}

// ── DebugConsole ──────────────────────────────────────────────────────────────

void PlaybackController::registerConsole() {
    auto& con = DebugConsole::instance();
    con.registerCommand("list", "list folders", [](int,char**,Print& out) {
        auto& pc = PlaybackController::instance();
        for (int i = 0; i < pc.folderCount(); ++i)
            out.printf("  %d: %s\n", i, pc.folderName(i));
    });
    con.registerCommand("playlist", "playlist <n>", [](int argc, char** argv, Print& out) {
        if (argc < 2) { out.println("usage: playlist <n>"); return; }
        auto& pc = PlaybackController::instance();
        if (!pc.selectFolder(atoi(argv[1]))) out.println("failed");
        else out.printf("loaded %s (%d tracks)\n",
            pc.currentFolderName(), pc.totalTracks());
    });
    con.registerCommand("tracks", "list tracks", [](int,char**,Print& out) {
        auto& pc = PlaybackController::instance();
        out.printf("folder:%s tracks:%d now:%d\n",
            pc.currentFolderName() ? pc.currentFolderName() : "-",
            pc.totalTracks(), pc.currentTrackIndex());
    });
    con.registerCommand("order", "order [seq|shf|rpt|r1]", [](int argc, char** argv, Print& out) {
        auto& pc = PlaybackController::instance();
        if (argc >= 2) {
            const char* s = argv[1];
            if      (!strcasecmp(s,"seq") || !strcasecmp(s,"sequential")) pc.setOrder(theme::SEQ);
            else if (!strcasecmp(s,"shf") || !strcasecmp(s,"shuffle"))    pc.setOrder(theme::SHF);
            else if (!strcasecmp(s,"rpt") || !strcasecmp(s,"repeat_all")) pc.setOrder(theme::RPT);
            else if (!strcasecmp(s,"r1")  || !strcasecmp(s,"repeat_one")) pc.setOrder(theme::RPT1);
            else { out.println("unknown order"); return; }
        }
        const char* names[] = {"SEQ","SHF","RPT","R1"};
        out.printf("order=%s\n", names[(uint8_t)pc.order() & 3]);
    });
    con.registerCommand("next", "next track", [](int,char**,Print&){ PlaybackController::instance().next(); });
    con.registerCommand("prev", "prev track", [](int,char**,Print&){ PlaybackController::instance().prev(); });
}
