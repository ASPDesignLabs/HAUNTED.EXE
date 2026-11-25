/* The world editor's side passages (docs/PLAN-side.md, step 9): a passage as the editor keeps it, its text for the game both ways
 * (the text side_text.c's sd_read reads, SIDE 1), and the checks that mirror that reader and what the game does with a passage.
 * No drawing here, so node can test it (tools/mapedit/test.js). */
(function (root) {
'use strict';
const LIM = { minW: 20, maxW: 256, minH: 10, maxH: 64, things: 160, platW: 8, links: 16, evThings: 256 };
const TILES = '#%,.=^~';                          /* F is a falling block: a thing, laid with the dark behind it */
const SOLID = new Set(['#', '%']);
const TILE_NAMES = { '#': 'Stone', '%': 'Brick', ',': 'Back wall', '.': 'The dark', '=': 'Ledge', '^': 'Spikes', '~': 'Standing poison' };
const KINDS = ['door', 'plat', 'fall', 'flow', 'mob', 'gold', 'key', 'life', 'switch', 'torch'];
const OPS = { door: 'DOOR', plat: 'PLAT', fall: 'FALL', flow: 'FLOW', mob: 'MOB', gold: 'GOLD', key: 'KEY', life: 'LIFE', switch: 'SWITCH', torch: 'TORCH' };
const GOLD = ['IT_COINS', 'IT_CHALICE', 'IT_GEMS', 'IT_CROWN', 'IT_CHEST'];
const RANKS = ['RK_I', 'RK_II', 'RK_III', 'RK_IV', 'RK_V', 'RK_GILDED'];
const KEYS = ['Gold', 'Silver', 'Crimson'];
const FALL_DELAY = 0.45, FALL_BACK = 4.0;         /* the game's own for an F in a row */

function blank(w, h) {
    w = w || 60; h = h || 16;
    const m = { kind: 'side', v: 1, name: 'A side passage', num: 1, level: 3, w, h, grid: new Array(w * h).fill('.'), things: [], next: 1 };
    for (let x = 0; x < w; x++) { m.grid[(h - 3) * w + x] = '#'; m.grid[(h - 2) * w + x] = '#'; m.grid[(h - 1) * w + x] = '#'; m.grid[x] = '#'; }
    for (let y = 0; y < h; y++) { m.grid[y * w] = '#'; m.grid[y * w + w - 1] = '#'; }
    for (let y = 1; y < h - 3; y++) for (let x = 1; x < w - 1; x++) m.grid[y * w + x] = ',';
    add(m, { k: 'door', x: 3, y: h - 4, link: 0, goal: false });
    add(m, { k: 'door', x: w - 4, y: h - 4, link: 1, goal: true });
    return m;
}
function add(m, t) { t.id = m.next++; m.things.push(t); return t; }
const at = (m, x, y) => x < 0 || y < 0 || x >= m.w || y >= m.h ? (y >= m.h ? '.' : '#') : m.grid[y * m.w + x];
function resize(m, w, h, fromBottom) {            /* kept against the top, or against the foot: what is cut off is gone */
    w = Math.max(LIM.minW, Math.min(LIM.maxW, w | 0)); h = Math.max(LIM.minH, Math.min(LIM.maxH, h | 0));
    const dy = fromBottom ? h - m.h : 0, g = new Array(w * h).fill(',');
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) { const sy = y - dy; if (sy >= 0 && sy < m.h && x < m.w) g[y * w + x] = m.grid[sy * m.w + x]; }
    m.things.forEach(t => { t.y += dy; });
    m.things = m.things.filter(t => t.x >= 0 && t.y >= 0 && t.x < w && t.y < h);
    m.w = w; m.h = h; m.grid = g;
}

/* ---- the text, out: what sd_read reads. Seconds carry a decimal (3.0), tiles none, so a passage written by hand reads back the same ---- */
const sec = v => { const n = +v || 0; return Number.isInteger(n) ? n.toFixed(1) : String(+n.toFixed(3)); };
const num = v => String(+(+v || 0).toFixed(3));
const defaultFall = t => t.k === 'fall' && +t.delay === FALL_DELAY && +t.back === FALL_BACK;
function line(t) {
    const p = [OPS[t.k], t.x, t.y];
    switch (t.k) {
    case 'door': p.push(t.link | 0); if (t.goal) p.push('GOAL'); break;
    case 'plat': p.push(t.w | 0, t.dx | 0, t.dy | 0, num(t.dist), sec(t.speed)); if (t.loop) p.push('LOOP'); if (+t.wait) p.push('WAIT', sec(t.wait)); if (+t.at) p.push('AT', num(t.at)); break;
    case 'fall': p.push('DELAY', sec(t.delay), 'BACK', sec(t.back)); break;
    case 'flow': p.push(t.w | 0, t.h | 0, sec(t.on), sec(t.off)); if (+t.at) p.push('AT', sec(t.at)); break;
    case 'mob': p.push(t.kind || 'ANY'); if (t.rank && t.rank !== -1 && t.rank !== '-1') p.push(t.rank); break;
    case 'gold': p.push(t.it || 'IT_COINS'); break;
    case 'key': p.push(t.c | 0); break;
    case 'switch': p.push(t.thing == null ? -1 : t.thing | 0); if (t.toggle) p.push('TOGGLE'); break;
    default: break;
    }
    return p.join(' ');
}
function exportText(m) {
    const out = ['SIDE 1', `NAME "${String(m.name || '').toUpperCase().replace(/["\\]/g, '').slice(0, 39)}"`, `SIZE ${m.w} ${m.h}`];
    const falls = new Set(m.things.filter(t => defaultFall(t) && at(m, t.x, t.y) === '.').map(t => t.x + ',' + t.y));
    for (let y = 0; y < m.h; y++) {
        let r = '';
        for (let x = 0; x < m.w; x++) r += falls.has(x + ',' + y) ? 'F' : m.grid[y * m.w + x];
        out.push(`ROW "${r}"`);
    }
    m.things.forEach(t => { if (!(defaultFall(t) && falls.has(t.x + ',' + t.y))) out.push(line(t)); });
    return out.join('\n') + '\n';
}
/* as a C string, for SIDE_TEXT[n] in src/crawl/game/side_levels.c */
function exportC(m) {
    const text = exportText(m), n = m.num | 0;
    const lines = text.split('\n').filter(l => l.length).map(l => '    "' + l.replace(/\\/g, '\\\\').replace(/"/g, '\\"') + '\\n"');
    lines[lines.length - 1] += ',';
    return `    /* ${n}: ${String(m.name || '').replace(/\*\//g, '')}, drawn in the World Editor (docs/PLAN-side.md); it must be SIDE_TEXT[${n}] */\n` + lines.join('\n') + '\n';
}

/* ---- the text, in: sd_read's rules, the same refusals with their lines ---- */
function words(s) {
    const w = []; let i = 0;
    while (i < s.length) {
        while (i < s.length && /\s/.test(s[i])) i++;
        if (i >= s.length || s[i] === ';') break;
        if (s[i] === '"') { const j = s.indexOf('"', i + 1); w.push('"' + s.slice(i + 1, j < 0 ? s.length : j)); i = j < 0 ? s.length : j + 1; }
        else { let j = i; while (j < s.length && !/[\s;]/.test(s[j])) j++; w.push(s.slice(i, j)); i = j; }
    }
    return w;
}
function parse(text, kindIds) {
    const m = { kind: 'side', v: 1, name: 'A SIDE PASSAGE', num: 1, level: 3, w: 0, h: 0, grid: [], things: [], next: 1 };
    let version = 0, rows = 0, ln = 0;
    const bad = msg => ({ error: msg, line: ln });
    const lines = String(text).split('\n');
    for (const raw of lines) {
        ln++;
        const w = words(raw);
        if (!w.length) continue;
        const op = w[0], iv = w.map(v => parseInt(v, 10)), fv = w.map(v => parseFloat(v));
        if (op === 'SIDE') { version = iv[1]; if (version !== 1) return bad(`a side passage of version ${w[1]}: this game reads 1`); continue; }
        if (!version) return bad('SIDE 1 must come first');
        if (op === 'NAME') { if (!w[1] || w[1][0] !== '"') return bad('NAME wants a "name"'); m.name = w[1].slice(1, 40); continue; }
        if (op === 'SIZE') {
            if (!(iv[1] >= LIM.minW && iv[1] <= LIM.maxW && iv[2] >= LIM.minH && iv[2] <= LIM.maxH)) return bad(`SIZE wants ${LIM.minW} to ${LIM.maxW} across and ${LIM.minH} to ${LIM.maxH} down`);
            m.w = iv[1]; m.h = iv[2]; m.grid = new Array(m.w * m.h).fill('.'); continue;
        }
        if (!m.w) return bad('SIZE must come before the rest');
        if (op === 'ROW') {
            const r = w[1] && w[1][0] === '"' ? w[1].slice(1) : null;
            if (r == null || r.length !== m.w) return bad(`a ROW is "...", ${m.w} letters long`);
            if (rows >= m.h) return bad(`more ROWs than SIZE gives (${m.h})`);
            for (let x = 0; x < m.w; x++) {
                let c = r[x];
                if (!'#%,.=^~F'.includes(c)) return bad('a letter the game has no tile for: ' + c);
                if (c === 'F') { if (m.things.length >= LIM.things) return bad(`more than ${LIM.things} things`); add(m, { k: 'fall', x, y: rows, delay: FALL_DELAY, back: FALL_BACK }); c = '.'; }
                m.grid[rows * m.w + x] = c;
            }
            rows++; continue;
        }
        const k = Object.keys(OPS).find(key => OPS[key] === op);
        if (!k) return bad("a word the game doesn't know: " + op);
        const need = { door: 4, plat: 8, fall: 3, flow: 7, mob: 4, gold: 4, key: 4, life: 3, switch: 4, torch: 3 }[k];
        if (w.length < need) return bad(`${op} wants ${need - w.length} more`);
        if (m.things.length >= LIM.things) return bad(`more than ${LIM.things} things`);
        const t = { k, x: iv[1], y: iv[2] };
        if (!(t.x >= 0 && t.y >= 0 && t.x < m.w && t.y < m.h)) return bad(op + ' outside the map');
        const rest = w.slice(need);
        switch (k) {
        case 'door':
            t.link = iv[3]; t.goal = rest.includes('GOAL');
            if (!(t.link >= 0 && t.link < LIM.links)) return bad("a doorway's link is 0 to 15");
            if (t.y < 1) return bad('a doorway stands two tiles high: not on the top row');
            break;
        case 'plat':
            Object.assign(t, { w: iv[3], dx: iv[4], dy: iv[5], dist: fv[6], speed: fv[7], loop: false, wait: 0, at: 0 });
            if (!(t.w >= 1 && t.w <= LIM.platW)) return bad('a platform is 1 to 8 tiles wide');
            if (![-1, 0, 1].includes(t.dx) || ![-1, 0, 1].includes(t.dy) || (!t.dx && !t.dy)) return bad("a platform's way is -1, 0 or 1 each, not both 0");
            if (!(t.dist > 0 && t.speed > 0)) return bad('a platform goes some way, at some speed');
            for (let i = 0; i < rest.length; i++) { if (rest[i] === 'LOOP') t.loop = true; else if (rest[i] === 'WAIT') t.wait = parseFloat(rest[++i]) || 0; else if (rest[i] === 'AT') t.at = parseFloat(rest[++i]) || 0; }
            break;
        case 'fall':
            t.delay = FALL_DELAY; t.back = FALL_BACK;
            for (let i = 0; i + 1 < rest.length; i++) { if (rest[i] === 'DELAY') t.delay = parseFloat(rest[++i]); else if (rest[i] === 'BACK') t.back = parseFloat(rest[++i]); }
            break;
        case 'flow':
            Object.assign(t, { w: iv[3], h: iv[4], on: fv[5], off: fv[6], at: 0 });
            if (!(t.w >= 1 && t.h >= 1 && t.x + t.w <= m.w && t.y + t.h <= m.h)) return bad("a flow's box is outside the map");
            if (!(t.on >= 0.5 && t.off >= 0.5)) return bad('a flow is full and dry half a second each at the least');
            for (let i = 0; i + 1 < rest.length; i++) if (rest[i] === 'AT') t.at = parseFloat(rest[++i]) || 0;
            break;
        case 'mob':
            t.kind = w[3]; t.rank = w[4] && w[4] !== '-1' ? w[4] : -1;
            if (t.kind !== 'ANY' && kindIds && !kindIds.includes(t.kind)) return bad('not a kind: ' + t.kind);
            if (t.rank !== -1 && !RANKS.includes(t.rank)) return bad('not a rank: ' + t.rank);
            break;
        case 'gold': t.it = w[3]; if (!GOLD.includes(t.it)) return bad('not gold: ' + t.it); break;
        case 'key': t.c = iv[3]; if (!(t.c >= 0 && t.c <= 2)) return bad("a key's colour is 0 to 2"); break;
        case 'switch':
            t.thing = iv[3]; t.toggle = rest.includes('TOGGLE');
            if (!(t.thing >= -1 && t.thing < LIM.evThings)) return bad(`a switch's thing is -1 or 0 to ${LIM.evThings - 1}`);
            break;
        default: break;
        }
        add(m, t);
    }
    if (!version) return bad('no SIDE 1');
    if (rows !== m.h) return bad(`${rows} ROWs, and SIZE says ${m.h}`);
    if (!m.things.some(t => t.k === 'door')) return bad('no doorway: there is no way in');
    return { map: m };
}

/* ---- the checks: errors the game would refuse or break on, warnings for what plays badly ---- */
function check(m) {
    const out = [], err = (msg, t) => out.push({ sev: 'error', msg, at: t ? [t.x, t.y] : null, id: t ? t.id : null }),
          warn = (msg, t) => out.push({ sev: 'warn', msg, at: t ? [t.x, t.y] : null, id: t ? t.id : null });
    const doors = m.things.filter(t => t.k === 'door');
    if (!doors.length) err('No doorway: there is no way in.');
    if (doors.length && !doors.some(t => t.goal)) warn('No goal doorway: the passage can be left but never finished (no life back).');
    if (m.things.length > LIM.things) err(`${m.things.length} things: the game holds ${LIM.things}.`);
    const links = {};
    doors.forEach(t => {
        if (t.y < 1) err('A doorway on the top row: it stands two tiles high.', t);
        if (SOLID.has(at(m, t.x, t.y)) || SOLID.has(at(m, t.x, t.y - 1))) err('A doorway set in stone: nobody can stand before it.', t);
        const b = at(m, t.x, t.y + 1);
        if (!SOLID.has(b) && b !== '=') warn('A doorway with nothing under it: it can be reached only in a fall.', t);
        if (links[t.link]) warn(`Two doorways share link ${t.link}: the way in from the crypt takes the first.`, t);
        links[t.link] = 1;
    });
    m.things.forEach(t => {
        if (t.x < 0 || t.y < 0 || t.x >= m.w || t.y >= m.h) { err('A thing outside the map.', t); return; }
        if (['gold', 'key', 'life', 'mob', 'switch'].includes(t.k) && SOLID.has(at(m, t.x, t.y))) warn('Set in stone: it cannot be reached.', t);
        if (t.k === 'plat') {
            if (!(t.w >= 1 && t.w <= LIM.platW)) err('A platform is 1 to 8 tiles wide.', t);
            if (!t.dx && !t.dy) err('A platform with no way to go.', t);
            if (!(t.dist > 0) || !(t.speed > 0)) err('A platform goes some way, at some speed.', t);
            const n = Math.hypot(t.dx, t.dy) || 1;
            for (let s = 0; s <= t.dist; s += 0.5) for (let k = 0; k < t.w; k++) {
                const x = Math.floor(t.x + k + t.dx / n * s + 0.01), y = Math.floor(t.y + t.dy / n * s + 0.01);
                if (SOLID.has(at(m, x, y))) { warn('A platform\'s way runs into stone: it passes through it.', t); s = 1e9; break; }
            }
        }
        if (t.k === 'flow') {
            if (!(t.w >= 1 && t.h >= 1 && t.x + t.w <= m.w && t.y + t.h <= m.h)) err("A flow's box runs off the map.", t);
            if (!(t.on >= 0.5 && t.off >= 0.5)) err('A flow is full and dry half a second each at the least.', t);
        }
        if (t.k === 'fall' && (!(t.delay >= 0) || !(t.back > 0))) err('A falling block wants its shaking and its time back.', t);
        if (t.k === 'switch' && !(t.thing >= -1 && t.thing < LIM.evThings)) err(`A switch's thing is -1 (none) or 0 to ${LIM.evThings - 1}.`, t);
        if (t.k === 'switch' && t.thing === -1) warn('A switch that names no crypt thing: it is thrown, and nothing in the crypt moves.', t);
    });
    if (!(m.num >= 0 && m.num <= 7)) err("The passage's number is 0 to 7 (the game's SIDE_TEXT).");
    if (m.num === 0) warn('Number 0 is the test passage built in: this one takes its place while it plays.');
    let open = 0; for (let x = 0; x < m.w; x++) if (!SOLID.has(at(m, x, 0))) open++;
    if (open) warn('The top row is open in places: a jump there meets nothing, as if it were stone.');
    return out;
}
function label(t, kinds) {
    switch (t.k) {
    case 'door': return (t.goal ? 'The goal doorway' : 'A doorway') + ', link ' + t.link;
    case 'plat': return `A platform, ${t.w} wide, ${t.dist} tiles ${t.dx ? (t.dx > 0 ? 'east' : 'west') : ''}${t.dx && t.dy ? ' and ' : ''}${t.dy ? (t.dy > 0 ? 'down' : 'up') : ''}`;
    case 'fall': return 'A falling block';
    case 'flow': return `A flow of poison, ${t.w} by ${t.h}`;
    case 'mob': { const k = kinds && kinds.find(x => x.id === t.kind); return t.kind === 'ANY' ? "A creature: the run's choice" : 'A creature: ' + (k ? k.name.toLowerCase() : t.kind); }
    case 'gold': return 'Gold: ' + t.it.slice(3).toLowerCase();
    case 'key': return 'The ' + KEYS[t.c].toLowerCase() + ' key';
    case 'life': return 'A life';
    case 'switch': return 'A switch' + (t.thing >= 0 ? ', crypt thing ' + t.thing : '');
    case 'torch': return 'A torch';
    default: return t.k;
    }
}

const API = { LIM, TILES, TILE_NAMES, SOLID, KINDS, GOLD, RANKS, KEYS, FALL_DELAY, FALL_BACK, blank, add, at, resize, exportText, exportC, parse, check, label };
if (typeof module !== 'undefined' && module.exports) module.exports = API; else root.SideCore = API;
})(typeof window !== 'undefined' ? window : globalThis);
