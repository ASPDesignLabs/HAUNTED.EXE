/* The world editor's side passages, drawn (docs/PLAN-side.md, step 9): a second kind of map, its own three columns shown in place of
 * the crypt's while a passage is open. The header, the store, the questions and PLAY are the editor's own (editor.html hands them over
 * in SideUI.init); the passage's logic is SideCore's (side.js). */
(function () {
'use strict';
const SC = window.SideCore;
const SideUI = window.SideUI = {};
let E = null, $ = null, esc = null;
const T = { map: null, id: null, hist: [], fut: [], dirty: false, savedAt: null, example: false, zoom: 24, tool: { kind: 'select' }, sel: null,
            tab: 'inspect', checks: [], hl: null, grid: true, drag: null, hover: null };
SideUI.active = () => !!T.map;
SideUI.isSide = body => !!body && (body.kind === 'side' || (body.map && body.map.kind === 'side'));

/* ---- the tiles and things, as the editor draws them: 16 x 16, after the game's draft art ---- */
function rng(seed) { let s = seed >>> 0 || 1; return () => (s = (s * 1664525 + 1013904223) >>> 0) / 4294967296; }
function tileCanvas(draw) { const c = document.createElement('canvas'); c.width = c.height = 16; const g = c.getContext('2d'); draw(g, rng(draw.toString().length * 31 + 7)); return c; }
function speckle(g, r, cols, wts) { for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const v = r(); let k = 0, a = wts[0]; while (v > a && k < wts.length - 1) a += wts[++k]; g.fillStyle = cols[k]; g.fillRect(x, y, 1, 1); } }
const STONE = ['#55505e', '#645e6d', '#47424f', '#736c7c'], BACK = ['#2a2630', '#302b36', '#232028'], BRICK = ['#8a3a2c', '#9c4634', '#743024'];
const TILE = {
  '#': tileCanvas((g, r) => { speckle(g, r, STONE, [0.6, 0.2, 0.2]); g.fillStyle = '#2b2830'; g.fillRect(0, 7, 16, 1); g.fillRect(0, 15, 16, 1); g.fillRect(7, 0, 1, 7); g.fillRect(3, 8, 1, 7); g.fillRect(12, 8, 1, 7); g.fillStyle = '#8a8394'; g.fillRect(0, 0, 7, 1); g.fillRect(8, 0, 8, 1); g.fillRect(0, 8, 3, 1); g.fillRect(4, 8, 8, 1); }),
  '%': tileCanvas((g, r) => { speckle(g, r, BRICK, [0.6, 0.2, 0.2]); g.fillStyle = '#5c4a3c'; for (let y = 3; y < 16; y += 4) g.fillRect(0, y, 16, 1); for (let y = 0; y < 16; y += 4) for (let x = ((y / 4) & 1) * 4; x < 16; x += 8) g.fillRect(x + 7, y, 1, 3); }),
  ',': tileCanvas((g, r) => { speckle(g, r, BACK, [0.7, 0.15, 0.15]); g.fillStyle = '#1a171e'; g.fillRect(0, 7, 16, 1); g.fillRect(0, 15, 16, 1); g.fillRect(9, 0, 1, 7); g.fillRect(2, 8, 1, 7); }),
  '.': tileCanvas((g, r) => { speckle(g, r, ['#0e0c12', '#121016', '#0b0a0e'], [0.7, 0.15, 0.15]); }),
  '=': tileCanvas((g, r) => { speckle(g, r, BACK, [0.7, 0.15, 0.15]); g.fillStyle = '#8a5530'; g.fillRect(0, 1, 16, 5); g.fillStyle = '#b07040'; g.fillRect(0, 1, 16, 1); g.fillStyle = '#4d2e18'; g.fillRect(0, 5, 16, 1); g.fillStyle = '#7a7f88'; g.fillRect(2, 6, 3, 3); g.fillRect(11, 6, 3, 3); }),
  '^': tileCanvas((g, r) => { speckle(g, r, BACK, [0.7, 0.15, 0.15]); for (let s = 0; s < 4; s++) { const cx = s * 4 + 2; for (let y = 6; y < 16; y++) { const h = ((y - 6) * 2 / 9) | 0; g.fillStyle = '#b8bcc4'; g.fillRect(cx - h - 1, y, h + 1, 1); g.fillStyle = '#6a6e76'; g.fillRect(cx, y, h + 1, 1); } } g.fillStyle = '#d2473b'; g.fillRect(6, 7, 1, 2); }),
  '~': tileCanvas((g, r) => { speckle(g, r, ['#3d6a26', '#4a7a2e', '#335a20'], [0.6, 0.2, 0.2]); g.fillStyle = '#9fd06a'; for (let x = 0; x < 16; x++) g.fillRect(x, 2 + Math.round(Math.sin(x * 0.8)), 1, 1); g.fillStyle = '#c6f2a8'; g.fillRect(4, 9, 1, 1); g.fillRect(11, 12, 1, 1); }),
};
function icon(draw) { const c = document.createElement('canvas'); c.width = c.height = 16; const g = c.getContext('2d'); draw(g); return c; }
const ICON = {
  door: icon(g => { g.fillStyle = '#8a8394'; g.fillRect(2, 2, 12, 14); g.fillStyle = '#1a171e'; g.fillRect(4, 4, 8, 12); g.fillStyle = '#c88a3a'; g.fillRect(6, 8, 4, 8); }),
  goal: icon(g => { g.fillStyle = '#e9c24c'; g.fillRect(2, 2, 12, 14); g.fillStyle = '#1a171e'; g.fillRect(4, 4, 8, 12); g.fillStyle = '#ffd34d'; g.fillRect(6, 8, 4, 8); }),
  plat: icon(g => { g.fillStyle = '#7a7f88'; g.fillRect(1, 8, 14, 5); g.fillStyle = '#b8bcc4'; g.fillRect(1, 8, 14, 1); g.fillStyle = '#4a4e56'; g.fillRect(3, 0, 1, 8); g.fillRect(12, 0, 1, 8); }),
  fall: icon(g => { g.fillStyle = '#9c8466'; g.fillRect(1, 1, 14, 14); g.fillStyle = '#c4ab8a'; g.fillRect(1, 1, 14, 2); g.fillStyle = '#4d3d2f'; [[5, 3], [6, 5], [7, 7], [8, 8], [9, 10]].forEach(([x, y]) => g.fillRect(x, y, 1, 2)); }),
  flow: icon(g => { g.fillStyle = '#4a7a2e'; g.fillRect(0, 6, 16, 10); g.fillStyle = '#9fd06a'; g.fillRect(0, 6, 16, 2); g.fillStyle = '#c6f2a8'; g.fillRect(3, 2, 1, 3); g.fillRect(8, 0, 1, 5); g.fillRect(12, 3, 1, 2); }),
  mob: icon(g => { g.fillStyle = '#d8d0bd'; g.beginPath(); g.arc(8, 7, 5, 0, 7); g.fill(); g.fillRect(5, 10, 6, 4); g.fillStyle = '#17141b'; g.fillRect(5, 6, 2, 2); g.fillRect(9, 6, 2, 2); g.fillRect(7, 12, 1, 2); }),
  gold: icon(g => { g.fillStyle = '#e9c24c'; g.beginPath(); g.arc(8, 9, 5, 0, 7); g.fill(); g.fillStyle = '#fff1a6'; g.fillRect(6, 6, 2, 2); }),
  key0: null, key1: null, key2: null,
  life: icon(g => { g.fillStyle = '#9c4634'; g.fillRect(3, 11, 10, 4); g.fillStyle = '#ffd34d'; g.fillRect(7, 3, 2, 8); g.fillStyle = '#f39a2a'; g.fillRect(6, 6, 4, 5); }),
  switch: icon(g => { g.fillStyle = '#7a7f88'; g.fillRect(3, 12, 10, 4); g.strokeStyle = '#9a6236'; g.lineWidth = 2; g.beginPath(); g.moveTo(8, 12); g.lineTo(4, 3); g.stroke(); g.fillStyle = '#d2473b'; g.fillRect(2, 1, 4, 4); }),
  torch: icon(g => { g.fillStyle = '#6a6e76'; g.fillRect(7, 8, 2, 8); g.fillRect(5, 8, 6, 1); g.fillStyle = '#f39a2a'; g.fillRect(6, 3, 4, 5); g.fillStyle = '#ffd34d'; g.fillRect(7, 1, 2, 5); }),
  select: icon(g => { g.fillStyle = '#d8d0bd'; g.beginPath(); g.moveTo(3, 1); g.lineTo(3, 13); g.lineTo(6, 10); g.lineTo(9, 15); g.lineTo(11, 14); g.lineTo(8, 9); g.lineTo(12, 9); g.closePath(); g.fill(); }),
  erase: icon(g => { g.strokeStyle = '#d2473b'; g.lineWidth = 3; g.beginPath(); g.moveTo(3, 3); g.lineTo(13, 13); g.moveTo(13, 3); g.lineTo(3, 13); g.stroke(); }),
};
['#e9c24c', '#c9ccd4', '#d2473b'].forEach((c, k) => { ICON['key' + k] = icon(g => { g.fillStyle = c; g.beginPath(); g.arc(5, 8, 3.5, 0, 7); g.fill(); g.fillRect(8, 7, 7, 2); g.fillRect(12, 9, 2, 3); g.fillStyle = '#17141b'; g.beginPath(); g.arc(5, 8, 1.5, 0, 7); g.fill(); }); });
const GOLD_NAMES = { IT_COINS: 'Coins', IT_CHALICE: 'Chalice', IT_GEMS: 'Gems', IT_CROWN: 'Crown', IT_CHEST: 'Chest' };
const PROTO = {
  door: { k: 'door', link: 0, goal: false }, goal: { k: 'door', link: 1, goal: true },
  plat: { k: 'plat', w: 3, dx: 1, dy: 0, dist: 6, speed: 2, loop: false, wait: 0.5, at: 0 },
  lift: { k: 'plat', w: 2, dx: 0, dy: -1, dist: 6, speed: 1.5, loop: false, wait: 1, at: 0 },
  fall: { k: 'fall', delay: SC.FALL_DELAY, back: SC.FALL_BACK }, flow: { k: 'flow', w: 4, h: 1, on: 3, off: 5, at: 0 },
  torch: { k: 'torch' }, mob: { k: 'mob', kind: 'ANY', rank: -1 }, life: { k: 'life' }, switch: { k: 'switch', thing: -1, toggle: false },
};

/* ---- history ---- */
const snap = () => JSON.stringify(T.map);
function change(fn, quiet) {
  const before = snap();
  fn();
  if (snap() === before) return;
  T.hist.push(before); if (T.hist.length > 200) T.hist.shift(); T.fut = [];
  T.dirty = true; refresh(quiet); draft();
}
function undo() { if (!T.hist.length) return; T.fut.push(snap()); T.map = JSON.parse(T.hist.pop()); T.sel = null; T.dirty = true; refresh(); E.toast('UNDONE'); }
function redo() { if (!T.fut.length) return; T.hist.push(snap()); T.map = JSON.parse(T.fut.pop()); T.sel = null; T.dirty = true; refresh(); E.toast('REDONE'); }
const pack = m => ({ ...m, grid: m.grid.join('') });
const unpack = p => ({ ...p, grid: typeof p.grid === 'string' ? p.grid.split('') : p.grid });
function draft() { try { localStorage.setItem('cme.sdraft', JSON.stringify({ id: T.id, map: pack(T.map), t: Date.now() })); } catch (e) {} }

/* ---- the columns ---- */
let cv = null, g = null;
function build() {
  const st = document.createElement('style');                    /* the page's .main is a grid, which [hidden] alone does not hide */
  st.textContent = '#app > .main[hidden] { display: none; } #sdMain h2 { margin-bottom: 2px; }';
  document.head.appendChild(st);
  const main = document.createElement('div');
  main.className = 'main'; main.id = 'sdMain'; main.hidden = true;
  main.innerHTML = `
    <aside class="col left slab" aria-label="Side passage tools">
      <h2>A SIDE PASSAGE</h2>
      <div class="tools" id="sdMode"></div>
      <details open><summary>TILES</summary><div class="tools" id="sdTiles" style="margin-top:6px"></div></details>
      <details open><summary>WAYS AND HAZARDS</summary><div class="tools" id="sdWays" style="margin-top:6px"></div></details>
      <details open><summary>THINGS</summary><div class="tools" id="sdThings" style="margin-top:6px"></div></details>
    </aside>
    <section class="mapwrap slab" aria-label="Side passage">
      <div class="maptools">
        <button class="btn" id="sdUndo" title="Undo (Ctrl+Z)">UNDO</button>
        <button class="btn" id="sdRedo" title="Redo (Ctrl+Y)">REDO</button>
        <button class="btn" id="sdGrid" aria-pressed="true" title="Show the tile grid">GRID</button>
        <span class="sep"></span>
        <button class="btn" id="sdZoomOut" title="Zoom out (-)">-</button>
        <span class="num" id="sdZoom">24 PX</span>
        <button class="btn" id="sdZoomIn" title="Zoom in (+)">+</button>
      </div>
      <div class="scroller well" id="sdScroll"><canvas id="sdMap"></canvas></div>
    </section>
    <aside class="col right slab" aria-label="Side passage details">
      <div class="tabs" role="tablist">
        <button class="tab" role="tab" data-sdtab="inspect" aria-selected="true">CHOSEN</button>
        <button class="tab" role="tab" data-sdtab="checks" aria-selected="false">CHECKS <span class="n" id="sdCheckN"></span></button>
      </div>
      <div class="pane" id="sdPane"></div>
    </aside>`;
  $('app').insertBefore(main, $('status'));
  cv = $('sdMap'); g = cv.getContext('2d');
  const tb = (host, label, ic, tool, hint) => {
    const b = document.createElement('button'); b.className = 'tool'; b.type = 'button'; b.setAttribute('aria-pressed', 'false');
    const c = document.createElement('canvas'); c.width = c.height = 16; c.getContext('2d').drawImage(ic, 0, 0); b.appendChild(c);
    b.appendChild(document.createTextNode(label)); b.addEventListener('click', () => setTool(tool, b, hint)); $(host).appendChild(b); return b;
  };
  T.selectBtn = tb('sdMode', 'Select', ICON.select, { kind: 'select' }, 'Choose a thing to change it; drag it to move it. Choose nothing for the passage itself.');
  tb('sdMode', 'Erase', ICON.erase, { kind: 'erase' }, 'Click a thing to take it away.');
  SC.TILES.split('').forEach(ch => tb('sdTiles', SC.TILE_NAMES[ch], TILE[ch], { kind: 'tile', ch }, 'Paint ' + SC.TILE_NAMES[ch].toLowerCase() + '. Shift and drag for a box of it.'));
  tb('sdWays', 'Doorway', ICON.door, { kind: 'thing', p: 'door' }, 'Click the tile a doorway stands on its foot in; it is two tiles high. Up goes through it.');
  tb('sdWays', 'Goal doorway', ICON.goal, { kind: 'thing', p: 'goal' }, 'Leaving by the goal finishes the passage: a life back.');
  tb('sdWays', 'Platform', ICON.plat, { kind: 'thing', p: 'plat' }, 'A platform that goes back and forth. Its way, distance and speed are in CHOSEN.');
  tb('sdWays', 'Lift', ICON.plat, { kind: 'thing', p: 'lift' }, 'A platform that goes up and down.');
  tb('sdWays', 'Falling block', ICON.fall, { kind: 'thing', p: 'fall' }, 'It shakes when stood on, falls, and comes back.');
  tb('sdWays', 'Poison flow', ICON.flow, { kind: 'thing', p: 'flow' }, 'A box the poison fills and drains on a clock. Click its top left tile.');
  tb('sdWays', 'Torch', ICON.torch, { kind: 'thing', p: 'torch' }, 'A light on the back wall.');
  tb('sdThings', 'Creature', ICON.mob, { kind: 'thing', p: 'mob' }, "One of the dead: the run's own unless you name it.");
  SC.GOLD.forEach(it => tb('sdThings', GOLD_NAMES[it], ICON.gold, { kind: 'thing', p: 'gold', it }, 'Gold, as the crypt\'s.'));
  [0, 2, 1].forEach(c => tb('sdThings', SC.KEYS[c] + ' key', ICON['key' + c], { kind: 'thing', p: 'key', c }, 'The crypt\'s own key: it opens that colour\'s doors there.'));
  tb('sdThings', 'A life', ICON.life, { kind: 'thing', p: 'life' }, 'A life, never more than three.');
  tb('sdThings', 'Switch', ICON.switch, { kind: 'thing', p: 'switch' }, "Thrown with Up; it fires the crypt's triggers on the thing it names.");
  $('sdUndo').addEventListener('click', undo); $('sdRedo').addEventListener('click', redo);
  $('sdGrid').addEventListener('click', e => { T.grid = !T.grid; e.target.setAttribute('aria-pressed', String(T.grid)); draw(); });
  $('sdZoomIn').addEventListener('click', () => zoom(1)); $('sdZoomOut').addEventListener('click', () => zoom(-1));
  document.querySelectorAll('[data-sdtab]').forEach(b => b.addEventListener('click', () => setTab(b.dataset.sdtab)));
  cv.addEventListener('pointerdown', down); cv.addEventListener('pointermove', move); cv.addEventListener('pointerup', up); cv.addEventListener('pointercancel', up);
  cv.addEventListener('pointerleave', () => { T.hover = null; draw(); });
}
function setTool(tool, btn, hint) {
  T.tool = tool;
  document.querySelectorAll('#sdMain .tool').forEach(b => b.setAttribute('aria-pressed', String(b === btn)));
  $('sHint').textContent = hint || '';
  cv.style.cursor = tool.kind === 'select' ? 'default' : 'crosshair';
}
const ZOOMS = [8, 12, 16, 20, 24, 32, 40, 48];
function zoom(d) { const i = Math.max(0, Math.min(ZOOMS.length - 1, ZOOMS.indexOf(T.zoom) + d)); T.zoom = ZOOMS[i]; $('sdZoom').textContent = T.zoom + ' PX'; draw(); }

/* ---- drawing ---- */
function bounds(t) {                              /* the tiles a thing covers: x, y, w, h */
  switch (t.k) {
  case 'door': return [t.x, t.y - 1, 1, 2];
  case 'plat': return [t.x, t.y, t.w, 1];
  case 'flow': return [t.x, t.y, t.w, t.h];
  default: return [t.x, t.y, 1, 1];
  }
}
function draw() {
  if (!T.map) return;
  const m = T.map, z = T.zoom, W = m.w * z, H = m.h * z;
  if (cv.width !== W || cv.height !== H) { cv.width = W; cv.height = H; cv.style.width = W + 'px'; cv.style.height = H + 'px'; }
  g.imageSmoothingEnabled = false;
  for (let y = 0; y < m.h; y++) for (let x = 0; x < m.w; x++) g.drawImage(TILE[m.grid[y * m.w + x]] || TILE['.'], x * z, y * z, z, z);
  if (T.grid && z >= 12) {
    g.strokeStyle = 'rgba(216,208,189,0.08)'; g.lineWidth = 1; g.beginPath();
    for (let x = 0; x <= m.w; x++) { g.moveTo(x * z + 0.5, 0); g.lineTo(x * z + 0.5, H); }
    for (let y = 0; y <= m.h; y++) { g.moveTo(0, y * z + 0.5); g.lineTo(W, y * z + 0.5); }
    g.stroke();
  }
  m.things.forEach(t => {                          /* the flows first: under all else */
    if (t.k !== 'flow') return;
    g.fillStyle = 'rgba(121,193,90,0.35)'; g.fillRect(t.x * z, t.y * z, t.w * z, t.h * z);
    g.setLineDash([4, 3]); g.strokeStyle = '#9fd06a'; g.lineWidth = 2; g.strokeRect(t.x * z + 1, t.y * z + 1, t.w * z - 2, t.h * z - 2); g.setLineDash([]);
    if (z >= 16) { g.fillStyle = '#c6f2a8'; g.font = Math.max(10, z * 0.5) + 'px VT323, monospace'; g.fillText(`${t.on}s / ${t.off}s`, t.x * z + 3, t.y * z + z * 0.7); }
  });
  m.things.forEach(t => {
    const [bx, by, bw, bh] = bounds(t);
    if (t.k === 'flow') return;
    if (t.k === 'door') {
      g.drawImage(t.goal ? ICON.goal : ICON.door, bx * z, by * z, z, 2 * z);
      g.fillStyle = t.goal ? '#e9c24c' : '#d8d0bd'; g.font = Math.max(10, z * 0.55) + 'px Silkscreen, monospace'; g.fillText(String(t.link), bx * z + 2, by * z + z * 0.6);
      return;
    }
    if (t.k === 'plat') {
      const n = Math.hypot(t.dx, t.dy) || 1, ex = t.x + t.dx / n * t.dist, ey = t.y + t.dy / n * t.dist;
      g.globalAlpha = 0.35; for (let k = 0; k < t.w; k++) g.drawImage(ICON.plat, (ex + k) * z, ey * z, z, z); g.globalAlpha = 1;
      g.strokeStyle = '#b8bcc4'; g.setLineDash([3, 3]); g.lineWidth = 1; g.beginPath(); g.moveTo((t.x + t.w / 2) * z, (t.y + 0.6) * z); g.lineTo((ex + t.w / 2) * z, (ey + 0.6) * z); g.stroke(); g.setLineDash([]);
      for (let k = 0; k < t.w; k++) g.drawImage(ICON.plat, (t.x + k) * z, t.y * z, z, z);
      return;
    }
    const ic = t.k === 'gold' ? ICON.gold : t.k === 'key' ? ICON['key' + t.c] : ICON[t.k];
    if (ic) g.drawImage(ic, bx * z, by * z, z, z);
    if (t.k === 'mob' && z >= 16) { g.fillStyle = '#d2473b'; g.font = Math.max(10, z * 0.45) + 'px VT323, monospace'; g.fillText(t.kind === 'ANY' ? '?' : t.kind.slice(4, 6), bx * z + 1, by * z + z - 1); }
  });
  const hl = T.hl && T.hl.at;
  if (hl) { g.strokeStyle = '#d2473b'; g.lineWidth = 3; g.strokeRect(hl[0] * z - 2, hl[1] * z - 2, z + 4, z + 4); }
  const s = selected();
  if (s) { const [bx, by, bw, bh] = bounds(s); g.strokeStyle = '#e9c24c'; g.lineWidth = 2; g.strokeRect(bx * z + 1, by * z + 1, bw * z - 2, bh * z - 2); }
  if (T.drag && T.drag.rect) { const r = T.drag.rect; g.strokeStyle = '#e9c24c'; g.setLineDash([4, 3]); g.lineWidth = 2; g.strokeRect(Math.min(r.x0, r.x1) * z, Math.min(r.y0, r.y1) * z, (Math.abs(r.x1 - r.x0) + 1) * z, (Math.abs(r.y1 - r.y0) + 1) * z); g.setLineDash([]); }
  if (T.hover) { g.strokeStyle = 'rgba(233,194,76,0.6)'; g.lineWidth = 1; g.strokeRect(T.hover[0] * z + 0.5, T.hover[1] * z + 0.5, z - 1, z - 1); }
}

/* ---- the pointer ---- */
function cell(e) { const r = cv.getBoundingClientRect(); return [Math.floor((e.clientX - r.left) / T.zoom), Math.floor((e.clientY - r.top) / T.zoom)]; }
const inMap = (x, y) => x >= 0 && y >= 0 && x < T.map.w && y < T.map.h;
function hit(x, y) {                               /* the thing on top at a tile: the last drawn first */
  for (let i = T.map.things.length - 1; i >= 0; i--) { const t = T.map.things[i], [bx, by, bw, bh] = bounds(t); if (x >= bx && y >= by && x < bx + bw && y < by + bh && t.k !== 'flow') return t; }
  for (let i = T.map.things.length - 1; i >= 0; i--) { const t = T.map.things[i], [bx, by, bw, bh] = bounds(t); if (x >= bx && y >= by && x < bx + bw && y < by + bh) return t; }
  return null;
}
const selected = () => T.sel == null ? null : T.map.things.find(t => t.id === T.sel) || null;
function paint(x, y) { if (inMap(x, y)) T.map.grid[y * T.map.w + x] = T.tool.ch; }
function down(e) {
  if (e.button !== 0 || !T.map) return;
  cv.setPointerCapture(e.pointerId);
  const [x, y] = cell(e);
  if (!inMap(x, y)) return;
  const tl = T.tool;
  if (tl.kind === 'tile') {
    if (e.shiftKey) { T.drag = { rect: { x0: x, y0: y, x1: x, y1: y } }; draw(); return; }
    T.drag = { paint: true, before: snap() }; paint(x, y); draw(); return;
  }
  if (tl.kind === 'erase') { const t = hit(x, y); if (t) change(() => { T.map.things = T.map.things.filter(o => o !== t); if (T.sel === t.id) T.sel = null; }); return; }
  if (tl.kind === 'thing') {
    if (tl.p === 'door' && y < 1) { E.toast('A DOORWAY STANDS TWO TILES HIGH: NOT ON THE TOP ROW'); return; }
    const proto = Object.assign(JSON.parse(JSON.stringify(PROTO[tl.p] || { k: tl.p })), tl.it ? { it: tl.it } : {}, tl.c != null ? { c: tl.c } : {});
    if (proto.k === 'door') { const used = new Set(T.map.things.filter(t => t.k === 'door').map(t => t.link)); let l = proto.goal ? proto.link : 0; while (used.has(l) && l < 15) l++; proto.link = l; }
    if (proto.k === 'flow') { proto.w = Math.min(proto.w, T.map.w - x); proto.h = Math.min(proto.h, T.map.h - y); }
    if (proto.k === 'plat') proto.w = Math.min(proto.w, T.map.w - x);
    change(() => { const o = SC.add(T.map, Object.assign(proto, { x, y })); T.sel = o.id; });
    setTab('inspect'); return;
  }
  const t = hit(x, y);
  T.sel = t ? t.id : null; T.hl = null;
  if (t) T.drag = { move: t.id, fx: x, fy: y, ox: t.x, oy: t.y, before: snap() };
  setTab('inspect'); draw();
}
function move(e) {
  if (!T.map) return;
  const [x, y] = cell(e);
  T.hover = inMap(x, y) ? [x, y] : null;
  $('sCell').textContent = T.hover ? `${x}, ${y}` : '-';
  $('sBlock').textContent = T.hover ? SC.TILE_NAMES[T.map.grid[y * T.map.w + x]] || '-' : '-';
  $('sLane').textContent = '-';
  const d = T.drag;
  if (d && d.paint && inMap(x, y)) paint(x, y);
  if (d && d.rect) { d.rect.x1 = Math.max(0, Math.min(T.map.w - 1, x)); d.rect.y1 = Math.max(0, Math.min(T.map.h - 1, y)); }
  if (d && d.move != null) {
    const t = T.map.things.find(o => o.id === d.move);
    if (t) { const [, , bw, bh] = bounds(t); t.x = Math.max(0, Math.min(T.map.w - bw, d.ox + x - d.fx)); t.y = Math.max(t.k === 'door' ? 1 : 0, Math.min(T.map.h - (t.k === 'door' ? 1 : bh), d.oy + y - d.fy)); }
  }
  draw();
}
function up() {
  const d = T.drag; T.drag = null;
  if (!d) return;
  if (d.rect) { const r = d.rect; change(() => { for (let y = Math.min(r.y0, r.y1); y <= Math.max(r.y0, r.y1); y++) for (let x = Math.min(r.x0, r.x1); x <= Math.max(r.x0, r.x1); x++) paint(x, y); }); return; }
  if (d.before && snap() !== d.before) { T.hist.push(d.before); T.fut = []; T.dirty = true; refresh(); draft(); }
}

/* ---- the tablet: what is chosen, or the passage; the checks ---- */
function setTab(t) { T.tab = t; document.querySelectorAll('[data-sdtab]').forEach(b => b.setAttribute('aria-selected', String(b.dataset.sdtab === t))); pane(); }
const opt = (v, label, cur) => `<option value="${esc(v)}"${String(v) === String(cur) ? ' selected' : ''}>${esc(label)}</option>`;
const numIn = (id, v, min, max, step) => `<input type="number" id="${id}" value="${v}" min="${min}" max="${max}" step="${step || 1}">`;
function pane() {
  if (!T.map) return;
  const p = $('sdPane');
  if (T.tab === 'checks') {
    p.innerHTML = T.checks.length ? `<div class="checks">${T.checks.map((c, i) => `<button class="check" data-ck="${i}" aria-pressed="${T.hl === c}"><span class="pip ${c.sev}">${c.sev === 'error' ? 'ERROR' : 'WARN'}</span><span>${esc(c.msg)}</span></button>`).join('')}</div>`
      : '<p class="note">Nothing found wrong. The game would read this passage as it stands.</p>';
    p.querySelectorAll('[data-ck]').forEach(b => b.addEventListener('click', () => { const c = T.checks[+b.dataset.ck]; T.hl = c; if (c.id != null) T.sel = c.id; draw(); pane(); if (c.at) scrollTo(c.at); }));
    return;
  }
  const t = selected();
  if (!t) { p.innerHTML = panePassage(); wirePassage(); return; }
  p.innerHTML = paneThing(t); wireThing(t);
}
function scrollTo([x, y]) { const s = $('sdScroll'); s.scrollTo({ left: x * T.zoom - s.clientWidth / 2, top: y * T.zoom - s.clientHeight / 2, behavior: 'smooth' }); }
function panePassage() {
  const m = T.map, n = k => m.things.filter(t => t.k === k).length;
  return `<h2>THE PASSAGE</h2>
    <div class="field"><label for="pNum">Number</label><select id="pNum">${[0, 1, 2, 3, 4, 5, 6, 7].map(k => opt(k, k === 0 ? '0: the test passage built in' : String(k), m.num)).join('')}</select></div>
    <p class="note">A crypt level's side doorways name a passage by this number and one of its links. In the game it is SIDE_TEXT[${m.num}].</p>
    <div class="field"><label for="pLevel">Played on</label>${numIn('pLevel', m.level, 1, 160)}</div>
    <p class="note">PLAY begins a new run on this level and goes straight in. Its creatures are that level's run's.</p>
    <h3>SIZE</h3>
    <div class="row"><label>Across</label>${numIn('pW', m.w, SC.LIM.minW, SC.LIM.maxW)}<label>Down</label>${numIn('pH', m.h, SC.LIM.minH, SC.LIM.maxH)}</div>
    <div class="row"><button class="btn" id="pResize">RESIZE, KEEP THE TOP</button><button class="btn" id="pResizeB">RESIZE, KEEP THE FOOT</button></div>
    <p class="note">${SC.LIM.minW} to ${SC.LIM.maxW} tiles across and ${SC.LIM.minH} to ${SC.LIM.maxH} down; the screen shows 20 by 9 and a half. What is cut off is gone.</p>
    <h3>IN IT</h3>
    <div class="kv"><span>Doorways</span><span>${n('door')}</span><span>Platforms</span><span>${n('plat')}</span><span>Falling blocks</span><span>${n('fall')}</span><span>Flows</span><span>${n('flow')}</span>
      <span>Creatures</span><span>${n('mob')}</span><span>Gold, keys, lives</span><span>${n('gold') + n('key') + n('life')}</span><span>Things in all</span><span>${m.things.length} of ${SC.LIM.things}</span></div>
    <p class="note">Below the foot is a drop: a fall there costs a life. Left and right of the map are stone.</p>`;
}
function wirePassage() {
  const on = (id, ev, fn) => { const el = $(id); if (el) el.addEventListener(ev, fn); };
  on('pNum', 'change', e => change(() => { T.map.num = +e.target.value; }));
  on('pLevel', 'change', e => change(() => { T.map.level = Math.max(1, Math.min(160, +e.target.value || 1)); }));
  on('pResize', 'click', () => change(() => SC.resize(T.map, +$('pW').value, +$('pH').value, false)));
  on('pResizeB', 'click', () => change(() => SC.resize(T.map, +$('pW').value, +$('pH').value, true)));
}
function paneThing(t) {
  let h = `<h2>${esc(SC.label(t, E.DATA.kinds).toUpperCase())}</h2><div class="row"><label>x</label>${numIn('tX', t.x, 0, T.map.w - 1)}<label>y</label>${numIn('tY', t.y, 0, T.map.h - 1)}</div>`;
  switch (t.k) {
  case 'door':
    h += `<div class="field"><label for="tLink">Link</label>${numIn('tLink', t.link, 0, 15)}</div>
      <div class="row"><label><input type="checkbox" id="tGoal"${t.goal ? ' checked' : ''}> The goal</label></div>
      <p class="note">Up here goes out to the crypt doorway that names this passage (${T.map.num}) and link ${t.link}; coming in by that doorway puts you here. ${t.goal ? 'Leaving by the goal finishes the passage: a life back, never more than three.' : ''} Its y is the tile its foot is in.</p>`;
    break;
  case 'plat':
    h += `<div class="field"><label for="tW">Wide</label>${numIn('tW', t.w, 1, 8)}</div>
      <div class="field"><label for="tWay">Way</label><select id="tWay">${[['1,0', 'east'], ['-1,0', 'west'], ['0,-1', 'up'], ['0,1', 'down'], ['1,-1', 'up and east'], ['-1,-1', 'up and west'], ['1,1', 'down and east'], ['-1,1', 'down and west']].map(([v, l]) => opt(v, l, t.dx + ',' + t.dy)).join('')}</select></div>
      <div class="field"><label for="tDist">Tiles</label>${numIn('tDist', t.dist, 0.5, 200, 0.5)}</div>
      <div class="field"><label for="tSpd">Speed</label>${numIn('tSpd', t.speed, 0.1, 20, 0.1)}</div>
      <div class="field"><label for="tWait">Wait</label>${numIn('tWait', t.wait, 0, 30, 0.1)}</div>
      <div class="field"><label for="tAt">Starts</label>${numIn('tAt', t.at, 0, 1, 0.05)}</div>
      <div class="row"><label><input type="checkbox" id="tLoop"${t.loop ? ' checked' : ''}> Round: back to its start at once</label></div>
      <p class="note">Speed in tiles a second; the wait in seconds at each end; starts at 0 (here) to 1 (the far end). Back and forth unless round. It carries whatever stands on it; it can be jumped up through.</p>`;
    break;
  case 'fall':
    h += `<div class="field"><label for="tDel">Shakes</label>${numIn('tDel', t.delay, 0, 10, 0.05)}</div><div class="field"><label for="tBack">Back after</label>${numIn('tBack', t.back, 0.5, 60, 0.5)}</div>
      <p class="note">Seconds: how long it shakes once stood on before it falls, and how long till it is back in its place.</p>`;
    break;
  case 'flow':
    h += `<div class="row"><label>Across</label>${numIn('tFw', t.w, 1, T.map.w)}<label>Down</label>${numIn('tFh', t.h, 1, T.map.h)}</div>
      <div class="field"><label for="tOn">Full</label>${numIn('tOn', t.on, 0.5, 60, 0.5)}</div><div class="field"><label for="tOff">Dry</label>${numIn('tOff', t.off, 0.5, 60, 0.5)}</div>
      <div class="field"><label for="tAt">Begins at</label>${numIn('tAt', t.at, 0, 120, 0.5)}</div>
      <p class="note">Seconds: the poison rises over half a second and stays full, then drains and stays dry. A tenth of the caster's health each half second in it, and half the pace. Begins at: where in its clock it is as the passage opens.</p>`;
    break;
  case 'mob':
    h += `<div class="field"><label for="tKind">Creature</label><select id="tKind">${opt('ANY', "the run's choice", t.kind)}${E.DATA.kinds.filter(k => !k.unique && k.id !== 'MOB_SAC').map(k => opt(k.id, k.name.toLowerCase(), t.kind)).join('')}</select></div>
      <div class="field"><label for="tRank">Rank</label><select id="tRank">${opt(-1, "the level's usual", t.rank)}${SC.RANKS.map(r => opt(r, r === 'RK_GILDED' ? 'gilded' : 'rank ' + r.slice(3), t.rank)).join('')}</select></div>
      <p class="note">Walkers keep to their floor and come at you; bats, spectres, shades, imps and the Lightless fly; skeletons and hunters throw. Their health and blows are the crypt's for the level. Until their own side art is drawn they show their bestiary pictures.</p>`;
    break;
  case 'gold': h += `<div class="field"><label for="tIt">Kind</label><select id="tIt">${SC.GOLD.map(k => opt(k, `${GOLD_NAMES[k]} (${E.DATA.items[k]})`, t.it)).join('')}</select></div>`; break;
  case 'key': h += `<div class="field"><label for="tC">Colour</label><select id="tC">${[0, 2, 1].map(k => opt(k, SC.KEYS[k].toLowerCase(), t.c)).join('')}</select></div><p class="note">The crypt's own key: taken out of the passage, it opens that colour's doors there.</p>`; break;
  case 'switch':
    h += `<div class="field"><label for="tTh">Crypt thing</label>${numIn('tTh', t.thing, -1, SC.LIM.evThings - 1)}</div>
      <div class="row"><label><input type="checkbox" id="tTog"${t.toggle ? ' checked' : ''}> Thrown back and forth</label></div>
      <p class="note">Up throws it. It fires the crypt level's triggers of the kind "Pulled or pressed" on the thing numbered here (its place in that level's export: a lever, say). -1 names none. A life lost puts it back.</p>`;
    break;
  case 'life': h += '<p class="note">Picked up, a life, never more than three; with three already, five souls.</p>'; break;
  case 'torch': h += '<p class="note">Light on the back wall, and on all about it.</p>'; break;
  }
  return h + `<div class="row"><button class="btn" id="tDup">COPY</button><button class="btn warn" id="tDel">REMOVE</button></div>`;
}
function wireThing(t) {
  const on = (id, ev, fn) => { const el = $(id); if (el) el.addEventListener(ev, fn); };
  const n = (id, f) => on(id, 'change', e => change(() => f(+e.target.value)));
  n('tX', v => { t.x = Math.max(0, Math.min(T.map.w - 1, v | 0)); }); n('tY', v => { t.y = Math.max(t.k === 'door' ? 1 : 0, Math.min(T.map.h - 1, v | 0)); });
  n('tLink', v => { t.link = Math.max(0, Math.min(15, v | 0)); }); on('tGoal', 'change', e => change(() => { t.goal = e.target.checked; }));
  n('tW', v => { t.w = Math.max(1, Math.min(8, v | 0)); });
  on('tWay', 'change', e => change(() => { const [dx, dy] = e.target.value.split(',').map(Number); t.dx = dx; t.dy = dy; }));
  n('tDist', v => { t.dist = Math.max(0.5, v); }); n('tSpd', v => { t.speed = Math.max(0.1, v); }); n('tWait', v => { t.wait = Math.max(0, v); });
  n('tAt', v => { t.at = Math.max(0, t.k === 'plat' ? Math.min(1, v) : v); }); on('tLoop', 'change', e => change(() => { t.loop = e.target.checked; }));
  n('tDel', v => { t.delay = Math.max(0, v); }); n('tBack', v => { t.back = Math.max(0.5, v); });
  n('tFw', v => { t.w = Math.max(1, Math.min(T.map.w - t.x, v | 0)); }); n('tFh', v => { t.h = Math.max(1, Math.min(T.map.h - t.y, v | 0)); });
  n('tOn', v => { t.on = Math.max(0.5, v); }); n('tOff', v => { t.off = Math.max(0.5, v); });
  on('tKind', 'change', e => change(() => { t.kind = e.target.value; }));
  on('tRank', 'change', e => change(() => { t.rank = e.target.value === '-1' ? -1 : e.target.value; }));
  on('tIt', 'change', e => change(() => { t.it = e.target.value; })); n('tC', v => { t.c = v; });
  n('tTh', v => { t.thing = Math.max(-1, Math.min(SC.LIM.evThings - 1, v | 0)); }); on('tTog', 'change', e => change(() => { t.toggle = e.target.checked; }));
  on('tDup', 'click', () => change(() => { const o = SC.add(T.map, Object.assign(JSON.parse(JSON.stringify(t)), { x: Math.min(T.map.w - 1, t.x + 1) })); T.sel = o.id; }));
  on('tDel', 'click', () => removeSelected());
}
function removeSelected() { const t = selected(); if (t) change(() => { T.map.things = T.map.things.filter(o => o !== t); T.sel = null; }); }

let checkTimer = null;
function refresh(quiet) {
  if (!T.map) return;
  draw();
  $('mapName').value = T.map.name; $('mapLevel').value = T.map.level;
  $('sdUndo').disabled = !T.hist.length; $('sdRedo').disabled = !T.fut.length;
  badge();
  clearTimeout(checkTimer);
  checkTimer = setTimeout(() => {
    T.checks = SC.check(T.map);
    if (T.hl && !T.checks.some(c => c.msg === T.hl.msg)) T.hl = null;
    const e = T.checks.filter(c => c.sev === 'error').length; $('sdCheckN').textContent = e ? e : '';
    if (T.tab !== 'inspect' || !quiet) pane();
  }, 100);
  if (T.tab === 'inspect') pane();
}
function badge() {
  const b = $('saved');
  b.className = 'saved' + (T.example ? '' : T.dirty ? ' dirty' : T.savedAt ? ' ok' : '');
  b.textContent = T.example && !T.dirty ? 'EXAMPLE: NOT SAVED' : T.dirty ? 'UNSAVED CHANGES' : T.savedAt ? 'SAVED ' + new Date(T.savedAt).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }) : 'NOT SAVED';
}

