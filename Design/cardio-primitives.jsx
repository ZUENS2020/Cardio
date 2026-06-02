// Cardio — shared primitives + data model
const { useState, useEffect, useRef, useCallback, useMemo } = React;

/* ---------------- icon ---------------- */
function Icon({ name, className = "", style }) {
  return <svg className={"ic " + className} style={style} aria-hidden="true"><use href={"#ic-" + name} /></svg>;
}

/* ---------------- status bar ----------------
   left: title ; right: connection + battery% + clock (text, icon-light) */
function StatusBar({ icon, title, cn, right, battery = 72, charging = false, clock = "14:25" }) {
  const battColor = battery <= 15 ? "var(--err)" : battery <= 35 ? "var(--warn)" : "var(--fg-2)";
  return (
    <div className="statusbar">
      <div className="title">
        <span>{title}</span>
        {cn && <span className="cjk" style={{ fontWeight: 400, color: "var(--fg-2)", fontSize: 10 }}>{cn}</span>}
      </div>
      <div className="ind">
        {right}
        <span style={{ fontSize: 11, color: battColor }}>{battery}%</span>
        <span className="clock">{clock}</span>
      </div>
    </div>
  );
}

/* connection chip: text only */
function NetChip({ mode }) {
  if (mode === "ble") return <span className="netchip" style={{ color: "var(--cyan)" }}>BLE</span>;
  if (mode === "wifi") return <span className="netchip" style={{ color: "var(--ok)" }}>WiFi</span>;
  return <span className="netchip" style={{ color: "var(--fg-3)" }}>离线</span>;
}

/* ---------------- scroll list ---------------- */
function ScrollList({ items, sel, visible, render, rowH }) {
  const start = Math.max(0, Math.min(sel - 2, items.length - visible));
  const shown = items.slice(start, start + visible);
  const overflow = items.length > visible;
  return (
    <div className="body">
      <div className="list">{shown.map((it, i) => render(it, start + i, start + i === sel))}</div>
      {overflow && (
        <div className="scrollbar">
          <div className="thumb" style={{ height: (visible / items.length) * 100 + "%", top: (start / items.length) * 100 + "%" }} />
        </div>
      )}
    </div>
  );
}

/* volume bars (0..21 → 7 cells), no icon */
function VolBar({ vol }) {
  const cells = 7, on = Math.round((vol / 21) * cells);
  return (
    <span className="vol">
      <span className="vlabel">音量</span>
      <span className="vbar">{Array.from({ length: cells }).map((_, i) =>
        <i key={i} className={i < on ? "on" : ""} style={{ height: 3 + i }} />)}</span>
    </span>
  );
}

/* ---------------- helpers ---------------- */
const fmtTime = (s) => `${Math.floor(s / 60)}:${String(Math.floor(s % 60)).padStart(2, "0")}`;

const ORDERS = [
  { id: "seq", cn: "顺序", k: "SEQ" },
  { id: "shuffle", cn: "随机", k: "SHF" },
  { id: "repeat", cn: "循环", k: "RPT" },
  { id: "one", cn: "单曲", k: "RP1" },
];

/* ---------------- data: playlists ---------------- */
const PLAYLISTS = [
  {
    name: "华语精选", en: "C-POP", type: "local", count: 12,
    tracks: [
      { t: "稻香", a: "周杰伦 · 依然范特西", d: 223, f: "FLAC" },
      { t: "晴天", a: "周杰伦 · 叶惠美", d: 269, f: "FLAC" },
      { t: "七里香", a: "周杰伦 · 七里香", d: 299, f: "MP3" },
      { t: "夜曲", a: "周杰伦 · 十一月的萧邦", d: 226, f: "FLAC" },
      { t: "一路向北", a: "周杰伦 · 十一月的萧邦", d: 350, f: "MP3" },
      { t: "蒲公英的约定", a: "周杰伦 · 我很忙", d: 235, f: "FLAC" },
    ],
  },
  {
    name: "Lo-Fi Beats", en: "FOCUS", type: "local", count: 8,
    tracks: [
      { t: "Midnight Tape", a: "Kessler", d: 184, f: "MP3" },
      { t: "Rainy Tokyo", a: "Aso", d: 201, f: "FLAC" },
      { t: "Slow Train", a: "Idealism", d: 156, f: "MP3" },
      { t: "Coffee Stain", a: "Eaup", d: 172, f: "OGG" },
    ],
  },
  {
    name: "硬核电台", en: "RSS", type: "rss", count: 5,
    tracks: [
      { t: "Vol.214 这一年的硬件", a: "硬核节目 · 2026-05-28", d: 4521, f: "RSS" },
      { t: "Vol.213 ESP32 之夜", a: "硬核节目 · 2026-05-21", d: 3980, f: "RSS" },
      { t: "Vol.212 焊台与人生", a: "硬核节目 · 2026-05-14", d: 4210, f: "RSS" },
    ],
  },
  {
    name: "英语听力", en: "RSS", type: "rss", count: 6,
    tracks: [
      { t: "EP.88 Small Talk", a: "Daily English · 05-30", d: 612, f: "RSS" },
      { t: "EP.87 At the Airport", a: "Daily English · 05-29", d: 588, f: "RSS" },
    ],
  },
];

/* ---------------- data: notifications ---------------- */
const NOTIFS = [
  { app: "微信", body: "张三：晚上的聚会还去吗？记得带相机", time: "14:26", count: 3, icon: "bell" },
  { app: "支付宝", body: "收款到账 ¥28.00　来自 楼下便利店", time: "14:24", count: 1, icon: "bell" },
  { app: "微博", body: "「M5Stack」发布了新动态：Cardputer ADV 固件开源", time: "14:20", count: 12, icon: "bell" },
];
const CALLS = [
  { name: "张三", num: "138 0013 8000" },
  { name: "妈妈", num: "139 8888 0001" },
];

Object.assign(window, { Icon, StatusBar, NetChip, ScrollList, VolBar, fmtTime, ORDERS, PLAYLISTS, NOTIFS, CALLS });
