// DebugConsole — 串口调试控制台（BLE NUS 通道在 Week 1 Day 6 接入）
//
// 在主 loop() 中调用 update()，非阻塞读取 Serial 行，解析并分发命令。
// 其他模块通过 registerCommand() 注册自己的命令，随功能逐步扩充。
//
// 用法：
//   DebugConsole::instance().begin();
//   DebugConsole::instance().registerCommand("play", "play <路径>", &onPlay);
//   // loop:
//   DebugConsole::instance().update();
#pragma once

#include <Arduino.h>

class DebugConsole {
public:
    // argc/argv 已按空格切分，argv[0] 为命令名；out 为输出通道（Serial 或 BLE）
    using Handler = void (*)(int argc, char** argv, Print& out);

    static DebugConsole& instance();

    void begin();
    void update();   // 主循环轮询，非阻塞

    // help 文本可为 nullptr。重名注册会覆盖旧的。
    bool registerCommand(const char* name, const char* help, Handler fn);

    // 供 BLE NUS 通道复用：直接喂一整行命令
    void execute(const char* line, Print& out);

    static constexpr int MAX_COMMANDS = 32;
    static constexpr int MAX_ARGS     = 8;
    static constexpr int LINE_CAP     = 160;

private:
    DebugConsole() = default;
    DebugConsole(const DebugConsole&) = delete;
    DebugConsole& operator=(const DebugConsole&) = delete;

    void registerBuiltins();
    void printHelp(Print& out);

    struct Command {
        const char* name = nullptr;
        const char* help = nullptr;
        Handler     fn   = nullptr;
    };
    Command _cmds[MAX_COMMANDS];
    int     _count = 0;

    char _buf[LINE_CAP];
    int  _len = 0;

    friend struct ConsoleBuiltins;   // 内置命令访问命令表打印 help
};
