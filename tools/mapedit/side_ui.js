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
ICON.gate = icon(g => { g.fillStyle = '#6a6e76'; g.fillRect(0, 1, 16, 2); for (let x = 1; x < 16; x += 4) { g.fillStyle = '#b8bcc4'; g.fillRect(x, 1, 1, 14); g.fillStyle = '#5a5e66'; g.fillRect(x + 1, 1, 1, 14); } g.fillStyle = '#8a8f99'; g.fillRect(0, 8, 16, 1); });
ICON.plate = icon(g => { g.fillStyle = '#736c7c'; g.fillRect(2, 11, 12, 3); g.fillStyle = '#a49cb0'; g.fillRect(2, 11, 12, 1); g.fillStyle = '#e9c24c'; g.fillRect(6, 9, 4, 1); });
ICON.zone = icon(g => { g.strokeStyle = '#c79bf0'; g.setLineDash([2, 2]); g.strokeRect(1.5, 1.5, 13, 13); g.fillStyle = '#c79bf0'; g.fillRect(7, 6, 2, 4); });
ICON.current = icon(g => { g.fillStyle = 'rgba(95,166,194,0.7)'; g.fillRect(0, 3, 16, 10); g.fillStyle = '#c8f6ff'; [[2, 6], [8, 9], [5, 11]].forEach(([x, y]) => g.fillRect(x, y, 5, 1)); g.fillRect(12, 5, 1, 3); g.fillRect(13, 6, 1, 1); });
ICON.pcurrent = icon(g => { g.fillStyle = 'rgba(121,193,90,0.8)'; g.fillRect(0, 3, 16, 10); g.fillStyle = '#c6f2a8'; [[2, 6], [8, 9], [5, 11]].forEach(([x, y]) => g.fillRect(x, y, 5, 1)); });
const FAM_COL = { fall: '#c4ab8a', line: '#f39a2a', creep: '#79c15a', cloud: '#c79bf0' };
['fall', 'line', 'creep', 'cloud'].forEach(f => { ICON['haz_' + f] = icon(g => { g.fillStyle = FAM_COL[f]; g.beginPath(); g.moveTo(8, 1); g.lineTo(15, 14); g.lineTo(1, 14); g.closePath(); g.fill(); g.fillStyle = '#17141b'; g.fillRect(7, 5, 2, 5); g.fillRect(7, 11, 2, 2); }); });
const PROTO = {
  door: { k: 'door', link: 0, goal: false }, goal: { k: 'door', link: 1, goal: true },
  plat: { k: 'plat', w: 3, dx: 1, dy: 0, dist: 6, speed: 2, loop: false, wait: 0.5, at: 0 },
  lift: { k: 'plat', w: 2, dx: 0, dy: -1, dist: 6, speed: 1.5, loop: false, wait: 1, at: 0 },
  fall: { k: 'fall', delay: SC.FALL_DELAY, back: SC.FALL_BACK }, flow: { k: 'flow', w: 4, h: 1, on: 3, off: 5, at: 0 },
  torch: { k: 'torch' }, mob: { k: 'mob', kind: 'ANY', rank: -1 }, life: { k: 'life' }, switch: { k: 'switch', thing: -1, toggle: false },
  gate: { k: 'gate', w: 1, h: 2, open: false }, plate: { k: 'plate' }, zone: { k: 'zone', w: 2, h: 2 },
  current: { k: 'current', w: 4, h: 1, dx: 1, dy: 0, speed: 3 }, pcurrent: { k: 'current', w: 4, h: 1, dx: 1, dy: 0, speed: 3, poison: true },
};