/* ---- in and out of the passage's columns ---- */
function show(on) {
  $('sdMain').hidden = !on;
  document.querySelector('#app > .main:not(#sdMain)').hidden = on;
  ['bLattice', 'bLanes'].forEach(id => { const b = $(id); if (b) b.disabled = on; });
  $('bExport').textContent = on ? 'EXPORT TEXT' : 'EXPORT C';
  document.querySelector('label[for="mapLevel"]').textContent = on ? 'Played on' : 'Level';
}
SideUI.open = (id, body, example) => {
  const m = unpack(body.map || body);
  m.kind = 'side';
  T.map = m; T.id = id; T.hist = []; T.fut = []; T.sel = null; T.hl = null; T.drag = null;
  T.dirty = false; T.example = !!example; T.savedAt = example ? null : body.updated || null;
  show(true); setTool({ kind: 'select' }, T.selectBtn, 'A side passage. Choose a tool at the left; choose nothing for the passage itself.');
  refresh(); E.close();
};
SideUI.leave = () => { if (!T.map) return; T.map = null; show(false); };
SideUI.newPassage = () => { SideUI.open(null, { map: pack(SC.blank()) }, false); T.dirty = true; badge(); E.toast('A NEW SIDE PASSAGE'); };
SideUI.example = n => {
  const s = E.DATA.sides.find(x => x.n === n), r = SC.parse(s.text, E.DATA.kinds.map(k => k.id));
  if (r.error) { E.toast('THE EXAMPLE DID NOT READ: ' + r.error.toUpperCase()); return; }
  r.map.num = n; r.map.name = r.map.name.toLowerCase().replace(/(^|\s)\S/g, c => c.toUpperCase());
  SideUI.open(null, { map: pack(r.map) }, true); E.toast('THE EXAMPLE, UNSAVED');
};
SideUI.rename = v => change(() => { T.map.name = v.trim() || 'A side passage'; }, true);
SideUI.setLevel = v => change(() => { T.map.level = Math.max(1, Math.min(160, +v || 1)); });
SideUI.undo = undo; SideUI.redo = redo;
SideUI.dirty = () => T.dirty;
SideUI.key = e => {                                  /* the keys the editor's own handler passes on while a passage is open */
  const k = e.key.toLowerCase();
  if ((e.ctrlKey || e.metaKey) && k === 'z') { e.preventDefault(); e.shiftKey ? redo() : undo(); return; }
  if ((e.ctrlKey || e.metaKey) && k === 'y') { e.preventDefault(); redo(); return; }
  if ((e.ctrlKey || e.metaKey) && k === 's') { e.preventDefault(); SideUI.save(); return; }
  if (e.ctrlKey || e.metaKey || e.altKey) return;
  if (k === 'delete' || k === 'backspace') { e.preventDefault(); removeSelected(); }
  else if (k === 'escape') { setTool({ kind: 'select' }, T.selectBtn, ''); T.sel = null; T.hl = null; refresh(); }
  else if (k === 'v') setTool({ kind: 'select' }, T.selectBtn, 'Choose a thing; drag it to move it.');
  else if (k === '+' || k === '=') zoom(1);
  else if (k === '-') zoom(-1);
};
SideUI.save = async () => {
  if (!T.id) T.id = E.newId();
  const body = { name: T.map.name, level: T.map.level, kind: 'side', num: T.map.num, updated: Date.now(), map: pack(T.map) };
  try {
    if (E.S.db) {
      await E.S.db.doc('maps/' + T.id).set(body);
      await E.S.db.doc('maps/' + T.id).collection('versions').doc(String(body.updated)).set({ saved: body.updated, name: body.name, map: body.map });
    } else {
      const all = JSON.parse(localStorage.getItem('cme.maps') || '{}'); all[T.id] = body; localStorage.setItem('cme.maps', JSON.stringify(all));
      const v = JSON.parse(localStorage.getItem('cme.v.' + T.id) || '[]'); v.push({ saved: body.updated, name: body.name, map: body.map }); while (v.length > 30) v.shift();
      localStorage.setItem('cme.v.' + T.id, JSON.stringify(v));
    }
    T.dirty = false; T.example = false; T.savedAt = body.updated; badge();
    try { localStorage.removeItem('cme.sdraft'); } catch (e) {}
    E.toast(E.S.db ? 'SAVED, WITH A SAVE POINT' : 'SAVED IN THIS BROWSER ONLY');
  } catch (e) { E.toast('NOT SAVED: ' + (e && e.code === 'quota_exceeded' ? 'THE STORE IS FULL' : 'TRY AGAIN')); }
};
SideUI.backup = () => {
  const slug = T.map.name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '') || 'passage';
  E.offer(`haunted-side-${T.map.num}-${slug}.json`, JSON.stringify({ format: 'haunted-world-editor', v: 1, kind: 'side', saved: Date.now(), map: pack(T.map) }, null, 1));
};
SideUI.exportText = () => {
  const errs = T.checks.filter(c => c.sev === 'error').length;
  E.showText(`EXPORT: SIDE PASSAGE ${T.map.num}`, (errs ? `${errs} check${errs > 1 ? 's say' : ' says'} the game would refuse this: see CHECKS. ` : '') +
    `Paste the block into SIDE_TEXT[] in src/crawl/game/side_levels.c as entry ${T.map.num} (the entries before it must be there). The same text, without the quotes, is what PLAY hands the game.`,
    SC.exportC(T.map), `side-passage-${T.map.num}.txt`);
};
SideUI.play = () => {
  const errs = T.checks.filter(c => c.sev === 'error');
  if (errs.length) { E.ask('Mend it first?', `${errs.length} check${errs.length > 1 ? 's are errors' : ' is an error'}, and the game would refuse the passage. The first: ${errs[0].msg}`, 'SHOW THE CHECKS', () => setTab('checks'), 'KEEP EDITING'); return; }
  E.playOpen('', { sides: [{ n: T.map.num, text: SC.exportText(T.map) }], sideNow: T.map.num, level: T.map.level, name: T.map.name },
    `PLAYING THE SIDE PASSAGE ${String(T.map.name).toUpperCase()}: A NEW RUN ON LEVEL ${T.map.level}, STRAIGHT IN. ITS DOORWAYS LEAD BACK TO WHERE IT BEGAN.`);
};
/* a crypt level's PLAY: the passages its side doorways name, the newest saved of each number (a number none is saved for: the game's own) */
SideUI.sidesFor = async (cryptMap) => {
  const nums = [...new Set((cryptMap.things || []).filter(t => t.t === 'sidedoor').map(t => t.num | 0))];
  if (!nums.length) return [];
  let maps = [];
  try { maps = await E.listMaps(); } catch (e) { maps = []; }
  const out = [];
  nums.forEach(n => {
    const m = maps.filter(x => SideUI.isSide(x) && (x.num != null ? x.num : unpack(x.map).num) === n).sort((a, b) => b.updated - a.updated)[0];
    if (m) out.push({ n, text: SC.exportText(unpack(m.map)) });
  });
  return out;
};
SideUI.init = env => {
  E = env; $ = env.$; esc = env.esc;
  build();
  let d = null;
  try { d = JSON.parse(localStorage.getItem('cme.sdraft') || 'null'); } catch (e) {}
  T.draft = d && d.map ? d : null;
};
SideUI.draftWaiting = () => T.draft;
SideUI.takeDraft = () => { const d = T.draft; T.draft = null; if (!d) return; SideUI.open(d.id, { map: d.map }, false); T.dirty = true; badge(); };
})();
