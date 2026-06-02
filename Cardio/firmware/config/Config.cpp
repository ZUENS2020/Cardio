#include "Config.h"
#include "DebugConsole.h"
#include <SD.h>

namespace {
constexpr char CONFIG_PATH[] = "/Cardio/config.txt";

// 去掉行内注释（首个 '#' 之后）并 trim 两端空白
String cleanValue(const String& raw) {
    String s = raw;
    int hash = s.indexOf('#');
    if (hash >= 0) s = s.substring(0, hash);
    s.trim();
    return s;
}
}  // namespace

Config& Config::instance() {
    static Config inst;
    return inst;
}

const char* Config::notifyModeName(NotifyMode m) {
    switch (m) {
        case NOTIFY_BLE:  return "ble";
        case NOTIFY_WIFI: return "wifi";
        default:          return "off";
    }
}

bool Config::parseBool(const String& s, bool& out) {
    if (s.equalsIgnoreCase("true")  || s == "1") { out = true;  return true; }
    if (s.equalsIgnoreCase("false") || s == "0") { out = false; return true; }
    return false;
}

void Config::applyDefaults() {
    _wifiSsid = ""; _wifiPass = "";
    _mqttHost = ""; _mqttPath = "/mqtt"; _mqttUser = ""; _mqttPass = "";
    _mqttPort = 443;
    _notifyMode = NOTIFY_OFF;
    _rssEnabled = false;
    _debugEnabled = false;
    _debugBle = false;
    _logLevel = Logger::INFO;
    _defaultVolume = 15;
    _defaultOrder = "sequential";
    _mute = false;
    _eq = "0,0,0,0,0";
}

bool Config::begin() {
    applyDefaults();

    File f = SD.open(CONFIG_PATH, FILE_READ);
    if (!f) {
        LOG_W("CFG", "%s not found, using defaults", CONFIG_PATH);
        return false;
    }

    int n = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;  // 空行/整行注释

        int eq = line.indexOf('=');
        if (eq < 0) continue;
        String key = line.substring(0, eq);   key.trim();
        String val = cleanValue(line.substring(eq + 1));
        if (key.length() == 0) continue;

        applyPair(key, val);
        n++;
    }
    f.close();

    LOG_I("CFG", "loaded %d keys: notify=%s rss=%d debug=%d vol=%d order=%s",
          n, notifyModeName(_notifyMode), _rssEnabled, _debugEnabled,
          _defaultVolume, _defaultOrder.c_str());
    return true;
}

void Config::applyPair(const String& key, const String& val) {
    if (!set(key, val)) {
        LOG_W("CFG", "unknown/invalid key skipped: %s=%s", key.c_str(), val.c_str());
    }
}

String Config::get(const String& k) const {
    if (k == "wifi_ssid")     return _wifiSsid;
    if (k == "wifi_pass")     return _wifiPass;
    if (k == "mqtt_host")     return _mqttHost;
    if (k == "mqtt_port")     return String(_mqttPort);
    if (k == "mqtt_path")     return _mqttPath;
    if (k == "mqtt_user")     return _mqttUser;
    if (k == "mqtt_pass")     return _mqttPass;
    if (k == "notify_mode")   return notifyModeName(_notifyMode);
    if (k == "rss_enabled")   return _rssEnabled ? "true" : "false";
    if (k == "debug_enabled") return _debugEnabled ? "true" : "false";
    if (k == "debug_ble")     return _debugBle ? "true" : "false";
    if (k == "log_level")     return Logger::levelName(_logLevel);
    if (k == "default_volume") return String(_defaultVolume);
    if (k == "default_order") return _defaultOrder;
    if (k == "mute")          return _mute ? "true" : "false";
    if (k == "eq")            return _eq;
    return "";
}

