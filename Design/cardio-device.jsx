// Cardio — interactive device (state machine) + debug console
// React hooks (useState/useEffect/useRef/useCallback) come from cardio-primitives.jsx global scope.

const INIT_SETTINGS = [
  { id: "notify", label: "Notify", cn: "推送", type: "select", val: 1, opts: ["off", "BLE", "WiFi"] },
  { id: "rss", label: "RSS", cn: "订阅", type: "toggle", on: true },
  { id: "vol", label: "Volume", cn: "音量", type: "slider", val: 71 },
  { id: "bright", label: "Bright", cn: "亮度", type: "slider", val: 80 },
  { id: "keysnd", label: "Key tone", cn: "按键音", type: "toggle", on: false },
  { id: "theme", label: "Accent", cn: "主题", type: "select", val: 0, opts: ["Orange", "Amber", "Cyan"] },
  { id: "pair", label: "BLE pair", cn: "配对设备", type: "action" },
  { id: "bond", label: "Clear bond", cn: "清除配对", type: "action" },
];
const MODES = ["off", "ble", "wifi"];

function CardioDevice() {
  const [armed, setArmed] = useState(false);
  const [screen, setScreen] = useState("boot");      // boot|player|lists|tracks|settings|pairing
  const [boot, setBoot] = useState({ p: 0, li: 0 });

  // player
  const [plIdx, setPlIdx] = useState(0);
  const [idx, setIdx] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [vol, setVol] = useState(15);
  const [orderIdx, setOrderIdx] = useState(0);
  const [pos, setPos] = useState(0);
  const [mode, setMode] = useState("ble");

  // browser
  const [listSel, setListSel] = useState(0);
  const [trackSel, setTrackSel] = useState(0);
  const [browsePl, setBrowsePl] = useState(0);

  // settings
  const [setItems, setSetItems] = useState(() => INIT_SETTINGS.map(x => ({ ...x })));
  const [setSel, setSetSel] = useState(0);
  const [pin, setPin] = useState("042913");

  // events
  const [overlay, setOverlay] = useState(null);
  const [call, setCall] = useState(null);
  const wasPlaying = useRef(false);
  const ovTimer = useRef(null);
  const notifPtr = useRef(0);
  const callPtr = useRef(0);

  const pl = PLAYLISTS[plIdx];
  const track = pl.tracks[idx];

  /* ---- boot sequence (timer-based: robust when backgrounded) ---- */
  const runBoot = useCallback(() => {
    setScreen("boot"); setBoot({ p: 0, li: 0 });
    const steps = 26, iv = 100;
    let i = 0;
    const t = setInterval(() => {
      i++;
      const p = Math.min(100, Math.round((i / steps) * 100));
      setBoot({ p, li: Math.min(BOOT_LINES.length, Math.floor((p / 100) * BOOT_LINES.length) + (p > 4 ? 1 : 0)) });
      if (i >= steps) { clearInterval(t); setScreen("player"); setPlaying(true); }
    }, iv);
    return t;
  }, []);
  useEffect(() => { const t = runBoot(); return () => clearInterval(t); }, [runBoot]);

  /* ---- playback timer (music advances regardless of screen) ---- */
  useEffect(() => {
    if (!playing) return;
    const t = setInterval(() => {
      setPos(p => {
        if (p + 1 >= track.d) { advance(1); return 0; }
        return p + 1;
      });
    }, 1000);
    return () => clearInterval(t);
  }, [playing, track, plIdx, idx]);

  const advance = useCallback((dir) => {
    setIdx(i => {
      const n = pl.tracks.length;
      if (orderIdx === 1) return Math.floor(Math.random() * n);  // shuffle
      if (orderIdx === 3) return i;                              // repeat-one
      return (i + dir + n) % n;
    });
    setPos(0);
  }, [pl, orderIdx]);

  /* ---- notify / call triggers ---- */
  const fireNotify = useCallback(() => {
    const n = NOTIFS[notifPtr.current % NOTIFS.length]; notifPtr.current++;
    setScreen("player"); setOverlay(n);
    if (ovTimer.current) clearTimeout(ovTimer.current);
    ovTimer.current = setTimeout(() => setOverlay(null), 5000);
  }, []);
  const fireCall = useCallback(() => {
    const c = CALLS[callPtr.current % CALLS.length]; callPtr.current++;
    wasPlaying.current = playing; setPlaying(false); setCall(c);
  }, [playing]);
  const endCall = useCallback(() => {
    setCall(null); if (wasPlaying.current) setPlaying(true);
  }, []);
  const reboot = useCallback(() => {
    setOverlay(null); setCall(null); setPos(0); runBoot();
  }, [runBoot]);

  /* ---- settings adjust ---- */
  const adjustSetting = useCallback((dir) => {
    setSetItems(prev => prev.map((it, i) => {
      if (i !== setSel) return it;
      if (it.type === "toggle") return { ...it, on: dir === 0 ? !it.on : dir > 0 };
      if (it.type === "slider") return { ...it, val: Math.max(0, Math.min(100, it.val + (dir > 0 ? 5 : -5))) };
      if (it.type === "select") {
        const nv = (it.val + (dir > 0 ? 1 : it.opts.length - 1)) % it.opts.length;
        if (it.id === "notify") setMode(MODES[nv]);
        return { ...it, val: nv };
      }
      return it;
    }));
  }, [setSel]);

  const doAction = useCallback(() => {
    const it = setItems[setSel];
    if (it.type === "action" && it.id === "pair") {
      setPin(String(Math.floor(Math.random() * 900000) + 100000));
      setScreen("pairing");
    } else if (it.type !== "action") adjustSetting(0);
  }, [setItems, setSel, adjustSetting]);

  /* ---- keyboard ---- */
  const onKey = useCallback((e) => {
    if (!armed) return;
    const k = e.key;
    const nav = ["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", " ", "Enter", "Escape"];
    if (nav.includes(k)) e.preventDefault();

    // global event shortcuts
    if (k === "n" || k === "N") { fireNotify(); return; }
    if (k === "c" || k === "C") { fireCall(); return; }

    if (call) { if (k === "Escape" || k === "Enter") endCall(); return; }
    if (screen === "boot") return;

    if (screen === "pairing") { if (k === "Escape") setScreen("settings"); return; }

    if (screen === "player") {
      if (k === " ") setPlaying(p => !p);
      else if (k === "ArrowLeft") advance(-1);
      else if (k === "ArrowRight") advance(1);
      else if (k === "ArrowUp") setVol(v => Math.min(21, v + 1));
      else if (k === "ArrowDown") setVol(v => Math.max(0, v - 1));
      else if (k === "o" || k === "O") setOrderIdx(o => (o + 1) % ORDERS.length);
      else if (k === "Enter" || k === "b" || k === "B") { setScreen("lists"); }
      else if (k === "s" || k === "S") { setScreen("settings"); }
      return;
    }
    if (screen === "lists") {
      if (k === "ArrowUp") setListSel(s => Math.max(0, s - 1));
      else if (k === "ArrowDown") setListSel(s => Math.min(PLAYLISTS.length - 1, s + 1));
      else if (k === "Enter") { setBrowsePl(listSel); setTrackSel(0); setScreen("tracks"); }
      else if (k === "Escape") setScreen("player");
      return;
    }
    if (screen === "tracks") {
      const n = PLAYLISTS[browsePl].tracks.length;
      if (k === "ArrowUp") setTrackSel(s => Math.max(0, s - 1));
      else if (k === "ArrowDown") setTrackSel(s => Math.min(n - 1, s + 1));
      else if (k === "Enter") { setPlIdx(browsePl); setIdx(trackSel); setPos(0); setPlaying(true); setScreen("player"); }
      else if (k === "Escape") setScreen("lists");
      return;
    }
    if (screen === "settings") {
      if (k === "ArrowUp") setSetSel(s => Math.max(0, s - 1));
      else if (k === "ArrowDown") setSetSel(s => Math.min(setItems.length - 1, s + 1));
      else if (k === "ArrowRight") adjustSetting(1);
      else if (k === "ArrowLeft") adjustSetting(-1);
      else if (k === "Enter") doAction();
      else if (k === "Escape") setScreen("player");
      return;
    }
  }, [armed, screen, call, listSel, trackSel, browsePl, setItems, setSel, advance, fireNotify, fireCall, endCall, doAction, adjustSetting]);

  useEffect(() => {
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [onKey]);

  // click-to-arm
  useEffect(() => {
    const off = (e) => { if (!e.target.closest(".live .vp")) setArmed(false); };
    window.addEventListener("mousedown", off);
    return () => window.removeEventListener("mousedown", off);
  }, []);

  const st = { pl, idx, playing, vol, orderIdx, pos, mode, battery: 72, clock: "14:25" };

  let body;
  if (call) body = <div className="screen"><CallScreen call={call} /></div>;
  else if (screen === "boot") body = <BootScreen progress={boot.p} lineIdx={boot.li} />;
  else if (screen === "player") body = <PlayerScreen st={st} overlay={overlay && <NotifyOverlay n={overlay} animate />} />;
  else if (screen === "lists") body = <BrowserScreen mode="lists" sel={listSel} now={{ plIdx, idx }} />;
  else if (screen === "tracks") body = <BrowserScreen mode="tracks" sel={trackSel} plIdx={browsePl} now={{ plIdx, idx }} />;
  else if (screen === "settings") body = <SettingsScreen items={setItems} sel={setSel} />;
  else if (screen === "pairing") body = <PairingScreen pin={pin} dev="Cardio-7F3A" />;

  return (
    <div className="live">
      <div className="stagewrap">
        <div className={"vp" + (armed ? " armed" : "")} onMouseDown={() => setArmed(true)} tabIndex={0}>
          {body}
        </div>
      </div>

      <div className="panel">
        <div className="console">
          <div className="lbl">debug console · 模拟事件</div>
          <div className="cmds">
            <span className="cmd" onClick={fireNotify}><span className="pmt">$</span> notify <span className="cn">通知</span></span>
            <span className="cmd" onClick={fireCall}><span className="pmt">$</span> call <span className="cn">来电</span></span>
            <span className="cmd" onClick={reboot}><span className="pmt">$</span> reboot <span className="cn">重启</span></span>
          </div>
          <div className="lbl" style={{ marginTop: 10, color: "var(--fg-3)" }}>
            also: press <b style={{ color: "var(--fg-1)" }}>N</b> notify · <b style={{ color: "var(--fg-1)" }}>C</b> call
          </div>
        </div>
      </div>
    </div>
  );
}

window.CardioDevice = CardioDevice;
