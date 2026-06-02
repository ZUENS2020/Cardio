/* Cardio — extra 16×16 pixel glyphs (transport + media).
   Drawn in the same hand-placed, crispEdges style as the base sprite.
   Self-injects a hidden <svg> sprite so <use href="#ic-…"> resolves. */
(function () {
  const R = (x, y, w, h) => `<rect x="${x}" y="${y}" width="${w}" height="${h || 1}"></rect>`;

  // filled right-pointing triangle: vertical left edge at xLeft, apex at right-middle
  function triRight(xLeft, yTop, height, maxW) {
    const mid = (height - 1) / 2; let s = "";
    for (let r = 0; r < height; r++) {
      const w = Math.max(1, Math.round((1 - Math.abs(r - mid) / (mid + 0.6)) * maxW));
      s += R(xLeft, yTop + r, w, 1);
    } return s;
  }
  function triLeft(xRight, yTop, height, maxW) {
    const mid = (height - 1) / 2; let s = "";
    for (let r = 0; r < height; r++) {
      const w = Math.max(1, Math.round((1 - Math.abs(r - mid) / (mid + 0.6)) * maxW));
      s += R(xRight - w + 1, yTop + r, w, 1);
    } return s;
  }

  const GLYPHS = {
    // ---- transport ----
    pause: R(4, 3, 2, 10) + R(10, 3, 2, 10),
    next: triRight(2, 4, 8, 5) + triRight(7, 4, 8, 5) + R(13, 4, 2, 8),
    prev: triLeft(13, 4, 8, 5) + triLeft(8, 4, 8, 5) + R(1, 4, 2, 8),
    stop: R(3, 3, 10, 10),

    // ---- music note (eighth) ----
    note:
      R(4, 9, 3, 1) + R(3, 10, 5, 1) + R(2, 11, 6, 1) + R(2, 12, 6, 1) + R(3, 13, 5, 1) + R(4, 14, 3, 1) + // head
      R(7, 2, 2, 10) + // stem
      R(9, 2, 2, 1) + R(10, 3, 2, 1) + R(11, 4, 1, 2) + R(10, 6, 2, 1) + R(9, 7, 1, 1), // flag

    // ---- bell (notification) ----
    bell:
      R(7, 1, 2, 1) +
      R(6, 2, 4, 1) + R(5, 3, 6, 1) + R(5, 4, 6, 1) +
      R(4, 5, 8, 1) + R(4, 6, 8, 1) + R(4, 7, 8, 1) +
      R(3, 8, 10, 1) + R(3, 9, 10, 1) +
      R(2, 10, 12, 1) + R(2, 11, 12, 1) +
      R(1, 12, 14, 1) +
      R(7, 13, 2, 1) + R(6, 14, 4, 1), // clapper

    // ---- phone handset (incoming call) ----
    phone:
      R(2, 2, 4, 1) + R(2, 3, 4, 1) + R(2, 4, 3, 1) +
      R(4, 5, 2, 1) + R(5, 6, 2, 1) + R(6, 7, 2, 1) +
      R(7, 8, 2, 1) + R(8, 9, 2, 1) +
      R(10, 10, 3, 1) + R(11, 11, 3, 1) + R(11, 12, 3, 1) +
      R(9, 11, 1, 1) + R(10, 12, 1, 1),

    // ---- heart (favourite) ----
    heart:
      R(3, 3, 3, 1) + R(10, 3, 3, 1) +
      R(2, 4, 5, 1) + R(9, 4, 5, 1) +
      R(2, 5, 12, 1) + R(2, 6, 12, 1) +
      R(3, 7, 10, 1) + R(4, 8, 8, 1) + R(5, 9, 6, 1) + R(6, 10, 4, 1) + R(7, 11, 2, 1),

    // ---- list / queue ----
    list:
      R(2, 3, 2, 2) + R(6, 3, 8, 1) + R(6, 4, 8, 1) +
      R(2, 7, 2, 2) + R(6, 7, 8, 1) + R(6, 8, 8, 1) +
      R(2, 11, 2, 2) + R(6, 11, 8, 1) + R(6, 12, 8, 1),
  };

  let sprite = "";
  for (const [name, body] of Object.entries(GLYPHS)) {
    sprite += `<symbol id="ic-${name}" viewBox="0 0 16 16">${body}</symbol>`;
  }
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" style="position:absolute;width:0;height:0" fill="currentColor" shape-rendering="crispEdges" aria-hidden="true">${sprite}</svg>`;

  function inject() {
    const d = document.createElement("div");
    d.style.cssText = "position:absolute;width:0;height:0;overflow:hidden";
    d.innerHTML = svg;
    document.body.appendChild(d);
  }
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", inject);
  else inject();
})();
