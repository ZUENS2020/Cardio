// Cardio — Browser, overlays, settings, pairing, notices, boot

/* ---------------- small value widgets ---------------- */
function Toggle({ on }) { return <span className={"toggle " + (on ? "on" : "off")}>{on ? "ON" : "OFF"}</span>; }
function MiniSlider({ value }) {
  return (
    <span className="slider">
      <span className="track"><span className="fill" style={{ width: value + "%" }} /></span>
      <span className="val">{value}%</span>
    </span>
  );
}

/* ============================================================
   BROWSER — playlists / tracks
   ============================================================ */
function BrowserScreen({ mode, sel, plIdx, now }) {
  if (mode === "tracks") {
    const pl = PLAYLISTS[plIdx];
    return (
      <div className="screen">
        <StatusBar icon="list" title={<span className="cjk" style={{ fontWeight: 700 }}>{pl.name}</span>}
          right={pl.type === "rss" ? <span className="rss-tag">RSS</span> : <span style={{ fontSize: 10, color: "var(--fg-2)" }}>{pl.count}</span>} />
        <ScrollList items={pl.tracks} sel={sel} visible={5} render={(tk, idx, isSel) => {
          const isNow = now && now.plIdx === plIdx && now.idx === idx;
          return (
            <div className={"row" + (isSel ? " sel" : "")} key={idx}>
              {isNow ? <span className="nowbar" /> : <span className="idx">{String(idx + 1).padStart(2, "0")}</span>}
              <span className="grow" style={{ overflow: "hidden", textOverflow: "ellipsis" }}>{tk.t}</span>
              {tk.f === "RSS" && <span className="rss-tag">{Math.round(tk.d / 60)}m</span>}
              <span className="dur">{tk.f === "RSS" ? "" : fmtTime(tk.d)}</span>
            </div>
          );
      </div>
    );
  }

  // playlists
  return (
    <div className="screen">
      <StatusBar icon="folder" title="LIBRARY" cn="曲库"
        right={<><Icon name="sd" className="sm" style={{ color: "var(--cyan)" }} /><span style={{ fontSize: 10, color: "var(--fg-2)" }}>3.7G</span></>} />
      <ScrollList items={PLAYLISTS} sel={sel} visible={5} render={(p, idx, isSel) => (
        <div className={"row" + (isSel ? " sel" : "")} key={idx}>
          <Icon name={p.type === "rss" ? "signal" : "folder"} style={{ color: isSel ? "var(--accent-ink)" : p.type === "rss" ? "var(--cyan)" : "var(--fg-1)" }} />
          <span className="grow cjk">{p.name}</span>
          {p.type === "rss" && <span className="rss-tag">RSS</span>}
          <span className="dur">{p.count}</span>
          <Icon name="chevronRight" style={{ width: 11, height: 11, color: isSel ? "var(--accent-ink)" : "var(--fg-3)" }} />
        </div>
      )} />
    </div>
  );
}

/* ============================================================
   NOTIFY overlay — returns a node placed over the player
   ============================================================ */
function NotifyOverlay({ n, animate }) {
  return (
    <div className={"notify-ov" + (animate ? " slide" : "")}>
      <span className="nicon"><Icon name="bell" style={{ width: 12, height: 12 }} /></span>
      <div className="ntext">
        <div className="nsrc">
          <span className="app">{n.app}</span>
          {n.count > 1 && <span className="ncount">+{n.count}</span>}
          <span className="time">{n.time}</span>
        </div>
        <div className="nbody">{n.body}</div>
      </div>
    </div>
  );
}

/* ============================================================
   CALL — full-bleed, music paused
   ============================================================ */
function CallScreen({ call }) {
  return (
    <div className="call">
      <div className="ch">
        <span className="l"><Icon name="phone" style={{ width: 11, height: 11 }} />来电</span>
        <span style={{ fontSize: 10, color: "var(--err)", letterSpacing: ".06em" }}>INCOMING</span>
      </div>
      <div className="cmid">
        <span className="cring"><Icon name="phone" style={{ width: 16, height: 16 }} /></span>
        <span className="cname">{call.name}</span>
        <span className="cnum">{call.num}</span>
        <span className="paused">♪ 音乐已暂停 · MUSIC PAUSED</span>
      </div>
      <div className="cfoot">
        <span className="b dismiss"><Icon name="x" style={{ width: 11, height: 11, color: "var(--fg-2)" }} />挂断</span>
        <span className="b" style={{ color: "var(--ok)" }}><Icon name="check" style={{ width: 11, height: 11 }} />静音</span>
      </div>
    </div>
  );
}

/* ============================================================
   SETTINGS
   ============================================================ */
function SettingsScreen({ items, sel }) {
  return (
    <div className="screen">
      <StatusBar icon="gear" title="SETTINGS" cn="设置" right={<span style={{ fontSize: 10, color: "var(--fg-3)" }}>v1.0</span>} />
      <ScrollList items={items} sel={sel} visible={5} render={(it, idx, isSel) => {
        const ink = isSel ? "var(--accent-ink)" : null;
        return (
          <div className={"row" + (isSel ? " sel" : "")} key={it.id}>
            <span className="grow" style={{ color: ink || "var(--fg-0)" }}>
              {it.label} <span className="cjk" style={{ fontSize: 10, color: ink || "var(--fg-3)" }}>{it.cn}</span>
            </span>
            {it.type === "toggle" && <Toggle on={it.on} />}
            {it.type === "slider" && <MiniSlider value={it.val} />}
            {it.type === "select" && (
              <span style={{ fontSize: 11, color: ink || "var(--cyan)", display: "inline-flex", gap: 3 }}>
                ‹ {it.opts[it.val]} ›
              </span>
            )}
            {it.type === "action" && <Icon name="chevronRight" style={{ width: 11, height: 11, color: ink || "var(--fg-3)" }} />}
          </div>
        );
      }} />
    </div>
  );
}

/* ============================================================
   PAIRING — BLE passkey
   ============================================================ */
function PairingScreen({ pin, dev }) {
  return (
    <div className="screen">
      <StatusBar icon="signal" title="BLE PAIR" cn="配对" right={<span className="netchip" style={{ color: "var(--cyan)" }}>BLE</span>} />
      <div className="pair">
        <span className="plabel">PASSKEY · 配对码</span>
        <div className="pin">{pin.split("").map((c, i) => <span key={i}>{c}</span>)}</div>
        <span className="pdev">设备 <b>{dev}</b></span>
        <span className="phint">在手机上输入此配对码</span>
      </div>
    </div>
  );
}

/* ============================================================
   NOTICE — no SD / offline
   ============================================================ */
function NoticeScreen({ kind }) {
  const map = {
    nosd: { icon: "sd", color: "var(--err)", word: "NO SD CARD", sub: "未检测到存储卡\n请插入 microSD 后重启" },
    offline: { icon: "wifi", color: "var(--warn)", word: "OFFLINE", sub: "服务不可达 · 继续离线播放\nMQTT connect timeout" },
  };
  const m = map[kind];
  return (
    <div className="screen">
      <StatusBar icon={m.icon} title={kind === "nosd" ? "STORAGE" : "NETWORK"} cn={kind === "nosd" ? "存储" : "网络"}
        right={<span className="netchip" style={{ color: m.color }}>{kind === "nosd" ? "FAIL" : "WARN"}</span>} />
      <div className="notice">
        <span className="nbig" style={{ color: m.color }}><Icon name={m.icon} style={{ width: 24, height: 24 }} /></span>
        <span className="nword" style={{ color: m.color }}>{m.word}</span>
        <span className="nsub">{m.sub.split("\n").map((l, i) => <div key={i}>{l}</div>)}</span>
      </div>
    </div>
  );
}

/* ============================================================
   BOOT — animated splash
   ============================================================ */
const BOOT_LINES = [
  "ESP32-S3 · 240MHz · 8MB PSRAM",
  "mount /sd … OK",
  "ES8311 codec … OK",
  "load /Cardio/config.txt … OK",
  "audio engine @ core0 … OK",
  "ready",
];
function BootScreen({ progress, lineIdx }) {
  return (
    <div className="screen">
      <div className="boot">
        <img className="mark" src={(window.__resources && window.__resources.logo) || "assets/logo-mark.svg"} alt="" />
        <div className="wm">CARDIO</div>
        <div className="sub">MUSIC · NOTIFY</div>
        <div className="bbar"><div className="f" style={{ width: progress + "%" }} /></div>
        <div className="blines">
          {BOOT_LINES.slice(0, lineIdx).map((l, i) => (
            <div key={i}>{l.includes("OK") ? <>{l.replace(" OK", " ")}<span style={{ color: "var(--ok)" }}>OK</span></> : l}
              {i === lineIdx - 1 && <span className="boot-cur" />}</div>
          ))}
        </div>
        <div className="bver">v1.0.0</div>
      </div>
    </div>
  );
}

/* self-looping boot for the gallery */
function AnimatedBoot() {
  const [p, setP] = useState(0);
  const [li, setLi] = useState(0);
  useEffect(() => {
    let i = 0; const steps = 30;
    const t = setInterval(() => {
      i = (i + 1) % (steps + 8); // brief pause at full before looping
      const prog = Math.min(100, Math.round((i / steps) * 100));
      setP(prog);
      setLi(Math.min(BOOT_LINES.length, Math.floor((prog / 100) * BOOT_LINES.length) + (prog > 4 ? 1 : 0)));
    }, 110);
    return () => clearInterval(t);
  }, []);
  return <BootScreen progress={p} lineIdx={li} />;
}

Object.assign(window, { Toggle, MiniSlider, BrowserScreen, NotifyOverlay, CallScreen, SettingsScreen, PairingScreen, NoticeScreen, BootScreen, AnimatedBoot, BOOT_LINES });