bool Config::set(const String& k, const String& v) {
    bool b;
    if (k == "wifi_ssid") { _wifiSsid = v; return true; }
    if (k == "wifi_pass") { _wifiPass = v; return true; }
    if (k == "mqtt_host") { _mqttHost = v; return true; }
    if (k == "mqtt_port") {
        int p = v.toInt();
        if (p <= 0 || p > 65535) return false;
        _mqttPort = p; return true;
    }
    if (k == "mqtt_path") { _mqttPath = v; return true; }
    if (k == "mqtt_user") { _mqttUser = v; return true; }
    if (k == "mqtt_pass") { _mqttPass = v; return true; }
    if (k == "notify_mode") {
        if (v == "off")  { _notifyMode = NOTIFY_OFF;  return true; }
        if (v == "ble")  { _notifyMode = NOTIFY_BLE;  return true; }
        if (v == "wifi") { _notifyMode = NOTIFY_WIFI; return true; }
        return false;
    }
    if (k == "rss_enabled")   { if (!parseBool(v, b)) return false; _rssEnabled = b;   return true; }
    if (k == "debug_enabled") { if (!parseBool(v, b)) return false; _debugEnabled = b; return true; }
    if (k == "debug_ble")     { if (!parseBool(v, b)) return false; _debugBle = b;     return true; }
    if (k == "log_level") {
        Logger::Level lv;
        if (!Logger::parseLevel(v.c_str(), lv)) return false;
        _logLevel = lv; return true;
    }
    if (k == "default_volume") {
        int vol = v.toInt();
        if (vol < 0 || vol > 21) return false;
        _defaultVolume = vol; return true;
    }
    if (k == "default_order") {
        if (v == "sequential" || v == "shuffle" || v == "repeat_one" ||
            v == "repeat_all" || v == "random") { _defaultOrder = v; return true; }
        return false;
    }
    if (k == "mute") { if (!parseBool(v, b)) return false; _mute = b; return true; }
    if (k == "eq")   { _eq = v; return true; }   // Equalizer parses/clamps the CSV defensively
    return false;
}

bool Config::save() {
    File f = SD.open(CONFIG_PATH, FILE_WRITE);   // 截断重写
    if (!f) {
        LOG_E("CFG", "save failed: cannot open %s", CONFIG_PATH);
        return false;
    }
    f.println("# WiFi");
    f.printf("wifi_ssid=%s\n", _wifiSsid.c_str());
    f.printf("wifi_pass=%s\n\n", _wifiPass.c_str());
    f.println("# MQTT");
    f.printf("mqtt_host=%s\n", _mqttHost.c_str());
    f.printf("mqtt_port=%d\n", _mqttPort);
    f.printf("mqtt_path=%s\n", _mqttPath.c_str());
    f.printf("mqtt_user=%s\n", _mqttUser.c_str());
    f.printf("mqtt_pass=%s\n\n", _mqttPass.c_str());
    f.println("# 功能开关");
    f.printf("notify_mode=%s\n", notifyModeName(_notifyMode));
    f.printf("rss_enabled=%s\n\n", _rssEnabled ? "true" : "false");
    f.println("# 调试");
    f.printf("debug_enabled=%s\n", _debugEnabled ? "true" : "false");
    f.printf("debug_ble=%s\n", _debugBle ? "true" : "false");
    // log_level 文件里用小写
    String lv = Logger::levelName(_logLevel); lv.trim(); lv.toLowerCase();
    f.printf("log_level=%s\n\n", lv.c_str());
    f.println("# 播放器");
    f.printf("default_volume=%d\n", _defaultVolume);
    f.printf("default_order=%s\n", _defaultOrder.c_str());
    f.printf("mute=%s\n", _mute ? "true" : "false");
    f.printf("eq=%s\n", _eq.c_str());
    f.close();
    LOG_I("CFG", "saved to %s", CONFIG_PATH);
    return true;
}

// ── 调试控制台命令 ────────────────────────────────────────────────────────
namespace {
void cmdConfig(int argc, char** argv, Print& out) {
    Config& cfg = Config::instance();
    if (argc >= 3 && !strcasecmp(argv[1], "get")) {
        out.printf("%s = %s\n", argv[2], cfg.get(argv[2]).c_str());
        return;
    }
    if (argc >= 4 && !strcasecmp(argv[1], "set")) {
        if (cfg.set(argv[2], argv[3])) out.printf("%s = %s (use 'config save' to persist)\n",
                                                  argv[2], cfg.get(argv[2]).c_str());
        else                           out.printf("invalid key/value: %s=%s\n", argv[2], argv[3]);
        return;
    }
    if (argc >= 2 && !strcasecmp(argv[1], "save"))   { cfg.save();  out.println("saved");  return; }
    if (argc >= 2 && !strcasecmp(argv[1], "reload")) { cfg.begin(); out.println("reloaded"); return; }
    out.println("usage: config get <key> | set <key> <val> | save | reload");
}
}  // namespace

void Config::registerConsole() {
    DebugConsole::instance().registerCommand(
        "config", "get <k> | set <k> <v> | save | reload", &cmdConfig);
}