/* ---- history ---- */
const snap = () => JSON.stringify(T.map);
function change(fn, quiet) {
  const before = snap();
  fn();
  if (snap() === before) return;
  T.hist.push(before); if (T.hist.length > 200) T.hist.shift(); T.fut = [];
  T.dirty = true; refresh(quiet); draft();
  if (T.pv) { const t = T.map.things.find(o => o.id === T.pv.id); if (t && t.k === 'haz') previewStart(t); else previewStop(); }   /* PREVIEW follows what is set */
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
      <details open><summary>GATES, PLATES, WATER</summary><div class="tools" id="sdWorks" style="margin-top:6px"></div></details>
      <details><summary>THE CRYPT'S TRAPS</summary><div id="sdTraps" style="margin-top:6px;display:flex;flex-direction:column;gap:8px"></div></details>
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
        <button class="tab" role="tab" data-sdtab="trigs" aria-selected="false">TRIGGERS</button>
        <button class="tab" role="tab" data-sdtab="tune" aria-selected="false">TUNING</button>
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
  tb('sdThings', 'Switch', ICON.switch, { kind: 'thing', p: 'switch' }, "Thrown with Up; a trigger can listen to it, and it can fire the crypt's own triggers.");
  tb('sdWorks', 'Gate', ICON.gate, { kind: 'thing', p: 'gate' }, 'Bars across a box, shut till a trigger opens them. Its size is in CHOSEN.');
  tb('sdWorks', 'Pressure plate', ICON.plate, { kind: 'thing', p: 'plate' }, 'Pressed while the caster, or something loose, stands on it. Bind it in TRIGGERS.');
  tb('sdWorks', 'Unseen zone', ICON.zone, { kind: 'thing', p: 'zone' }, 'A box never seen: pressed while the caster is in it. Bind it in TRIGGERS.');
  tb('sdWorks', 'Water', ICON.current, { kind: 'thing', p: 'current' }, 'Water running through a box: it carries what is loose and pushes the caster. Its way is in CHOSEN.');
  tb('sdWorks', 'Poisoned water', ICON.pcurrent, { kind: 'thing', p: 'pcurrent' }, 'The same, and it bites.');
  const runs = (E.DATA.hazards && E.DATA.hazards.fixed) || [], RUNS = ['THE CRYPTS', 'THE CATACOMBS', 'THE HOLLOWS', 'THE DEEP'];
  runs.forEach((fixed, r) => {
    const kinds = [...fixed, ...(E.DATA.hazards.arise[r] || []), E.DATA.hazards.sapper[r]].filter(k => k && SC.HAZ_FAMILY[k]);
    const box = document.createElement('div'); box.innerHTML = `<h3>${RUNS[r]}</h3><div class="tools" id="sdTrap${r}" style="margin-top:4px"></div>`; $('sdTraps').appendChild(box);
    kinds.forEach(k => tb('sdTrap' + r, SC.HAZ_NAMES[k] || k, ICON['haz_' + SC.HAZ_FAMILY[k]], { kind: 'thing', p: 'haz', hz: k }, `${SC.HAZ_NAMES[k]}: ${SC.FAMILY_SAYS[SC.HAZ_FAMILY[k]]}. Click a tile for it, then paint its cells.`));
  });
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
  case 'flow': case 'gate': case 'zone': case 'current': return [t.x, t.y, t.w, t.h];
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
  m.things.forEach(t => {                          /* water, zones, gates, and the traps' cells */
    const x = t.x * z, y = t.y * z;
    if (t.k === 'current') {
      g.fillStyle = t.poison ? 'rgba(121,193,90,0.4)' : 'rgba(95,166,194,0.4)'; g.fillRect(x, y, t.w * z, t.h * z);
      g.strokeStyle = t.poison ? '#c6f2a8' : '#c8f6ff'; g.lineWidth = 2; g.beginPath();
      const cx = x + t.w * z / 2, cy = y + t.h * z / 2, L = Math.min(t.w, t.h) * z * 0.35 + z * 0.3;
      g.moveTo(cx - t.dx * L, cy - t.dy * L); g.lineTo(cx + t.dx * L, cy + t.dy * L); g.stroke();
      g.fillStyle = g.strokeStyle; g.fillRect(cx + t.dx * L - 3, cy + t.dy * L - 3, 6, 6);
      if (t.stopped) { g.fillStyle = '#d8d0bd'; g.font = Math.max(10, z * 0.45) + 'px VT323, monospace'; g.fillText('OFF', x + 3, y + z * 0.6); }
    } else if (t.k === 'zone') {
      g.setLineDash([5, 3]); g.strokeStyle = '#c79bf0'; g.lineWidth = 2; g.strokeRect(x + 1, y + 1, t.w * z - 2, t.h * z - 2); g.setLineDash([]);
    } else if (t.k === 'gate') {
      g.globalAlpha = t.open ? 0.35 : 1; for (let k = 0; k < t.w; k++) for (let j = 0; j < t.h; j++) g.drawImage(ICON.gate, x + k * z, y + j * z, z, z); g.globalAlpha = 1;
    } else if (t.k === 'haz') {
      const col = FAM_COL[SC.HAZ_FAMILY[t.kind]] || '#d2473b';
      (t.cells || []).forEach(([cx, cy], i) => {
        g.fillStyle = col; g.globalAlpha = 0.45; g.fillRect(cx * z, cy * z, z, z); g.globalAlpha = 1;
        if (z >= 16) { g.fillStyle = '#17141b'; g.font = Math.max(9, z * 0.42) + 'px VT323, monospace'; g.fillText(String(i + 1), cx * z + 2, cy * z + z * 0.5); }
      });
      if (t.enter) { g.setLineDash([4, 3]); g.strokeStyle = '#e89a3a'; g.lineWidth = 1.5; g.strokeRect(t.enter.x0 * z + 1, t.enter.y0 * z + 1, (t.enter.x1 - t.enter.x0 + 1) * z - 2, (t.enter.y1 - t.enter.y0 + 1) * z - 2); g.setLineDash([]); }
      g.drawImage(ICON['haz_' + (SC.HAZ_FAMILY[t.kind] || 'fall')], x, y, z * 0.8, z * 0.8);
      drawFxMarks(t, z);
    }
    if (t.tag && z >= 16 && ['gate', 'plate', 'zone', 'current', 'switch', 'haz'].includes(t.k)) { g.fillStyle = '#e9c24c'; g.font = Math.max(9, z * 0.4) + 'px VT323, monospace'; g.fillText(t.tag, x + 2, y + (t.k === 'plate' || t.k === 'switch' ? -2 : t.h ? t.h * z - 3 : z - 2)); }
  });
  const ts = selected();                           /* a chosen source's triggers: lines to what they move */
  if (ts && ts.tag) (m.trigs || []).forEach(gr => {
    const src = m.things.find(o => o.tag === gr.src); if (!src) return;
    if (src !== ts && !gr.acts.some(a => a.tag === ts.tag)) return;
    gr.acts.forEach(a => { const o = a.tag && m.things.find(q => q.tag === a.tag); if (!o) return;
      g.strokeStyle = '#e9c24c'; g.setLineDash([6, 4]); g.lineWidth = 2; g.beginPath(); g.moveTo((src.x + 0.5) * z, (src.y + 0.5) * z); g.lineTo((o.x + 0.5) * z, (o.y + 0.5) * z); g.stroke(); g.setLineDash([]); });
  });
  m.things.forEach(t => {
    const [bx, by, bw, bh] = bounds(t);
    if (['flow', 'current', 'zone', 'gate', 'haz'].includes(t.k)) return;
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
    if (t.loose && z >= 12) { g.strokeStyle = '#8fc6dc'; g.lineWidth = 1; g.beginPath(); g.moveTo(bx * z + z / 2, by * z + z); g.lineTo(bx * z + z / 2, by * z + z + z * 0.4); g.stroke(); }   /* loose: it will fall */
    if (t.k === 'mob' && z >= 16) { g.fillStyle = '#d2473b'; g.font = Math.max(10, z * 0.45) + 'px VT323, monospace'; g.fillText(t.kind === 'ANY' ? '?' : t.kind.slice(4, 6), bx * z + 1, by * z + z - 1); }
  });
  drawPreview(z);                                  /* PREVIEW (2e) */
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
  for (let i = T.map.things.length - 1; i >= 0; i--) { const t = T.map.things[i], [bx, by, bw, bh] = bounds(t); if (x >= bx && y >= by && x < bx + bw && y < by + bh && !['flow', 'current', 'zone'].includes(t.k)) return t; }
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
  if (tl.kind === 'paint') {                       /* a trap's cells: each click adds one in order, or takes it off */
    const t = T.map.things.find(o => o.id === tl.id); if (!t) return;
    change(() => { const k = t.cells.findIndex(([cx, cy]) => cx === x && cy === y); if (k >= 0) t.cells.splice(k, 1); else t.cells.push([x, y]); }, true);
    return;
  }
  if (tl.kind === 'thing' && tl.p === 'haz') {
    change(() => { const o = SC.add(T.map, { k: 'haz', kind: tl.hz, x, y, cells: [[x, y]], zone: false, safe: false, clock: { every: 4, give: 1, first: 2 }, enter: null, harm: -1, chance: -1, warn: -1, stays: 6, tag: SC.newTag(T.map, 'trap') }); T.sel = o.id; });
    T.tool = { kind: 'paint', id: T.sel }; document.querySelectorAll('#sdMain .tool').forEach(b => b.setAttribute('aria-pressed', 'false'));
    $('sHint').textContent = 'Painting its cells: click each, in the order it goes. Esc when done.'; setTab('inspect'); return;
  }
  if (tl.kind === 'thing') {
    if (tl.p === 'door' && y < 1) { E.toast('A DOORWAY STANDS TWO TILES HIGH: NOT ON THE TOP ROW'); return; }
    const proto = Object.assign(JSON.parse(JSON.stringify(PROTO[tl.p] || { k: tl.p })), tl.it ? { it: tl.it } : {}, tl.c != null ? { c: tl.c } : {});
    if (proto.k === 'door') { const used = new Set(T.map.things.filter(t => t.k === 'door').map(t => t.link)); let l = proto.goal ? proto.link : 0; while (used.has(l) && l < 15) l++; proto.link = l; }
    if (proto.k === 'flow') { proto.w = Math.min(proto.w, T.map.w - x); proto.h = Math.min(proto.h, T.map.h - y); }
    if (proto.k === 'plat') proto.w = Math.min(proto.w, T.map.w - x);
    if (['gate', 'zone', 'current'].includes(proto.k)) { proto.w = Math.min(proto.w, T.map.w - x); proto.h = Math.min(proto.h, T.map.h - y); }
    if (['gate', 'plate', 'zone', 'current', 'switch'].includes(proto.k)) proto.tag = SC.newTag(T.map, proto.k === 'current' ? 'water' : proto.k);
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
  if (T.tab === 'trigs') { p.innerHTML = paneTrigs(); wireTrigs(); return; }
  if (T.tab === 'tune') { p.innerHTML = paneTune(); wireTune(p); return; }
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
  case 'gate': h += `<div class="row"><label>Across</label>${numIn('tFw', t.w, 1, T.map.w)}<label>Down</label>${numIn('tFh', t.h, 1, T.map.h)}</div>
      <div class="row"><label><input type="checkbox" id="tOpen"${t.open ? ' checked' : ''}> Open at the start</label></div>
      <p class="note">Iron bars across the box, solid while shut; they rise from the foot as it opens. A trigger's ON opens it, OFF shuts it. Something loose rests on the bars.</p>`; break;
  case 'plate': h += '<p class="note">Pressed while the caster stands on it, or something loose lies on it. Bind it in TRIGGERS: HOLD for on-while-pressed.</p>'; break;
  case 'zone': h += `<div class="row"><label>Across</label>${numIn('tFw', t.w, 1, T.map.w)}<label>Down</label>${numIn('tFh', t.h, 1, T.map.h)}</div>
      <p class="note">Never seen in the game. Pressed while the caster is anywhere in the box. Bind it in TRIGGERS.</p>`; break;
  case 'current': h += `<div class="row"><label>Across</label>${numIn('tFw', t.w, 1, T.map.w)}<label>Down</label>${numIn('tFh', t.h, 1, T.map.h)}</div>
      <div class="field"><label for="tWay">Runs</label><select id="tWay">${[['1,0', 'east'], ['-1,0', 'west'], ['0,-1', 'up'], ['0,1', 'down'], ['1,-1', 'up and east'], ['-1,-1', 'up and west'], ['1,1', 'down and east'], ['-1,1', 'down and west']].map(([v, l]) => opt(v, l, t.dx + ',' + t.dy)).join('')}</select></div>
      <div class="field"><label for="tSpd">Speed</label>${numIn('tSpd', t.speed, 0.1, 20, 0.1)}</div>
      <div class="row"><label><input type="checkbox" id="tPois"${t.poison ? ' checked' : ''}> Poisoned</label></div>
      <p class="note">Tiles a second. It carries whatever is loose (a key, gold, a life marked loose) its way and pushes the caster (TUNING: current). OFF at the start keeps it dry till a trigger's ON.</p>`; break;
  case 'haz': {
    const fam = SC.HAZ_FAMILY[t.kind];
    h += `<div class="field"><label for="tHz">Trap</label><select id="tHz">${Object.keys(SC.HAZ_FAMILY).map(k => opt(k, SC.HAZ_NAMES[k], t.kind)).join('')}</select></div>
      <p class="note">${esc(SC.FAMILY_SAYS[fam])}. ${t.kind === 'HZ_VOID' ? 'No spells while in it. ' : t.kind === 'HZ_SNARE' ? 'It may hold the caster a second. ' : ''}${(t.cells || []).length} cell${(t.cells || []).length === 1 ? '' : 's'}.</p>
      <div class="row"><button class="btn" id="tPaint" aria-pressed="${T.tool.kind === 'paint' && T.tool.id === t.id}">PAINT ITS CELLS</button><button class="btn" id="tClear">CLEAR THEM</button></div>
      ${fam === 'fall' || fam === 'line' ? `<div class="row"><label><input type="checkbox" id="tZone"${t.zone ? ' checked' : ''}> A zone: each time, one place in its cells, near the caster</label></div>` : ''}
      <h3>WHAT SETS IT OFF</h3>
      <div class="row"><label><input type="checkbox" id="tClk"${t.clock ? ' checked' : ''}> Its clock</label></div>
      ${t.clock ? `<div class="row"><label>Every</label>${numIn('tEv', t.clock.every, 0.5, 120, 0.5)}<label>give</label>${numIn('tGv', t.clock.give, 0, 60, 0.5)}<label>first</label>${numIn('tFi', t.clock.first, 0, 120, 0.5)}</div>` : ''}
      <div class="row"><label><input type="checkbox" id="tEnt"${t.enter ? ' checked' : ''}> A box walked into</label></div>
      ${t.enter ? `<div class="row"><label>From</label>${numIn('tE0', t.enter.x0, 0, T.map.w - 1)}${numIn('tE1', t.enter.y0, 0, T.map.h - 1)}<label>to</label>${numIn('tE2', t.enter.x1, 0, T.map.w - 1)}${numIn('tE3', t.enter.y1, 0, T.map.h - 1)}</div>` : ''}
      <p class="note">Neither: only a trigger's ON sets it off (a spread, till OFF). A trap OFF at the start does nothing till a trigger wakes it.</p>
      <h3>SETTINGS</h3>
      <div class="field"><label for="tHarm">Harm %</label>${numIn('tHarm', t.harm, -1, 100)}</div>
      <div class="field"><label for="tWarn">Warning s</label>${numIn('tWarn', t.warn, -1, 10, 0.1)}</div>
      ${fam === 'creep' || fam === 'cloud' ? `<div class="field"><label for="tStay">Stays s</label>${numIn('tStay', t.stays == null ? 6 : t.stays, 0.5, 60, 0.5)}</div>` : ''}
      ${t.kind === 'HZ_SNARE' ? `<div class="field"><label for="tCh">Holds %</label>${numIn('tCh', t.chance, -1, 100)}</div>` : ''}
      <div class="row"><label><input type="checkbox" id="tSafe"${t.safe ? ' checked' : ''}> May not kill (never below a quarter of health)</label></div>
      <p class="note">-1 for the game's own: harm a strike ${fam === 'fall' ? '30%' : fam === 'line' ? '25%' : '8% each bite'}; warning ${fam === 'fall' ? '1.2' : fam === 'line' ? '1.4' : fam === 'cloud' ? '1.0' : 'none'} s, never under 0.9.</p>
      ${fxPane(t)}`;
    break;
  }
  }
  if (['plat', 'flow', 'current', 'haz'].includes(t.k)) h += `<div class="row"><label><input type="checkbox" id="tStop"${t.stopped ? ' checked' : ''}> OFF at the start: still till a trigger's ON</label></div>`;
  if (['gold', 'key', 'life'].includes(t.k)) h += `<div class="row"><label><input type="checkbox" id="tLoose"${t.loose ? ' checked' : ''}> Loose: it falls, and water carries it</label></div>`;
  h += `<div class="field"><label for="tTag">Name</label><input type="text" id="tTag" maxlength="15" value="${esc(t.tag || '')}" placeholder="for the triggers"></div>`;
  return h + `<div class="row"><button class="btn" id="tDup">COPY</button><button class="btn warn" id="tDel">REMOVE</button></div>`;
}
/* 2e: the emitter of a trap that pours or drifts, or the censer's swing; PREVIEW runs it here on the map */
function fxPane(t) {
  if (!SC.FX_KINDS.has(t.kind)) return t.kind === 'HZ_ROCK' ? '<p class="note">A crack shows in the roof over each cell all the while; where it falls, rubble lies after (walked through).</p>' : '';
  const d = SC.FX_DEF[t.kind] || {}, e = t.emit || {}, own = (v, dv) => (v != null && v >= 0 ? v : dv == null ? -1 : dv), pv = T.pv && T.pv.id === t.id;
  let h = `<h3>${t.kind === 'HZ_CENSER' ? 'THE SWING' : 'THE EMITTER'}</h3><p class="note">${esc(SC.FX_SAYS[t.kind])}.</p>`;
  if (t.kind === 'HZ_CENSER') {
    const c = SC.censerGeom(t);
    h += `<div class="field"><label for="fLen">Chain</label>${numIn('fLen', t.len == null ? -1 : t.len, -1, 20, 0.5)}</div>
      <div class="field"><label for="fHang">Hang s</label>${numIn('fHang', own(t.hang, d.hang), 0, 30, 0.1)}</div>
      <div class="field"><label for="fSwing">Swing s</label>${numIn('fSwing', own(t.swing, d.swing), 0.2, 30, 0.05)}</div>
      <p class="note">Paint its two ends: the first cell and the second (PAINT ITS CELLS). Chain in tiles, -1 to reach its ends (now ${(c.r / 16).toFixed(1)}); three at the least, and the room clear along its arc (CHECKS says where it is not). Hang: still at each end; swing: a pass from one to the other. Crouched, the caster goes under a bowl at head height.</p>`;
  } else {
    const rate = t.kind === 'HZ_VOTIVE' ? (e.rate != null && e.rate >= 0 ? e.rate : -1) : own(e.rate, d.rate);
    h += `<div class="field"><label for="fRate">Rate /s</label>${numIn('fRate', rate, -1, 400, 1)}</div>
      <div class="field"><label for="fSpd">Speed</label>${numIn('fSpd', own(e.speed, d.speed), 0, 20, 0.1)}</div>
      <div class="field"><label for="fSpr">Spread &deg;</label>${numIn('fSpr', own(e.spread, d.spread), 0, 180, 1)}</div>
      <div class="field"><label for="fLife">Life s</label>${numIn('fLife', own(e.life, d.life), 0.1, 30, 0.1)}</div>
      ${t.kind === 'HZ_RELIQ' ? `<div class="field"><label for="fRad">Reach</label>${numIn('fRad', own(t.radial, d.radial), 0.5, 8, 0.5)}</div>` : ''}
      <div class="row"><label><input type="checkbox" id="fDirOwn"${t.dir == null ? ' checked' : ''}> Its own way</label>${t.dir == null ? '' : numIn('fDir', t.dir, -180, 360, 5)}</div>
      <p class="note">Particles a second (${t.kind === 'HZ_VOTIVE' ? '-1: 18 for each cell painted on its side' : 'its own ' + d.rate}), tiles a second, degrees either side of its way, seconds each lasts. The way in degrees: 0 right, 90 up, -90 down; its own is ${t.kind === 'HZ_VOTIVE' ? 'out along the candle as it tips' : t.kind === 'HZ_RELIQ' ? 'all round' : 'out of its open side'}.${t.kind === 'HZ_RELIQ' ? ' Reach: tiles from it each way (1, three by three).' : ''}</p>`;
  }
  return h + `<div class="row"><button class="btn play" id="fPrev" aria-pressed="${pv}">${pv ? 'STOP' : 'PREVIEW'}</button><button class="btn" id="fReset">ITS OWN</button></div>
    <p class="note">PREVIEW runs it here on the map, round and round (a warning, its turn, a rest); PLAY tries it in the game.</p>`;
}
function wireFx(t) {
  const on = (id, ev, fn) => { const el = $(id); if (el) el.addEventListener(ev, fn); };
  const d = SC.FX_DEF[t.kind] || {}, em = () => (t.emit = t.emit || { rate: d.rate == null ? -1 : d.rate, speed: d.speed, spread: d.spread, life: d.life });
  const n = (id, f) => on(id, 'change', e => change(() => f(+e.target.value)));
  n('fLen', v => { t.len = v < 0 ? null : Math.min(20, v); }); n('fHang', v => { t.hang = Math.max(0, Math.min(30, v)); }); n('fSwing', v => { t.swing = Math.max(0.2, Math.min(30, v)); });
  n('fRate', v => { em().rate = v < 0 ? -1 : Math.min(400, v); }); n('fSpd', v => { em().speed = Math.max(0, Math.min(20, v)); });
  n('fSpr', v => { em().spread = Math.max(0, Math.min(180, v)); }); n('fLife', v => { em().life = Math.max(0.1, Math.min(30, v)); });
  n('fRad', v => { t.radial = Math.max(0.5, Math.min(8, v)); });
  on('fDirOwn', 'change', e => change(() => { t.dir = e.target.checked ? null : 0; })); n('fDir', v => { t.dir = v; });
  on('fReset', 'click', () => change(() => { t.emit = null; t.dir = null; t.radial = null; t.len = null; t.hang = null; t.swing = null; }));
  on('fPrev', 'click', () => { if (T.pv && T.pv.id === t.id) previewStop(); else previewStart(t); pane(); });
}
function previewStart(t) {
  previewStop();
  const pv = { id: t.id, sim: SC.fxSim(T.map, t, 11), last: performance.now(), raf: 0 };
  const tick = now => {
    if (T.pv !== pv) return;
    const dt = Math.min(0.1, (now - pv.last) / 1000); pv.last = now;
    for (let left = dt; left > 1e-6; left -= 1 / 120) pv.sim.step(Math.min(1 / 120, left));
    draw(); pv.raf = requestAnimationFrame(tick);
  };
  T.pv = pv; pv.raf = requestAnimationFrame(tick);
}
function previewStop() { if (T.pv) cancelAnimationFrame(T.pv.raf); T.pv = null; }
function drawPreview(z) {                          /* the particles, the chain and the bowl, in the editor's pixels */
  const S = T.pv && T.pv.sim; if (!S) return;
  const k = z / 16, C = { wax: p => p.heat > 0.6 ? '#ffd27a' : p.heat > 0.3 ? '#e89a3a' : '#cbbf9e', vapour: () => 'rgba(160,200,120,0.7)', rime: () => '#d8f4ff', haze: () => 'rgba(199,155,240,0.85)', smoke: () => 'rgba(190,186,196,0.6)' };
  S.parts.forEach(p => { g.fillStyle = (C[p.kind] || C.smoke)(p); const s = Math.max(1.5, (p.kind === 'wax' ? 2 : p.kind === 'vapour' ? 3 : 1.5) * k); g.fillRect(p.x * k - s / 2, p.y * k - s / 2, s, s); });
  if (S.cen) {
    g.strokeStyle = '#b8bcc4'; g.lineWidth = Math.max(1, k); g.beginPath(); S.links.forEach((l, i) => (i ? g.lineTo(l.x * k, l.y * k) : g.moveTo(l.x * k, l.y * k))); g.stroke();
    g.fillStyle = '#5c5a66'; g.beginPath(); g.ellipse(S.ball[0] * k, (S.ball[1] + 2.5) * k, 6 * k, 3.5 * k, 0, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#f0742a'; g.fillRect((S.ball[0] - 4) * k, (S.ball[1] - 1) * k, 8 * k, Math.max(1, k));
  }
  if (S.kind === 'wax') {                          /* the candle, as far over as it is */
    const t = T.map.things.find(o => o.id === T.pv.id); if (!t) return;
    const a = S.tip * (0.6 + 0.25 * Math.min(4, S.vn)) * S.vdir, bx = (t.x * 16 + 8) * k, by = (t.y * 16 + 10) * k;
    g.strokeStyle = '#e6dcc0'; g.lineWidth = 3 * k; g.beginPath(); g.moveTo(bx, by); g.lineTo(bx + Math.sin(a) * 9 * k, by - Math.cos(a) * 9 * k); g.stroke();
  }
}
function drawFxMarks(t, z) {                       /* what a trap that pours or swings will do, while not previewed */
  const k = z / 16;
  if (t.kind === 'HZ_CENSER') {
    const c = SC.censerGeom(t);
    g.strokeStyle = 'rgba(232,154,58,0.85)'; g.setLineDash([4, 3]); g.lineWidth = 1.5; g.beginPath();
    for (let a = c.a0; a <= c.a1 + 1e-6; a += 0.03) { const x = (c.px + c.r * Math.sin(a)) * k, y = (c.py + c.r * Math.cos(a)) * k; a === c.a0 ? g.moveTo(x, y) : g.lineTo(x, y); }
    g.stroke(); g.setLineDash([]);
    if (!(T.pv && T.pv.id === t.id)) { const [bx, by] = SC.censerAt(c, 0); g.strokeStyle = '#b8bcc4'; g.lineWidth = 1; g.beginPath(); g.moveTo(c.px * k, c.py * k); g.lineTo(bx * k, by * k); g.stroke(); }
  } else if (t.kind === 'HZ_RELIQ') {
    const r = (t.radial != null && t.radial >= 0 ? t.radial : 1) + 0.5;
    g.strokeStyle = 'rgba(199,155,240,0.8)'; g.setLineDash([3, 3]); g.lineWidth = 1.5; g.beginPath(); g.arc((t.x + 0.5) * z, (t.y + 1 - 10 / 16) * z, r * z, 0, Math.PI * 2); g.stroke(); g.setLineDash([]);
  } else if (t.kind === 'HZ_VOTIVE') {
    const sides = (t.cells || []).reduce((a, [x]) => (x < t.x ? [a[0] + 1, a[1]] : x > t.x ? [a[0], a[1] + 1] : a), [0, 0]), d = sides[1] >= sides[0] ? 1 : -1;
    g.strokeStyle = '#f0742a'; g.lineWidth = 2; g.beginPath(); g.moveTo((t.x + 0.5) * z, (t.y + 0.3) * z); g.quadraticCurveTo((t.x + 0.5 + d) * z, (t.y + 0.1) * z, (t.x + 0.5 + d * 1.4) * z, (t.y + 1) * z); g.stroke();
  }
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
  on('tOpen', 'change', e => change(() => { t.open = e.target.checked; }));
  on('tPois', 'change', e => change(() => { t.poison = e.target.checked; }));
  on('tStop', 'change', e => change(() => { t.stopped = e.target.checked; }));
  on('tLoose', 'change', e => change(() => { t.loose = e.target.checked; }));
  on('tTag', 'change', e => change(() => { const v = e.target.value.trim().replace(/[^A-Za-z0-9_-]/g, '').slice(0, 15), was = t.tag;
    t.tag = v || undefined; (T.map.trigs || []).forEach(g => { if (g.src === was) g.src = v; g.acts.forEach(a => { if (a.tag === was) a.tag = v; }); }); }));
  on('tHz', 'change', e => change(() => { t.kind = e.target.value; }));
  on('tPaint', 'click', () => { if (T.tool.kind === 'paint') { setTool({ kind: 'select' }, T.selectBtn, ''); pane(); return; } T.tool = { kind: 'paint', id: t.id }; $('sHint').textContent = 'Painting its cells: click each, in the order it goes. Esc when done.'; pane(); });
  on('tClear', 'click', () => change(() => { t.cells = []; }));
  on('tZone', 'change', e => change(() => { t.zone = e.target.checked; }));
  on('tClk', 'change', e => change(() => { t.clock = e.target.checked ? { every: 4, give: 1, first: 2 } : null; }));
  n('tEv', v => { t.clock.every = Math.max(0.5, v); }); n('tGv', v => { t.clock.give = Math.max(0, v); }); n('tFi', v => { t.clock.first = Math.max(0, v); });
  on('tEnt', 'change', e => change(() => { t.enter = e.target.checked ? { x0: Math.max(0, t.x - 2), y0: Math.max(0, t.y - 2), x1: Math.min(T.map.w - 1, t.x + 2), y1: t.y } : null; }));
  ['tE0', 'tE1', 'tE2', 'tE3'].forEach((id, i) => n(id, v => { t.enter[['x0', 'y0', 'x1', 'y1'][i]] = v | 0; }));
  n('tHarm', v => { t.harm = Math.max(-1, Math.min(100, v | 0)); }); n('tWarn', v => { t.warn = v < 0 ? -1 : Math.min(10, v); });
  n('tStay', v => { t.stays = Math.max(0.5, v); }); n('tCh', v => { t.chance = Math.max(-1, Math.min(100, v | 0)); });
  on('tSafe', 'change', e => change(() => { t.safe = e.target.checked; }));
  on('tDup', 'click', () => change(() => { const o = SC.add(T.map, Object.assign(JSON.parse(JSON.stringify(t)), { x: Math.min(T.map.w - 1, t.x + 1) })); T.sel = o.id; }));
  on('tDel', 'click', () => removeSelected());
  if (t.k === 'haz') wireFx(t);
}
/* ---- TRIGGERS: what presses (a plate, a zone, a switch), how, and what it moves ---- */
const MODE_SAYS = { ONCE: 'the first press does it, for good', TOGGLE: 'each press does it, then undoes it', HOLD: 'done while pressed (a switch: while thrown), undone after', TIMED: 'done when pressed, undone so many seconds after it is let go' };
function paneTrigs() {
  const m = T.map, tr = m.trigs || (m.trigs = []);
  const srcs = m.things.filter(t => SC.SOURCES.includes(t.k) && t.tag), tgts = m.things.filter(t => SC.TARGETS.includes(t.k) && t.tag);
  let h = `<h2>TRIGGERS</h2><p class="note">Each one listens to a plate, an unseen zone or a switch, and moves gates, water, flows, platforms and traps by their names; or fires the crypt's own triggers on a thing of its level. A life lost puts everything back.</p>`;
  tr.forEach((g, i) => {
    h += `<div class="well" style="padding:8px;display:flex;flex-direction:column;gap:6px">
      <div class="row"><b>${i + 1}</b><select data-tsrc="${i}">${opt('', 'choose what presses', g.src)}${srcs.map(t => opt(t.tag, `${t.tag} (${t.k === 'zone' ? 'unseen zone' : t.k})`, g.src)).join('')}</select>
        <select data-tmode="${i}">${SC.MODES.map(md => opt(md, md.toLowerCase(), g.mode)).join('')}</select>${g.mode === 'TIMED' ? `<input type="number" data-tsecs="${i}" min="0.1" step="0.1" value="${g.secs || 3}">s` : ''}</div>
      <p class="note">${esc(MODE_SAYS[g.mode])}.</p>
      ${(g.acts || []).map((a, j) => `<div class="row"><select data-atag="${i}_${j}">${opt('', 'choose', a.crypt != null ? '#crypt' : a.tag)}${tgts.map(t => opt(t.tag, `${t.tag} (${t.k === 'haz' ? 'trap' : t.k === 'plat' ? 'platform' : t.k})`, a.tag)).join('')}${opt('#crypt', "the crypt's thing", a.crypt != null ? '#crypt' : '')}</select>
        ${a.crypt != null ? `<input type="number" data-acr="${i}_${j}" min="0" max="255" value="${a.crypt}">` : `<select data-averb="${i}_${j}">${['ON', 'OFF', 'FLIP'].map(v => opt(v, v === 'ON' ? 'on (opens, runs)' : v === 'OFF' ? 'off (shuts, stops)' : 'flip', a.verb)).join('')}</select>`}
        <button class="btn warn" data-adel="${i}_${j}">X</button></div>`).join('')}
      <div class="row"><button class="btn" data-aadd="${i}">ADD WHAT IT MOVES</button><button class="btn warn" data-tdel="${i}">REMOVE TRIGGER</button></div></div>`;
  });
  h += `<div class="row"><button class="btn" id="tgAdd">ADD A TRIGGER</button></div>`;
  if (!srcs.length) h += `<p class="note">Lay a pressure plate, an unseen zone or a switch first: they are given names, which triggers use.</p>`;
  return h;
}
function wireTrigs() {
  const m = T.map, q = sel => document.querySelectorAll(sel), ij = s => s.split('_').map(Number);
  const on = (id, ev, fn) => { const el = $(id); if (el) el.addEventListener(ev, fn); };
  on('tgAdd', 'click', () => change(() => { const s = m.things.find(t => SC.SOURCES.includes(t.k) && t.tag), tg = m.things.find(t => SC.TARGETS.includes(t.k) && t.tag);
    (m.trigs = m.trigs || []).push({ src: s ? s.tag : '', mode: 'HOLD', acts: [tg ? { tag: tg.tag, verb: 'ON' } : { tag: '', verb: 'ON' }] }); }));
  q('[data-tsrc]').forEach(el => el.addEventListener('change', () => change(() => { m.trigs[+el.dataset.tsrc].src = el.value; })));
  q('[data-tmode]').forEach(el => el.addEventListener('change', () => change(() => { const g = m.trigs[+el.dataset.tmode]; g.mode = el.value; if (g.mode === 'TIMED' && !g.secs) g.secs = 3; })));
  q('[data-tsecs]').forEach(el => el.addEventListener('change', () => change(() => { m.trigs[+el.dataset.tsecs].secs = Math.max(0.1, +el.value || 1); })));
  q('[data-atag]').forEach(el => el.addEventListener('change', () => change(() => { const [i, j] = ij(el.dataset.atag); m.trigs[i].acts[j] = el.value === '#crypt' ? { crypt: 0 } : { tag: el.value, verb: (m.trigs[i].acts[j].verb || 'ON') }; })));
  q('[data-averb]').forEach(el => el.addEventListener('change', () => change(() => { const [i, j] = ij(el.dataset.averb); m.trigs[i].acts[j].verb = el.value; })));
  q('[data-acr]').forEach(el => el.addEventListener('change', () => change(() => { const [i, j] = ij(el.dataset.acr); m.trigs[i].acts[j].crypt = Math.max(0, +el.value | 0); })));
  q('[data-adel]').forEach(el => el.addEventListener('click', () => change(() => { const [i, j] = ij(el.dataset.adel); m.trigs[i].acts.splice(j, 1); })));
  q('[data-aadd]').forEach(el => el.addEventListener('click', () => change(() => { m.trigs[+el.dataset.aadd].acts.push({ tag: '', verb: 'ON' }); })));
  q('[data-tdel]').forEach(el => el.addEventListener('click', () => change(() => { m.trigs.splice(+el.dataset.tdel, 1); })));
}

/* ---- TUNING: every number of how the caster moves and what harms it (TUNE[] in side_text.c), this passage's own ---- */
const TU = () => E.DATA.tune || [];
const tuneVal = (m, k) => (m.tune && m.tune[k.name] != null ? +m.tune[k.name] : k.def);
function tuneRows(m, prefix) {
  return TU().map((k, i) => { const v = tuneVal(m, k), changed = v !== k.def;
    return `<div class="tunerow"><label for="${prefix}${i}" title="${esc(k.what)}">${esc(k.name.replace(/_/g, ' '))}</label>
      <input type="range" id="${prefix}${i}" data-tu="${i}" min="${k.lo}" max="${k.hi}" step="${k.step}" value="${v}">
      <span class="num${changed ? ' tuned' : ''}" id="${prefix}v${i}">${+v.toFixed(3)}</span><button class="btn" data-tureset="${i}" title="Back to the game's ${k.def}"${changed ? '' : ' disabled'}>&#8634;</button></div>`; }).join('');
}
function paneTune() {
  return `<h2>TUNING</h2><p class="note">This passage's own numbers; the rest are the game's. Hover a name for what it is. In PLAY the same sliders change the game as it runs (TUNING, top right).</p>
    <div class="tune">${tuneRows(T.map, 'tu')}</div>
    <div class="row"><button class="btn" id="tuAll">ALL BACK TO THE GAME'S</button><button class="btn" id="tuCopy">COPY AS THE GAME'S DEFAULTS</button></div>
    <p class="note">The game's defaults: COPY gives the table for TUNE[] in src/crawl/game/side_text.c, with these numbers, so every passage plays so.</p>`;
}
function setTune(i, v, live) {
  const k = TU()[i]; if (!k) return;
  v = Math.max(k.lo, Math.min(k.hi, +v));
  const apply = () => { T.map.tune = T.map.tune || {}; if (v === k.def) delete T.map.tune[k.name]; else T.map.tune[k.name] = v; };
  if (live) { apply(); T.dirty = true; badge(); draft(); } else change(apply, true);
  const f = $('playFrame'); if (f && !$('playLayer').hidden && f.contentWindow) f.contentWindow.postMessage({ type: 'hvs-tune', k: i, v }, location.origin && location.origin !== 'null' ? location.origin : '*');
}
function wireTune(root) {
  root.querySelectorAll('[data-tu]').forEach(el => {
    el.addEventListener('input', () => { const i = +el.dataset.tu, v = +el.value; root.querySelector('#' + el.id.replace(/(\d+)$/, 'v$1')).textContent = +v.toFixed(3); setTune(i, v, true); });
    el.addEventListener('change', () => { T.hist.push(snap()); T.fut = []; if (root.id === 'sdPane') pane(); });
  });
  root.querySelectorAll('[data-tureset]').forEach(b => b.addEventListener('click', () => { const i = +b.dataset.tureset; setTune(i, TU()[i].def, false); if (root.id === 'sdPane') pane(); else drawer(); }));
  const all = root.querySelector('#tuAll'); if (all) all.addEventListener('click', () => change(() => { T.map.tune = {}; }));
  const cp = root.querySelector('#tuCopy'); if (cp) cp.addEventListener('click', () => {
    const rows = TU().map(k => { const v = tuneVal(T.map, k), f = x => (Number.isInteger(x) ? x.toFixed(1) : String(+x.toFixed(4))) + 'f';
      return `    { "${k.name}", ${f(v)}, ${f(k.lo)}, ${f(k.hi)}, ${f(k.step)}, "${k.what}" },`; });
    E.showText("THE GAME'S TUNING", "Paste over the rows of TUNE[] in src/crawl/game/side_text.c (the same order), then build: every passage plays with these unless its own TUNE lines say otherwise.", rows.join('\n'), 'side-tuning.txt');
  });
}
function drawer() {                                    /* the tuning beside the game while a passage plays */
  const d = $('sdTuneDrawer'); if (!d || d.hidden) return;
  d.innerHTML = `<div class="row" style="justify-content:space-between"><h2>TUNING</h2><button class="btn" id="tuShut">CLOSE</button></div><div class="tune">${tuneRows(T.map, 'tp')}</div>
    <p class="note">Each change goes into the game at once, and into this passage: SAVE keeps it.</p>`;
  wireTune(d); $('tuShut').addEventListener('click', () => { d.hidden = true; });
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
  if (!on) previewStop();
  $('sdMain').hidden = !on;
  document.querySelector('#app > .main:not(#sdMain)').hidden = on;
  ['bLattice', 'bLanes'].forEach(id => { const b = $(id); if (b) b.disabled = on; });
  $('bExport').textContent = on ? 'EXPORT TEXT' : 'EXPORT C';
  document.querySelector('label[for="mapLevel"]').textContent = on ? 'Played on' : 'Level';
}
SideUI.open = (id, body, example) => {
  previewStop();
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
  const s = E.DATA.sides.find(x => x.n === n), r = SC.parse(s.text, E.DATA.kinds.map(k => k.id), (E.DATA.tune || []).map(k => k.name));
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
  $('sdTuneBtn').hidden = false;
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
  const st = document.createElement('style');                    /* the sliders, in the tab and in the drawer over the game */
  st.textContent = '.tune { display: flex; flex-direction: column; gap: 4px; } .tunerow { display: grid; grid-template-columns: 7.5em minmax(0, 1fr) 3.4em auto; gap: 6px; align-items: center; font-size: 17px; }'
    + ' .tunerow .tuned { color: var(--gold); } .tunerow .btn { padding: 2px 6px; }'
    + ' #sdTuneDrawer { position: absolute; right: 10px; top: 46px; bottom: 10px; width: min(380px, 92vw); overflow: auto; padding: 10px; z-index: 60; display: flex; flex-direction: column; gap: 8px; }'
    + ' #sdTuneDrawer[hidden] { display: none; } #playLayer { position: fixed; }';
  document.head.appendChild(st);
  const btn = document.createElement('button'); btn.className = 'btn'; btn.id = 'sdTuneBtn'; btn.textContent = 'TUNING'; btn.hidden = true;
  $('bPlayClose').parentNode.insertBefore(btn, $('bPlayClose'));
  const dr = document.createElement('div'); dr.id = 'sdTuneDrawer'; dr.className = 'slab'; dr.hidden = true; $('playLayer').appendChild(dr);
  btn.addEventListener('click', () => { dr.hidden = !dr.hidden; drawer(); });
  $('bPlayClose').addEventListener('click', () => { btn.hidden = true; dr.hidden = true; if (T.map) refresh(); });
  let d = null;
  try { d = JSON.parse(localStorage.getItem('cme.sdraft') || 'null'); } catch (e) {}
  T.draft = d && d.map ? d : null;
};
SideUI.draftWaiting = () => T.draft;
SideUI.takeDraft = () => { const d = T.draft; T.draft = null; if (!d) return; SideUI.open(d.id, { map: d.map }, false); T.dirty = true; badge(); };
})();
