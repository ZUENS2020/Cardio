// Config — 读写 /Cardio/config.txt，键值对解析
//
// 启动时 begin() 读取并填充全局配置；值缺失时用内置默认值。
// 提供类型化 getter，以及面向调试控制台 / 未来 SettingsScreen / Web UI 的
// 通用 get(key) / set(key,val) 字符串接口。save() 以规范格式回写文件。
//
// 用法：
//   Config& cfg = Config::instance();
//   cfg.begin();
//   Logger::instance().setLevel(cfg.logLevel());
#pragma once

#include <Arduino.h>
#include "Logger.h"

class Config {
public:
    enum NotifyMode { NOTIFY_OFF = 0, NOTIFY_BLE, NOTIFY_WIFI };

    static Config& instance();

    // 读取 /Cardio/config.txt。文件缺失时全用默认值，返回 false。
    bool begin();
    // 以规范格式回写 /Cardio/config.txt（含分区注释）。
    bool save();

    // 把 "config" 命令注册到 DebugConsole（get/set/save/reload）
    void registerConsole();

    // ── 类型化 getter ────────────────────────────────────────────────────
    const String& wifiSsid()  const { return _wifiSsid; }
    const String& wifiPass()  const { return _wifiPass; }
    const String& mqttHost()  const { return _mqttHost; }
    int           mqttPort()  const { return _mqttPort; }
    const String& mqttPath()  const { return _mqttPath; }
    const String& mqttUser()  const { return _mqttUser; }
    const String& mqttPass()  const { return _mqttPass; }
    NotifyMode    notifyMode()    const { return _notifyMode; }
    bool          rssEnabled()    const { return _rssEnabled; }
    bool          debugEnabled()  const { return _debugEnabled; }
    bool          debugBle()      const { return _debugBle; }
    Logger::Level logLevel()      const { return _logLevel; }
    int           defaultVolume() const { return _defaultVolume; }
    const String& defaultOrder()  const { return _defaultOrder; }
    bool          muted()         const { return _mute; }  // boot silent (classroom)

    // ── 通用字符串接口（控制台 / 设置页）────────────────────────────────
    String get(const String& key) const;          // 未知键返回 ""
    bool   set(const String& key, const String& val);  // 未知键/非法值返回 false

    static const char* notifyModeName(NotifyMode m);

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void   applyDefaults();
    void   applyPair(const String& key, const String& val);
    static bool parseBool(const String& s, bool& out);

    String _wifiSsid, _wifiPass;
    String _mqttHost, _mqttPath, _mqttUser, _mqttPass;
    int    _mqttPort = 443;
    NotifyMode _notifyMode = NOTIFY_OFF;
    bool   _rssEnabled   = false;
    bool   _debugEnabled = false;
    bool   _debugBle     = false;
    Logger::Level _logLevel = Logger::INFO;
    int    _defaultVolume = 15;
    String _defaultOrder = "sequential";
    bool   _mute = false;
};
