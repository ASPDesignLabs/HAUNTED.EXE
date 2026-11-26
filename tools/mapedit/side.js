/* The world editor's side passages (docs/PLAN-side.md, step 9): a passage as the editor keeps it, its text for the game both ways
 * (the text side_text.c's sd_read reads, SIDE 1), and the checks that mirror that reader and what the game does with a passage.
 * No drawing here, so node can test it (tools/mapedit/test.js). */
(function (root) {
'use strict';
const LIM = { minW: 20, maxW: 256, minH: 10, maxH: 64, things: 160, platW: 8, links: 16, evThings: 256 };
const TILES = '#%,.=^~';                          /* F is a falling block: a thing, laid with the dark behind it */
const SOLID = new Set(['#', '%']);
const TILE_NAMES = { '#': 'Stone', '%': 'Brick', ',': 'Back wall', '.': 'The dark', '=': 'Ledge', '^': 'Spikes', '~': 'Standing poison' };
const KINDS = ['door', 'plat', 'fall', 'flow', 'mob', 'gold', 'key', 'life', 'switch', 'torch', 'gate', 'plate', 'zone', 'current', 'haz'];
const OPS = { door: 'DOOR', plat: 'PLAT', fall: 'FALL', flow: 'FLOW', mob: 'MOB', gold: 'GOLD', key: 'KEY', life: 'LIFE', switch: 'SWITCH', torch: 'TORCH',
              gate: 'GATE', plate: 'PLATE', zone: 'ZONE', current: 'CURRENT', haz: 'HAZ' };
const NEED = { door: 4, plat: 8, fall: 3, flow: 7, mob: 4, gold: 4, key: 4, life: 3, switch: 4, torch: 3, gate: 5, plate: 3, zone: 5, current: 8, haz: 4 };
/* the crypt's traps in side view, each in the shape of its own (side_text.c's sd_haz_family): their names as the game shows them */
const HAZ_FAMILY = { HZ_ROCK: 'fall', HZ_TIMBER: 'fall', HZ_GLASS: 'fall', HZ_CRYSTAL: 'fall',
  HZ_BILE: 'creep', HZ_SILT: 'creep', HZ_DAMP: 'creep', HZ_SLUICE: 'creep', HZ_COLD: 'creep', HZ_DCOLD: 'creep', HZ_SAINT: 'creep', HZ_TITHE: 'creep',
  HZ_VOID: 'cloud', HZ_ASH: 'cloud', HZ_BREATH: 'cloud', HZ_RELIQ: 'cloud', HZ_MIRROR: 'cloud',
  HZ_FIRE: 'line', HZ_VOTIVE: 'line', HZ_FISSURE: 'line', HZ_LODE: 'line', HZ_GALV: 'line', HZ_CART: 'line', HZ_CENSER: 'line', HZ_SNARE: 'line' };
const HAZ_NAMES = { HZ_ROCK: 'Rockfall', HZ_TIMBER: 'Falling timber', HZ_GLASS: 'Glass shards', HZ_CRYSTAL: 'Crystal fall', HZ_BILE: 'Bile', HZ_SILT: 'Silt',
  HZ_DAMP: 'Damp rot', HZ_SLUICE: 'The sluice', HZ_COLD: 'The cold vault', HZ_DCOLD: 'Deep cold', HZ_SAINT: "The saint's tears", HZ_TITHE: 'The tithe',
  HZ_VOID: 'The void', HZ_ASH: 'Ash', HZ_BREATH: "The embalmers' breath", HZ_RELIQ: 'The reliquary', HZ_MIRROR: 'The black mirror',
  HZ_FIRE: 'Hellrent', HZ_VOTIVE: 'Votive wax', HZ_FISSURE: 'Fissure breath', HZ_LODE: 'The lodestone', HZ_GALV: 'Galvanic arc', HZ_CART: 'The cart',
  HZ_CENSER: 'The censer', HZ_SNARE: 'The snare' };
const FAMILY_SAYS = { fall: 'dust and a shadow, then it falls on its cells; rubble a while', line: 'its cells glow in the order painted, then burst together; embers',
  creep: 'it spreads over its cells in their order, stays, and drains back', cloud: 'it gathers over its cells, hangs a while, and lifts' };
const MODES = ['ONCE', 'TOGGLE', 'HOLD', 'TIMED'];
const SOURCES = ['plate', 'zone', 'switch'], TARGETS = ['gate', 'current', 'flow', 'plat', 'haz'];
const TAG_RE = /^[A-Za-z0-9_-]{1,15}$/;
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
function newTag(m, base) { const used = new Set(m.things.map(t => t.tag).filter(Boolean)); let n = 1; while (used.has(base + n)) n++; return base + n; }
const at = (m, x, y) => x < 0 || y < 0 || x >= m.w || y >= m.h ? (y >= m.h ? '.' : '#') : m.grid[y * m.w + x];
function resize(m, w, h, fromBottom) {            /* kept against the top, or against the foot: what is cut off is gone */
    w = Math.max(LIM.minW, Math.min(LIM.maxW, w | 0)); h = Math.max(LIM.minH, Math.min(LIM.maxH, h | 0));
    const dy = fromBottom ? h - m.h : 0, g = new Array(w * h).fill(',');
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) { const sy = y - dy; if (sy >= 0 && sy < m.h && x < m.w) g[y * w + x] = m.grid[sy * m.w + x]; }
    m.things.forEach(t => { t.y += dy; if (t.cells) t.cells.forEach(c => { c[1] += dy; }); if (t.enter) { t.enter.y0 += dy; t.enter.y1 += dy; } });
    m.things = m.things.filter(t => t.x >= 0 && t.y >= 0 && t.x < w && t.y < h);
    m.things.forEach(t => { if (t.cells) t.cells = t.cells.filter(([x, y]) => x >= 0 && y >= 0 && x < w && y < h); });
    m.w = w; m.h = h; m.grid = g;
}

/* ---- the text, out: what sd_read reads. Seconds carry a decimal (3.0), tiles none, so a passage written by hand reads back the same ---- */
const sec = v => { const n = +v || 0; return Number.isInteger(n) ? n.toFixed(1) : String(+n.toFixed(3)); };
const num = v => String(+(+v || 0).toFixed(3));
const defaultFall = t => t.k === 'fall' && +t.delay === FALL_DELAY && +t.back === FALL_BACK;
function line(t) {
    const p = t.k === 'haz' ? [OPS[t.k], t.kind, t.x, t.y] : [OPS[t.k], t.x, t.y];
    switch (t.k) {
    case 'door': p.push(t.link | 0); if (t.goal) p.push('GOAL'); break;
    case 'plat': p.push(t.w | 0, t.dx | 0, t.dy | 0, num(t.dist), sec(t.speed)); if (t.loop) p.push('LOOP'); if (+t.wait) p.push('WAIT', sec(t.wait)); if (+t.at) p.push('AT', num(t.at)); break;
    case 'fall': p.push('DELAY', sec(t.delay), 'BACK', sec(t.back)); break;
    case 'flow': p.push(t.w | 0, t.h | 0, sec(t.on), sec(t.off)); if (+t.at) p.push('AT', sec(t.at)); break;
    case 'mob': p.push(t.kind || 'ANY'); if (t.rank && t.rank !== -1 && t.rank !== '-1') p.push(t.rank); break;
    case 'gold': p.push(t.it || 'IT_COINS'); break;
    case 'key': p.push(t.c | 0); break;
    case 'switch': p.push(t.thing == null ? -1 : t.thing | 0); if (t.toggle) p.push('TOGGLE'); break;
    case 'gate': p.push(t.w | 0, t.h | 0); if (t.open) p.push('OPEN'); break;
    case 'zone': p.push(t.w | 0, t.h | 0); break;
    case 'current': p.push(t.w | 0, t.h | 0, t.dx | 0, t.dy | 0, sec(t.speed)); if (t.poison) p.push('POISON'); break;
    case 'haz':
        if (t.zone) p.push('ZONE'); if (t.safe) p.push('SAFE');
        if (t.clock) p.push('CLOCK', sec(t.clock.every), sec(t.clock.give), sec(t.clock.first));
        if (t.enter) p.push('ENTER', t.enter.x0, t.enter.y0, t.enter.x1, t.enter.y1);
        if (t.harm >= 0) p.push('HARM', t.harm | 0); if (t.chance >= 0) p.push('CHANCE', t.chance | 0); if (t.warn >= 0) p.push('WARN', sec(t.warn));
        if (t.stays != null && +t.stays !== 6) p.push('STAYS', sec(t.stays));
        break;
    default: break;
    }
    if (t.stopped && ['plat', 'flow', 'current', 'haz'].includes(t.k)) p.push('OFF');   /* (stopped: flow's own off is its dry seconds) */
    if (t.loose && ['gold', 'key', 'life'].includes(t.k)) p.push('LOOSE');
    if (t.tag) p.push('TAG', t.tag);
    let out = p.join(' ');
    if (t.k === 'haz') (t.cells || []).forEach(([x, y]) => { out += `\nHZC ${x} ${y}`; });
    return out;
}
function trigLine(g) {
    const p = ['TRIG', g.src, g.mode];
    if (g.mode === 'TIMED') p.push(sec(g.secs || 1));
    (g.acts || []).forEach(a => p.push(a.crypt != null ? 'CRYPT:' + (a.crypt | 0) : `${a.tag}:${a.verb}`));
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
    (m.trigs || []).forEach(g => out.push(trigLine(g)));
    Object.keys(m.tune || {}).forEach(k => out.push(`TUNE ${k} ${+(+m.tune[k]).toFixed(4)}`));
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
function parse(text, kindIds, tuneNames) {
    const m = { kind: 'side', v: 1, name: 'A SIDE PASSAGE', num: 1, level: 3, w: 0, h: 0, grid: [], things: [], next: 1, trigs: [], tune: {} };
    let lastHaz = null;
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
        if (op === 'TUNE') {
            if (w.length < 3 || (tuneNames && !tuneNames.includes(w[1]))) return bad('TUNE wants a name the game tunes and a number: ' + (w[1] || ''));
            m.tune[w[1]] = parseFloat(w[2]); continue;
        }
        if (op === 'HZC') {
            if (!lastHaz) return bad('HZC before any HAZ');
            if (!(iv[1] >= 0 && iv[2] >= 0 && iv[1] < m.w && iv[2] < m.h)) return bad('HZC wants a cell in the map');
            lastHaz.cells.push([iv[1], iv[2]]); continue;
        }
        if (op === 'TRIG') {
            if (w.length < 4) return bad('TRIG wants a source, a mode and something to do');
            const g = { src: w[1], mode: w[2], acts: [] };
            if (!MODES.includes(g.mode)) return bad("a trigger's mode is ONCE, TOGGLE, HOLD or TIMED: " + g.mode);
            let i = 3;
            if (g.mode === 'TIMED') { g.secs = fv[3]; i = 4; if (!(g.secs > 0)) return bad('TIMED wants its seconds'); }
            for (; i < w.length; i++) {
                const c = w[i].indexOf(':');
                if (c < 0) return bad("a trigger's action is tag:ON, tag:OFF, tag:FLIP or CRYPT:n: " + w[i]);
                const a = w[i].slice(0, c), v = w[i].slice(c + 1);
                if (a === 'CRYPT') g.acts.push({ crypt: parseInt(v, 10) });
                else { if (!['ON', 'OFF', 'FLIP'].includes(v)) return bad("a trigger's verb is ON, OFF or FLIP: " + v); g.acts.push({ tag: a, verb: v }); }
            }
            if (!g.acts.length) return bad('a trigger with nothing to do');
            m.trigs.push(g); continue;
        }
        const k = Object.keys(OPS).find(key => OPS[key] === op);
        if (!k) return bad("a word the game doesn't know: " + op);
        const need = NEED[k];
        if (w.length < need) return bad(`${op} wants ${need - w.length} more`);
        if (m.things.length >= LIM.things) return bad(`more than ${LIM.things} things`);
        const t = k === 'haz' ? { k, kind: w[1], x: iv[2], y: iv[3] } : { k, x: iv[1], y: iv[2] };
        if (!(t.x >= 0 && t.y >= 0 && t.x < m.w && t.y < m.h)) return bad(op + ' outside the map');
        for (let i = 3; i < w.length; i++) {                   /* the words any thing may carry */
            if (w[i] === 'TAG' && w[i + 1]) { t.tag = w[++i]; continue; }
            if (w[i] === 'OFF') t.stopped = true; else if (w[i] === 'LOOSE') t.loose = true; else if (w[i] === 'POISON') t.poison = true; else if (w[i] === 'OPEN') t.open = true;
        }
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
        case 'gate': case 'zone':
            t.w = iv[3]; t.h = iv[4]; if (k === 'gate') t.open = !!t.open;
            if (!(t.w >= 1 && t.h >= 1 && t.x + t.w <= m.w && t.y + t.h <= m.h)) return bad(`${op}'s box is outside the map`);
            break;
        case 'current':
            Object.assign(t, { w: iv[3], h: iv[4], dx: iv[5], dy: iv[6], speed: fv[7] });
            if (!(t.w >= 1 && t.h >= 1 && t.x + t.w <= m.w && t.y + t.h <= m.h)) return bad("a current's box is outside the map");
            if (![-1, 0, 1].includes(t.dx) || ![-1, 0, 1].includes(t.dy) || (!t.dx && !t.dy) || !(t.speed > 0)) return bad('a current runs -1, 0 or 1 each way, at some speed');
            break;
        case 'haz':
            if (!HAZ_FAMILY[t.kind]) return bad("not one of the crypt's traps: " + t.kind);
            Object.assign(t, { cells: [], zone: false, safe: false, clock: null, enter: null, harm: -1, chance: -1, warn: -1, stays: 6 });
            for (let i = 4; i < w.length; i++) {
                if (w[i] === 'ZONE') t.zone = true; else if (w[i] === 'SAFE') t.safe = true;
                else if (w[i] === 'CLOCK') { t.clock = { every: fv[i + 1], give: fv[i + 2], first: fv[i + 3] }; i += 3; }
                else if (w[i] === 'ENTER') { t.enter = { x0: iv[i + 1], y0: iv[i + 2], x1: iv[i + 3], y1: iv[i + 4] }; i += 4; }
                else if (w[i] === 'HARM') t.harm = iv[++i]; else if (w[i] === 'CHANCE') t.chance = iv[++i];
                else if (w[i] === 'WARN') t.warn = fv[++i]; else if (w[i] === 'STAYS') t.stays = fv[++i]; else if (w[i] === 'TAG') i++;
            }
            if (t.clock && !(t.clock.every >= 0.5)) return bad("a trap's clock wants half a second at the least");
            lastHaz = t;
            break;
        default: break;
        }
        add(m, t);
    }
    if (!version) return bad('no SIDE 1');
    if (rows !== m.h) return bad(`${rows} ROWs, and SIZE says ${m.h}`);
    if (!m.things.some(t => t.k === 'door')) return bad('no doorway: there is no way in');
    for (const g of m.trigs) {                                  /* the names, now all is read */
        const s = m.things.find(t => t.tag === g.src);
        if (!s || !SOURCES.includes(s.k)) return { error: `a trigger's source is no plate, zone or switch named ${g.src}`, line: ln };
        for (const a of g.acts) if (a.crypt == null && !m.things.some(t => t.tag === a.tag)) return { error: `a trigger names nothing called ${a.tag}`, line: ln };
    }
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
    const tags = {};
    m.things.forEach(t => {
        if (!t.tag) return;
        if (!TAG_RE.test(t.tag)) err(`The name "${t.tag}": letters, digits, - and _ only, up to 15.`, t);
        if (tags[t.tag]) err(`Two things are named ${t.tag}: a trigger could not tell them apart.`, t);
        tags[t.tag] = t;
        if (['gate', 'zone', 'current'].includes(t.k) && !(t.w >= 1 && t.h >= 1 && t.x + t.w <= m.w && t.y + t.h <= m.h)) err('Its box runs off the map.', t);
    });
    m.things.forEach(t => {
        if (t.k === 'haz') {
            if (!(t.cells || []).length) err('A trap with no cells: paint them (PAINT ITS CELLS).', t);
            if (!t.clock && !t.enter && !(m.trigs || []).some(g => g.acts.some(a => a.tag && a.tag === t.tag)) && t.stopped) warn('A trap that is off, with no clock, no box and no trigger: it never goes.', t);
            if (t.clock && !(t.clock.every >= 0.5)) err("A trap's clock wants half a second at the least.", t);
        }
        if (t.k === 'current' && (!t.dx && !t.dy)) err('A current with no way to run.', t);
        if (t.k === 'plate' && !SOLID.has(at(m, t.x, t.y + 1)) && at(m, t.x, t.y + 1) !== '=') warn('A plate with nothing under it: it lies in its tile\'s foot.', t);
        if (['plate', 'zone'].includes(t.k) && !(m.trigs || []).some(g => g.src === t.tag)) warn(`A ${t.k} no trigger listens to.`, t);
    });
    (m.trigs || []).forEach((g, i) => {
        const s = tags[g.src];
        if (!s || !SOURCES.includes(s.k)) out.push({ sev: 'error', msg: `Trigger ${i + 1}: its source is no plate, zone or switch (${g.src || 'none'}).`, at: null, trig: i });
        if (!(g.acts || []).length) out.push({ sev: 'error', msg: `Trigger ${i + 1} does nothing.`, at: null, trig: i });
        (g.acts || []).forEach(a => { if (a.crypt == null && (!tags[a.tag] || !TARGETS.includes(tags[a.tag].k))) out.push({ sev: 'error', msg: `Trigger ${i + 1}: ${a.tag || 'nothing'} is no gate, current, flow, platform or trap.`, at: null, trig: i }); });
        if (g.mode === 'TIMED' && !(g.secs > 0)) out.push({ sev: 'error', msg: `Trigger ${i + 1}: TIMED wants its seconds.`, at: null, trig: i });
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
    case 'gate': return `A gate, ${t.w} by ${t.h}${t.tag ? ': ' + t.tag : ''}`;
    case 'plate': return 'A pressure plate' + (t.tag ? ': ' + t.tag : '');
    case 'zone': return 'An unseen zone' + (t.tag ? ': ' + t.tag : '');
    case 'current': return 'Running water' + (t.poison ? ', poisoned' : '') + (t.tag ? ': ' + t.tag : '');
    case 'haz': return (HAZ_NAMES[t.kind] || t.kind) + (t.tag ? ': ' + t.tag : '');
    default: return t.k;
    }
}

const API = { LIM, TILES, TILE_NAMES, SOLID, KINDS, GOLD, RANKS, KEYS, FALL_DELAY, FALL_BACK, HAZ_FAMILY, HAZ_NAMES, FAMILY_SAYS, MODES, SOURCES, TARGETS, TAG_RE,
              blank, add, newTag, at, resize, exportText, exportC, parse, check, label };
if (typeof module !== 'undefined' && module.exports) module.exports = API; else root.SideCore = API;
})(typeof window !== 'undefined' ? window : globalThis);
