#include "DebugConsole.h"
#include "Logger.h"
#include <Arduino.h>

DebugConsole& DebugConsole::instance() {
    static DebugConsole inst;
    return inst;
}

// ── 内置命令（friend，可访问私有 printHelp）──────────────────────────────
struct ConsoleBuiltins {
    static void help(int, char**, Print& out) {
        DebugConsole::instance().printHelp(out);
    }

    static void heap(int, char**, Print& out) {
        out.printf("SRAM  free=%u  min=%u  largest=%u\n",
                   (unsigned)ESP.getFreeHeap(),
                   (unsigned)ESP.getMinFreeHeap(),
                   (unsigned)ESP.getMaxAllocHeap());
        out.printf("PSRAM free=%u  total=%u\n",
                   (unsigned)ESP.getFreePsram(),
                   (unsigned)ESP.getPsramSize());
    }

    static void log(int argc, char** argv, Print& out) {
        if (argc >= 3 && !strcasecmp(argv[1], "level")) {
            Logger::Level lv;
            if (Logger::parseLevel(argv[2], lv)) {
                Logger::instance().setLevel(lv);
                out.printf("log level = %s\n", Logger::levelName(lv));
            } else {
                out.println("usage: log level <debug|info|warn|error>");
            }
            return;
        }
        if (argc >= 2 && !strcasecmp(argv[1], "dump")) {
            Logger::instance().dumpTo(out);
            return;
        }
        if (argc >= 2 && !strcasecmp(argv[1], "clear")) {
            Logger::instance().clearFile();
            out.println("log cleared");
            return;
        }
        out.println("usage: log level <lvl> | log dump | log clear");
    }

    static void reboot(int, char**, Print& out) {
        out.println("rebooting...");
        out.flush();
        delay(100);
        ESP.restart();
    }
};

void DebugConsole::begin() {
    _len = 0;
    registerBuiltins();
    Serial.println();
    Serial.println("=== Cardio Debug Console ===");
    printHelp(Serial);
    Serial.print("> ");
}

void DebugConsole::registerBuiltins() {
    registerCommand("help",   "this list",                       &ConsoleBuiltins::help);
    registerCommand("heap",   "show SRAM/PSRAM usage",           &ConsoleBuiltins::heap);
    registerCommand("log",    "log level <lvl> | dump | clear",  &ConsoleBuiltins::log);
    registerCommand("reboot", "restart device",                  &ConsoleBuiltins::reboot);
}

bool DebugConsole::registerCommand(const char* name, const char* help, Handler fn) {
    if (!name || !fn) return false;
    for (int i = 0; i < _count; ++i) {        // 覆盖重名
        if (!strcasecmp(_cmds[i].name, name)) {
            _cmds[i].help = help;
            _cmds[i].fn   = fn;
            return true;
        }
    }
    if (_count >= MAX_COMMANDS) return false;
    _cmds[_count].name = name;
    _cmds[_count].help = help;
    _cmds[_count].fn   = fn;
    _count++;
    return true;
}

void DebugConsole::printHelp(Print& out) {
    out.println("commands:");
    for (int i = 0; i < _count; ++i) {
        out.printf("  %-8s %s\n", _cmds[i].name,
                   _cmds[i].help ? _cmds[i].help : "");
    }
}

void DebugConsole::update() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            _buf[_len] = '\0';
            if (_len > 0) {
                Serial.println();              // 回显换行
                execute(_buf, Serial);
            }
            _len = 0;
            Serial.print("> ");
            continue;
        }
        if (c == 8 || c == 127) {              // 退格
            if (_len > 0) { _len--; Serial.print("\b \b"); }
            continue;
        }
        if (_len < LINE_CAP - 1) {
            _buf[_len++] = c;
            Serial.print(c);                   // 回显
        }
    }
}

void DebugConsole::execute(const char* line, Print& out) {
    char work[LINE_CAP];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char* argv[MAX_ARGS];
    int argc = 0;
    char* save = nullptr;
    for (char* tok = strtok_r(work, " \t", &save);
         tok && argc < MAX_ARGS;
         tok = strtok_r(nullptr, " \t", &save)) {
        argv[argc++] = tok;
    }
    if (argc == 0) return;

    for (int i = 0; i < _count; ++i) {
        if (!strcasecmp(_cmds[i].name, argv[0])) {
            _cmds[i].fn(argc, argv, out);
            return;
        }
    }
    out.printf("unknown command: %s (try 'help')\n", argv[0]);
}
