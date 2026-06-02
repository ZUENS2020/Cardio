// Cardio — review page: live device + static gallery of every screen state

function Art({ name, cn, tag, desc, s = 2, children }) {
  return (
    <div className="art">
      <div className="cap">
        <span className="n">{name}</span>
        {cn && <span className="cn">{cn}</span>}
        {tag && <span className="tag">{tag}</span>}
      </div>
      <div className="vp" style={{ "--s": s }}>{children}</div>
      {desc && <div className="desc" style={{ "--s": s }}>{desc}</div>}
    </div>
  );
}

const stPlay = { pl: PLAYLISTS[0], idx: 0, playing: true, vol: 15, orderIdx: 0, pos: 83, mode: "ble", battery: 72, clock: "14:25" };
const stPause = { pl: PLAYLISTS[2], idx: 0, playing: false, vol: 9, orderIdx: 2, pos: 1240, mode: "wifi", battery: 54, clock: "22:10" };

function App() {
  return (
    <div className="page">
      <div className="page-head">
        <p className="kick">Cardio · Firmware UI</p>
        <h1>Cardputer ADV — 音乐播放器 + 通知推送</h1>
        <p>
          A complete on-device interface for <b style={{ color: "var(--fg-1)" }}>Cardio</b> — the M5Stack Cardputer ADV
          music player with cross-platform notification push. Authored pixel-exact at the device's true
          240×135 RGB565 resolution, keyboard-only, in the Cardputer ADV design language. The device below is
          live — click it and drive it with the keyboard. Every screen state is catalogued underneath.
        </p>
        <div className="specs">
          <span className="spec">240×135 · <b>IPS LCD</b></span>
          <span className="spec">ESP32-S3 · <b>240MHz</b></span>
          <span className="spec">输入 · <b>56-key</b></span>
          <span className="spec">音频 · <b>ES8311 / FLAC</b></span>
          <span className="spec">推送 · <b>BLE / MQTT</b></span>
        </div>
      </div>

      <CardioDevice />

      <div className="section">
        <h2>全部界面 <span className="cn">· Every screen</span></h2>
        <p className="lead">
          The full screen set from <code style={{ color: "var(--cyan)" }}>ARCHITECTURE.md</code> — boot, player,
          library, the notification state machine (IDLE → NOTIFY → CALL), settings, BLE pairing, and the failure notices.
        </p>

        <div className="gallery">
          <Art name="BOOT" cn="开机" tag="animated" desc="Logo + boot log streaming in. splash.jpg is user-customizable.">
            <AnimatedBoot />
          </Art>
          <Art name="PLAYER" cn="播放器" tag="home" desc="Cover · title · live visualizer · progress · order tag + volume.">
            <PlayerScreen st={stPlay} />
          </Art>
          <Art name="PLAYER · paused" cn="暂停" desc="RSS episode, repeat order, WiFi push. Visualizer freezes to grey when paused.">
            <PlayerScreen st={stPause} />
          </Art>
          <Art name="LIBRARY" cn="曲库" tag="browser" desc="Local playlists + RSS feeds on the SD card. Cursor fills orange.">
            <BrowserScreen mode="lists" sel={2} now={{ plIdx: 0, idx: 0 }} />
          </Art>
          <Art name="TRACKS" cn="曲目" tag="browser" desc="Inside a playlist. The now-playing row shows a 2px orange bar instead of an index.">
            <BrowserScreen mode="tracks" sel={1} plIdx={0} now={{ plIdx: 0, idx: 0 }} />
          </Art>
          <Art name="NOTIFY" cn="通知" tag="overlay" desc="Top banner slides in over the player. Music keeps playing; auto-dismiss after 5s.">
            <PlayerScreen st={stPlay} overlay={<NotifyOverlay n={NOTIFS[0]} />} />
          </Art>
          <Art name="INCOMING CALL" cn="来电" tag="full-bleed" desc="priority=high · source=来电. Full-screen, music pauses, red pulse ring.">
            <div className="screen"><CallScreen call={CALLS[0]} /></div>
          </Art>
          <Art name="SETTINGS" cn="设置" desc="Toggles, sliders, selects. notify_mode = off / BLE / WiFi. Square 2-state, never an iOS switch.">
            <SettingsScreen items={INIT_SETTINGS} sel={0} />
          </Art>
          <Art name="BLE PAIRING" cn="配对" tag="security" desc="6-digit passkey shown on-screen for LE Secure Connections. Enter it on the phone.">
            <PairingScreen pin="042913" dev="Cardio-7F3A" />
          </Art>
          <Art name="NO SD CARD" cn="无存储" tag="error" desc="Hardware-state failure. Terminal register: glyph + colour + uppercase word.">
            <NoticeScreen kind="nosd" />
          </Art>
          <Art name="OFFLINE" cn="离线" tag="notice" desc="Application-layer unreachable (MQTT timeout). Keeps playing offline.">
            <NoticeScreen kind="offline" />
          </Art>
        </div>
      </div>
    </div>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);
