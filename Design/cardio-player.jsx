// Cardio — Player screen (now playing), minimal: no cover, no visualizer, no key labels

function PlayerScreen({ st, overlay }) {
  const pl = st.pl, track = pl.tracks[st.idx], order = ORDERS[st.orderIdx];
  const total = track.d, pos = Math.min(st.pos, total);
  const pct = (pos / total) * 100;

  return (
    <div className="screen">
      <StatusBar
        title={<span className="cjk" style={{ fontWeight: 700 }}>{pl.name}</span>}
        right={<NetChip mode={st.mode} />}
        battery={st.battery} clock={st.clock} charging={st.charging}
      />
      <div className="player">
        <div className="pmeta">
          <div className="ptitle">{track.t}</div>
          <div className="partist">{track.a}</div>
          <div className="pfmt">
            <span className="src">{track.f}</span>
            <span className="dot">·</span>
            <span className="cn">{order.cn}</span>
          </div>
        </div>

        <div className="pbottom">
          <div className="progress">
            <span className="t">{fmtTime(pos)}</span>
            <span className="ptrack">
              <span className="fill" style={{ width: pct + "%" }} />
              <span className="knob" style={{ left: pct + "%" }} />
            </span>
            <span className="t r">{fmtTime(total)}</span>
          </div>

          <div className="pctl">
            <span className="pstate">
              <Icon name={st.playing ? "pause" : "play"} style={{ width: 11, height: 11, color: "var(--accent)" }} />
              <span className="cn">{st.playing ? "播放中" : "已暂停"}</span>
            </span>
            <VolBar vol={st.vol} />
          </div>
        </div>
      </div>
      {overlay}
    </div>
  );
}

window.PlayerScreen = PlayerScreen;
