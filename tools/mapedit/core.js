/* The map editor's logic (docs/PLAN-mapedit.md): the map, the stamps laid on it, the drawn level's letters both ways, the checks,
 * the budgets and the dream's walk. No drawing here, so node can test it (tools/mapedit/test.js). DATA is what extract.py read
 * from the game's source. */
(function (root) {
'use strict';
const N = 45;                                   /* MAPN */
const DIRS = [[1, 0], [0, 1], [-1, 0], [0, -1]];   /* the game's DX, DY: east, south, west, north */
const FRONT_ORDER = [3, 1, 2, 0];               /* drawn_front: north, south, west, east first */
const DOORS = new Set(['D', 'G', 'C', '=']);
const WALLS = new Set(['#', 'X', 'h', 'c']);    /* stone, and the three things set in it */
const LANE_OF = { '.': '+', 'F': 'f', 'I': 'i', 'q': 'Q' };
const BASE_OF = { '+': '.', 'f': 'F', 'i': 'I', 'Q': 'q' };
const RUNS = ['THE CRYPTS', 'THE CATACOMBS', 'THE HOLLOWS', 'THE DEEP'];
const RANKS = ['I', 'II', 'III', 'IV', 'V', 'GILDED'];
const ELEM_CHANCE = [0, 0.15, 0.25, 0.35, 0.45, 0];   /* elem_roll's CHANCE by rank (damage.c) */
const ELEM_FAV = [['EL_FIRE', 'EL_SPIRIT'], ['EL_FROST', 'EL_POISON'], ['EL_LIGHT', 'EL_POISON'], ['EL_FIRE', 'EL_SPIRIT']];
const GOLD_KINDS = ['IT_COINS', 'IT_CHALICE', 'IT_GEMS', 'IT_CROWN', 'IT_CHEST'];
const KEY_NAMES = ['GOLD', 'SILVER', 'CRIMSON'];
const MARK_NAMES = ['COFFER', 'CAGE', "THE CLASS'S OWN"];
const MAX = { heads: 48, mobs: 64, items: 96, fire: 16, ice: 24, rooms: 12, rest: 2 };

const idx = (x, y) => y * N + x;
const inMap = (x, y) => x >= 0 && y >= 0 && x < N && y < N;
const isOpen = ch => !WALLS.has(ch);            /* is_open: a cell one may stand in (a door too) */

function blank(level) {
    return { v: 1, name: 'Untitled level', level: level || 3, face: 0, seed: '', lattice: true, corrupt: false,
             grid: new Array(N * N).fill('#'), lane: new Array(N * N).fill(0), rooms: [], things: [], stamps: [], next: 1 };
}
function clone(m) { return JSON.parse(JSON.stringify(m)); }

/* ---- stamps: turned and mirrored as stamp_xform does, then laid on the grid ---- */
function stampDef(DATA, name) { return DATA.stamps.find(s => s.name === name); }
function xformRows(rows, t) {                   /* t 0..7: bit 2 a mirror in x first, then t & 3 quarter turns clockwise */
    let r = rows.slice();
    if (t & 4) r = r.map(row => row.split('').reverse().join(''));
    for (let k = 0; k < (t & 3); k++) {
        const h = r.length, w = r[0].length, out = [];
        for (let y = 0; y < w; y++) { let s = ''; for (let x = 0; x < h; x++) s += r[h - 1 - x][y]; out.push(s); }
        r = out;
    }
    return r;
}
function stampRows(DATA, st) { const d = stampDef(DATA, st.name); return d ? xformRows(d.rows, st.t || 0) : ['#']; }
/* What a stamp's glyph becomes in the drawn level's letters. A secret's floor is the secret's room; its first entrance (or the one
 * chosen) its cracked wall, the rest stone; a doorway on to the next link is stone (a chain of one, as the plan assumed). */
function stampLetter(kind, g, entryIndex, chosen) {
    if (g === '#') return '#';
    if (g === 'E') return entryIndex === chosen ? 'c' : '#';
    if (g === 'D') return '#';
    return kind === 'secret' ? 'q' : '.';
}
/* The map as the game will hold it: the grid with every stamp laid over it in order. Returns the letters, the stamps' marks and
 * which stamp lies on each cell. */
function compose(DATA, m) {
    const cells = m.grid.slice(), owner = new Array(N * N).fill(-1), marks = [];
    m.stamps.forEach((st, si) => {
        const d = stampDef(DATA, st.name); if (!d) return;
        const rows = xformRows(d.rows, st.t || 0);
        let e = 0;
        for (let y = 0; y < rows.length; y++)
            for (let x = 0; x < rows[y].length; x++) {
                const mx = st.x + x, my = st.y + y, g = rows[y][x];
                if (!inMap(mx, my)) continue;
                const ei = g === 'E' ? e++ : -1;
                cells[idx(mx, my)] = stampLetter(d.kind, g, ei, st.entry || 0);
                owner[idx(mx, my)] = si;
                if ('abcst0123456789'.includes(g)) marks.push({ x: mx, y: my, g, stamp: si });
                if (g === 'P' || g === 'p') marks.push({ x: mx, y: my, g, stamp: si });
            }
    });
    return { cells, owner, marks };
}
/* Whether a cell is on the dream's lane: doors, the start and a secret's wall always; a cell painted on or off; else, on the lattice,
 * the hall's middle line (x or y = 2 mod 4), as the generator lays it. */
function laneAt(m, cells, x, y) {
    const i = idx(x, y), ch = cells[i];
    if (ch === '#' || ch === 'X' || ch === 'h') return false;
    if (DOORS.has(ch) || ch === 'S' || ch === 'c') return true;
    if (m.lane[i] === 1) return true;
    if (m.lane[i] === 2) return false;
    return !!m.lattice && (x % 4 === 2 || y % 4 === 2);
}
function lanes(m, cells) { const L = new Uint8Array(N * N); for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) L[idx(x, y)] = laneAt(m, cells, x, y) ? 1 : 0; return L; }

/* ---- the drawn level's letters, both ways ---- */
function importDrawn(D, level) {
    const m = blank(level || D.n);
    m.name = 'Tutorial level ' + D.n + ' (from drawn.c)'; m.face = D.face; m.seed = D.seed; m.lattice = false;
    for (let y = 0; y < D.h; y++)
        for (let x = 0; x < D.w; x++) {
            const ch = D.rows[y][x], i = idx(D.ox + x, D.oy + y);
            m.grid[i] = BASE_OF[ch] || ch;
            if (BASE_OF[ch]) m.lane[i] = 1;
        }
    D.rooms.forEach(r => m.rooms.push({ x: D.ox + r.x, y: D.oy + r.y, w: r.w, h: r.h, rt: r.rt, vault: -1 }));
    D.vault.forEach((r, k) => { if (r >= 0 && m.rooms[r]) m.rooms[r].vault = k; });
    D.things.forEach(t => {
        const o = { id: m.next++, x: +(D.ox + t.x).toFixed(4), y: +(D.oy + t.y).toFixed(4) };
        switch (t.what) {
        case 'GOLD': case 'SGOLD': o.t = 'gold'; o.k = t.kind; break;
        case 'KEY': o.t = 'key'; o.k = +t.kind; break;
        case 'DEAD': o.t = 'dead'; o.k = t.kind; break;
        case 'MARK': o.t = 'mark'; o.k = +t.kind; break;
        case 'SWITCH': o.t = 'switch'; break;
        case 'GSEAL': o.t = 'gseal'; break;
        case 'MAP': o.t = 'map'; break;
        default: return;
        }
        m.things.push(o);
    });
    return m;
}
function bounds(cells) {
    let x0 = N, y0 = N, x1 = -1, y1 = -1;
    for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) if (cells[idx(x, y)] !== '#') { x0 = Math.min(x0, x); y0 = Math.min(y0, y); x1 = Math.max(x1, x); y1 = Math.max(y1, y); }
    return x1 < 0 ? null : { x0, y0, x1, y1, w: x1 - x0 + 1, h: y1 - y0 + 1 };
}
const f2 = v => v.toFixed(2) + 'f';
const thingCell = t => [Math.floor(t.x), Math.floor(t.y)];
/* The level as drawn.c holds one: DRAWNn_MAP, _THING, _ROOM and its row in DRAWN[], n the slot it takes. Spawn points go in their own
 * array, which the game reads only once part 2 is in (docs/PLAN-mapedit.md). */
function exportDrawn(DATA, m, n) {
    const { cells } = compose(DATA, m), L = lanes(m, cells), b = bounds(cells);
    if (!b) return { text: '/* the map is all stone: nothing to export */', ok: false };
    const rows = [];
    for (let y = b.y0; y <= b.y1; y++) {
        let s = '';
        for (let x = b.x0; x <= b.x1; x++) { const ch = cells[idx(x, y)]; s += L[idx(x, y)] && LANE_OF[ch] ? LANE_OF[ch] : ch; }
        rows.push(s);
    }
    const P = 'DRAWN' + n, out = [];
    const things = [], spawns = [];
    m.things.forEach(t => {
        const x = t.x - b.x0, y = t.y - b.y0, [cx, cy] = thingCell(t), sec = inMap(cx, cy) && cells[idx(cx, cy)] === 'q';
        switch (t.t) {
        case 'gold': things.push(`{ ${sec ? 'DT_SGOLD' : 'DT_GOLD'}, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'key': things.push(`{ DT_KEY, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'dead': things.push(`{ DT_DEAD, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'mark': things.push(`{ DT_MARK, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'switch': things.push(`{ DT_SWITCH, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'gseal': things.push(`{ DT_GSEAL, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'map': things.push(`{ DT_MAP, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'spawn': spawns.push(`{ ${f2(x)}, ${f2(y)}, ${t.k || -1}, ${t.chance == null ? 100 : t.chance}, ${t.echance == null ? -1 : t.echance}, ${t.el || -1}, ${t.rank === '' || t.rank == null ? -1 : 'RK_' + t.rank} }`); break;
        }
    });
    const marks = compose(DATA, m).marks.filter(k => k.g === 's');
    marks.forEach(k => spawns.push(`{ ${f2(k.x - b.x0 + 0.5)}, ${f2(k.y - b.y0 + 0.5)}, -1, 65, -1, -1, -1 }`));
    const vault = [-1, -1, -1];
    m.rooms.forEach((r, i) => { if (r.vault >= 0 && r.vault < 3) vault[r.vault] = i; });
    const seed = m.seed && /^0x[0-9a-f]+$/i.test(m.seed) ? m.seed : '0x7D1A' + (0x10000 + n).toString(16).slice(1).toUpperCase();
    out.push(`/* ${m.name.replace(/\*\//g, '')}: level ${m.level}, drawn in the map editor (docs/PLAN-mapedit.md) */`);
    out.push(`static const char *const ${P}_MAP[${rows.length}] = {`);
    rows.forEach((r, y) => out.push(`    "${r}",    /* ${String(y).padStart(2)} */`));
    out.push('};');
    out.push(`static const DrawnThing ${P}_THING[] = {`);
    for (let i = 0; i < things.length; i += 3) out.push('    ' + things.slice(i, i + 3).join(', ') + ',');
    if (!things.length) out.push('    { DT_GOLD, IT_COINS, 0.0f, 0.0f },    /* the array may not be empty: move or replace this */');
    out.push('};');
    out.push(`static const DrawnRoom ${P}_ROOM[] = {`);
    m.rooms.forEach(r => out.push(`    { ${r.x - b.x0}, ${r.y - b.y0}, ${r.w}, ${r.h}, ${r.rt} },`));
    if (!m.rooms.length) out.push('    { 0, 0, 1, 1, RT_HALL },    /* the array may not be empty: draw a room */');
    out.push('};');
    if (spawns.length) {
        out.push(`/* spawn points (x, y, kind or -1 the budget's choice, chance %, element chance % or -1 by rank, element or -1, rank or -1 the`);
        out.push(` * level's): read by the game once part 2 is in (docs/PLAN-mapedit.md) */`);
        out.push(`static const DrawnSpawn ${P}_SPAWN[] = {`);
        for (let i = 0; i < spawns.length; i += 2) out.push('    ' + spawns.slice(i, i + 2).join(', ') + ',');
        out.push('};');
    }
    out.push(`/* its row in DRAWN[] */`);
    out.push(`    { ${P}_MAP, ${b.w}, ${b.h}, ${b.x0}, ${b.y0}, ${m.face}, ${P}_ROOM, (int)(sizeof ${P}_ROOM / sizeof ${P}_ROOM[0]), { ${vault.join(', ')} },`);
    out.push(`      ${P}_THING, (int)(sizeof ${P}_THING / sizeof ${P}_THING[0]), ${seed}u },`);
    return { text: out.join('\n') + '\n', ok: true, rows, b, things, spawns, vault, seed };
}

/* ---- the dream's walk: lanes only, a key opens its doors, the seal opens the gate; the nearest thing it wants next, then the exit.
 * An approximation of the dream (goals.c), not the game's own code: it shows what can be reached and in what order. ---- */
function frontOf(cells, x, y) {                 /* drawn_front: a wall's open side, north, south, west, east first */
    for (const d of FRONT_ORDER) {
        const fx = x + DIRS[d][0], fy = y + DIRS[d][1];
        if (inMap(fx, fy) && isOpen(cells[idx(fx, fy)]) && cells[idx(fx, fy)] !== 'q') return [fx, fy];
    }
    return null;
}
function dream(DATA, m, opts) {
    opts = opts || {};
    const { cells } = compose(DATA, m), L = lanes(m, cells);
    const st = { keys: [0, 0, 0], gate: false, secret: !!opts.secrets };
    const pass = (x, y) => {
        if (!inMap(x, y)) return false;
        const i = idx(x, y), ch = cells[i];
        if (!L[i]) return false;
        if (ch === 'G') return !!st.keys[0];
        if (ch === 'C') return !!st.keys[2];
        if (ch === '=') return st.gate;
        if (ch === 'c' || ch === 'q') return st.secret;
        return true;
    };
    let start = null;
    for (let i = 0; i < N * N; i++) if (cells[i] === 'S') { start = [i % N, (i / N) | 0]; break; }
    const res = { route: [], events: [], ok: false, reached: new Set(), missed: [], exit: null, steps: 0 };
    if (!start) { res.events.push({ at: 0, msg: 'There is no start (S): the dream has nowhere to begin.' }); return res; }
    let exit = null;
    for (let i = 0; i < N * N; i++) if (cells[i] === 'X') { exit = frontOf(cells, i % N, (i / N) | 0); }
    res.exit = exit;
    const goals = m.things.filter(t => ['gold', 'key', 'map', 'gseal', 'switch'].includes(t.t))
        .map(t => ({ t, c: thingCell(t), secret: (() => { const [x, y] = thingCell(t); return inMap(x, y) && cells[idx(x, y)] === 'q'; })() }));
    const near = (x, y, g) => Math.abs(x - g.c[0]) + Math.abs(y - g.c[1]) <= 1;
    const bfs = (sx, sy) => {
        const dist = new Int32Array(N * N).fill(-1), prev = new Int32Array(N * N).fill(-1), q = [idx(sx, sy)];
        dist[idx(sx, sy)] = 0;
        for (let h = 0; h < q.length; h++) {
            const i = q[h], x = i % N, y = (i / N) | 0;
            for (const [dx, dy] of DIRS) {
                const nx = x + dx, ny = y + dy;
                if (!pass(nx, ny) || dist[idx(nx, ny)] >= 0) continue;
                dist[idx(nx, ny)] = dist[i] + 1; prev[idx(nx, ny)] = i; q.push(idx(nx, ny));
            }
        }
        return { dist, prev };
    };
    const pathTo = (prev, i) => { const p = []; while (i >= 0) { p.push([i % N, (i / N) | 0]); i = prev[i]; } return p.reverse(); };
    const pick = (cur, g) => {                  /* what it gathers standing here */
        if (res.reached.has(g.t.id)) return;
        res.reached.add(g.t.id);
        const at = res.route.length - 1, where = `(${g.c[0]},${g.c[1]})`;
        if (g.t.t === 'key') { st.keys[g.t.k] = 1; res.events.push({ at, msg: `Takes the ${KEY_NAMES[g.t.k].toLowerCase()} key at ${where}.` }); }
        else if (g.t.t === 'gseal') { st.gate = true; res.events.push({ at, msg: `Strikes the gate's seal at ${where}: the gate opens.` }); }
        else if (g.t.t === 'map') res.events.push({ at, msg: `Takes the map at ${where}.` });
        else if (g.t.t === 'switch') res.events.push({ at, msg: `Strikes the switch at ${where}.` });
    };
    let cur = start; res.route.push(cur.slice());
    const gatherHere = () => goals.forEach(g => { if (near(cur[0], cur[1], g) && (!g.secret || st.secret)) pick(cur, g); });
    gatherHere();
    for (let guard = 0; guard < 400; guard++) {
        const { dist, prev } = bfs(cur[0], cur[1]);
        let best = null, bd = 1e9;
        for (const g of goals) {
            if (res.reached.has(g.t.id) || (g.secret && !st.secret)) continue;
            for (let k = -1; k < 4; k++) {      /* the cells it may stand on to take it: its own and the four beside it */
                const x = g.c[0] + (k < 0 ? 0 : DIRS[k][0]), y = g.c[1] + (k < 0 ? 0 : DIRS[k][1]);
                if (!inMap(x, y)) continue;
                const d = dist[idx(x, y)];
                if (d >= 0 && d < bd) { bd = d; best = idx(x, y); }
            }
        }
        if (best === null) break;
        const p = pathTo(prev, best);
        for (let k = 1; k < p.length; k++) {
            cur = p[k]; res.route.push(cur.slice());
            const ch = cells[idx(cur[0], cur[1])];
            if (ch === 'G' || ch === 'C') res.events.push({ at: res.route.length - 1, msg: `Opens the ${ch === 'G' ? 'gold' : 'crimson'} door at (${cur[0]},${cur[1]}).` });
            gatherHere();
        }
    }
    if (exit) {
        const { dist, prev } = bfs(cur[0], cur[1]);
        if (dist[idx(exit[0], exit[1])] >= 0) {
            pathTo(prev, idx(exit[0], exit[1])).slice(1).forEach(c => { cur = c; res.route.push(c.slice()); gatherHere(); });
            res.events.push({ at: res.route.length - 1, msg: `Reaches the exit at (${exit[0]},${exit[1]}).` });
            res.ok = true;
        } else res.events.push({ at: res.route.length - 1, msg: 'Cannot reach the exit from here: the level cannot be finished.' });
    } else res.events.push({ at: res.route.length - 1, msg: 'There is no exit (X) with an open side.' });
    res.missed = goals.filter(g => !res.reached.has(g.t.id)).map(g => g.t);
    res.steps = res.route.length - 1;
    return res;
}

/* ---- budgets: the dead's cost against the level's, gold in the open and in the secrets ---- */
function levelInfo(DATA, level) {
    const run = Math.floor((level - 1) / 10), set = run % 4, step = (level - 1) % 10;
    return { run, set, step, runName: RUNS[set], boss: step === 9, loop: Math.floor((level - 1) / 40) };
}
function rankCost(r) { return r === 5 ? 1.4 : 1 + 0.2 * Math.min(r, 4); }
function kindOf(DATA, id) { return DATA.kinds.find(k => k.id === id); }
function budget(DATA, m) {
    const L = levelInfo(DATA, m.level), { cells } = compose(DATA, m);
    let B = DATA.budget[L.set][Math.min(L.step, 9)] * (m.corrupt ? 1.4 : 1) * (L.boss ? 0.5 : 1);
    let fixed = 0, expect = 0, most = 0, heads = 0, headsMost = 0;
    const lines = [];
    m.things.forEach(t => {
        if (t.t === 'dead') { const k = kindOf(DATA, t.k), c = k && k.cost != null ? k.cost : 1; fixed += c; heads++; headsMost++; }
        if (t.t === 'spawn') {
            const k = t.k ? kindOf(DATA, t.k) : null, ri = t.rank ? RANKS.indexOf(t.rank) : 0;
            const c = (k && k.cost != null ? k.cost : 1) * rankCost(Math.max(0, ri)), ch = (t.chance == null ? 100 : t.chance) / 100;
            expect += c * ch; most += c; headsMost++; heads += ch;
        }
    });
    compose(DATA, m).marks.filter(k => k.g === 's').forEach(() => { expect += 0.65; most += 1; heads += 0.65; headsMost++; });
    const gold = { open: 0, secret: 0, n: 0, ns: 0, byKind: {} };
    m.things.forEach(t => {
        if (t.t !== 'gold') return;
        const v = DATA.items[t.k] || 0, [x, y] = thingCell(t), sec = inMap(x, y) && cells[idx(x, y)] === 'q';
        if (sec) { gold.secret += v; gold.ns++; } else { gold.open += v; gold.n++; }
        gold.byKind[t.k] = (gold.byKind[t.k] || 0) + 1;
    });
    /* each secret's own total: its room's cells, flooded from behind its cracked wall */
    const secrets = [];
    for (let i = 0; i < N * N; i++) {
        if (cells[i] !== 'c') continue;
        const seen = new Set(), q = [];
        for (const [dx, dy] of DIRS) { const x = i % N + dx, y = ((i / N) | 0) + dy; if (inMap(x, y) && cells[idx(x, y)] === 'q') { q.push(idx(x, y)); seen.add(idx(x, y)); } }
        for (let h = 0; h < q.length; h++) for (const [dx, dy] of DIRS) {
            const x = q[h] % N + dx, y = ((q[h] / N) | 0) + dy;
            if (inMap(x, y) && cells[idx(x, y)] === 'q' && !seen.has(idx(x, y))) { seen.add(idx(x, y)); q.push(idx(x, y)); }
        }
        let v = 0, n = 0;
        m.things.forEach(t => { if (t.t === 'gold') { const [x, y] = thingCell(t); if (seen.has(idx(x, y))) { v += DATA.items[t.k] || 0; n++; } } });
        secrets.push({ x: i % N, y: (i / N) | 0, cells: seen.size, gold: v, n });
    }
    return { L, budget: B, fixed, expect: fixed + expect, most: fixed + most, heads, headsMost, gold, secrets, lines };
}
function elemDefault(level, rank) {             /* the chance a creature of that rank is born to an element, as the game rolls it */
    return ELEM_CHANCE[Math.max(0, Math.min(5, rank))];
}

/* ---- the checks: what would break the level in the game, and what only looks wrong ---- */
function check(DATA, m) {
    const out = [], add = (sev, msg, cells) => out.push({ sev, msg, cells: cells || [] });
    const comp = compose(DATA, m), cells = comp.cells, L = lanes(m, cells), lv = levelInfo(DATA, m.level);
    const at = (x, y) => inMap(x, y) ? cells[idx(x, y)] : '#';
    const find = ch => { const r = []; for (let i = 0; i < N * N; i++) if (cells[i] === ch) r.push([i % N, (i / N) | 0]); return r; };

    const edge = [];
    for (let k = 0; k < N; k++) for (const [x, y] of [[k, 0], [k, N - 1], [0, k], [N - 1, k]]) if (at(x, y) !== '#') edge.push([x, y]);
    if (edge.length) add('error', 'The map\'s outer ring must be stone: the game keeps a wall round every level.', edge);

    const S = find('S');
    if (S.length !== 1) add('error', S.length ? `There are ${S.length} starts (S): a level has one.` : 'There is no start (S).', S);
    const X = find('X');
    if (!X.length) add('error', 'There is no exit (X): set one in a wall, beside the floor it is used from.');
    if (X.length > 1) add('warn', `There are ${X.length} exits: the game uses only the last one it finds, reading left to right, top to bottom.`, X);
    X.forEach(c => { if (!frontOf(cells, c[0], c[1])) add('error', 'The exit has no open side to be used from.', [c]); });
    const H = find('h');
    if (H.length > MAX.rest) add('warn', `There are ${H.length} places to mend: the game keeps ${MAX.rest}.`, H.slice(MAX.rest));
    H.forEach(c => { if (!frontOf(cells, c[0], c[1])) add('error', 'A place to mend has no open side to be used from.', [c]); });

    /* doors: open on one axis, stone on the other, as the generator builds them */
    for (let i = 0; i < N * N; i++) {
        if (!DOORS.has(cells[i])) continue;
        const x = i % N, y = (i / N) | 0;
        const ew = isOpen(at(x - 1, y)) && isOpen(at(x + 1, y)), ns = isOpen(at(x, y - 1)) && isOpen(at(x, y + 1));
        const ewWall = !isOpen(at(x - 1, y)) && !isOpen(at(x + 1, y)), nsWall = !isOpen(at(x, y - 1)) && !isOpen(at(x, y + 1));
        if (!((ew && nsWall) || (ns && ewWall))) add('error', 'A door must stand in a wall: floor on two opposite sides, stone on the other two.', [[x, y]]);
    }
    const keyOf = c => m.things.filter(t => t.t === 'key' && t.k === c);
    if (find('G').length && !keyOf(0).length) add('error', 'There is a gold door but no gold key.', find('G'));
    if (find('C').length && !keyOf(2).length) add('error', 'There is a crimson door but no crimson key.', find('C'));
    if (keyOf(1).length) add('warn', 'A silver key has no door of its own in the drawn levels\' letters yet.', keyOf(1).map(thingCell));
    if (keyOf(0).length && !find('G').length) add('warn', 'There is a gold key but no gold door.', keyOf(0).map(thingCell));
    if (keyOf(2).length && !find('C').length) add('warn', 'There is a crimson key but no crimson door.', keyOf(2).map(thingCell));
    const gates = find('='), seals = m.things.filter(t => t.t === 'gseal');
    if (gates.length > 1) add('error', 'There is more than one gate: the game keeps one.', gates);
    if (gates.length && !seals.length) add('error', 'The gate has no seal to open it.', gates);
    if (seals.length && !gates.length) add('warn', 'There is a gate\'s seal but no gate.', seals.map(thingCell));
    const fire = cells.filter(c => c === 'F').length, ice = cells.filter(c => c === 'I').length;
    if (fire > MAX.fire) add('error', `${fire} cells of burning floor: the game keeps ${MAX.fire}.`, find('F').slice(MAX.fire));
    if (ice > MAX.ice) add('error', `${ice} cells of ice: the game keeps ${MAX.ice}.`, find('I').slice(MAX.ice));
    if (ice && !m.things.some(t => t.t === 'switch')) add('warn', 'There is ice but no switch: nothing drains it.', find('I'));

    /* rooms */
    if (m.rooms.length > MAX.rooms) add('error', `${m.rooms.length} rooms: the game keeps ${MAX.rooms}.`);
    if (!m.rooms.length) add('warn', 'No rooms are marked: every cell will look like a hall.');
    m.rooms.forEach((r, i) => {
        let open = 0;
        for (let y = r.y; y < r.y + r.h; y++) for (let x = r.x; x < r.x + r.w; x++) if (isOpen(at(x, y))) open++;
        if (!open) add('warn', `Room ${i + 1} holds no floor.`, [[r.x, r.y]]);
        if (m.lattice && (![3, 7, 11, 15].includes(r.w) || ![3, 7, 11, 15].includes(r.h) || r.x % 4 !== 1 || r.y % 4 !== 1))
            add('info', `Room ${i + 1} is off the lattice (3, 7, 11 or 15 cells a side, from a block's first cell). A drawn level allows it.`, [[r.x, r.y]]);
    });
    const vaults = [0, 1, 2].map(k => m.rooms.filter(r => r.vault === k).length);
    vaults.forEach((n, k) => { if (n > 1) add('error', `${n} rooms are the ${KEY_NAMES[k].toLowerCase()} vault: one at most.`); });
    if (find('G').length && !vaults[0]) add('info', 'The gold door keeps no room marked as its vault.');

    /* secrets, as gen_drawn lays them */
    const C = find('c');
    if (C.length > 1) add('error', 'More than one secret: a drawn level holds one today (gen_drawn takes every secret room as one).', C);
    C.forEach(([x, y]) => {
        let into = -1;
        for (let d = 0; d < 4; d++) { const bx = x - DIRS[d][0], by = y - DIRS[d][1]; if (isOpen(at(bx, by)) && at(bx, by) !== 'q') { into = d; break; } }
        if (into < 0) add('error', 'The secret\'s wall touches no hall to be found from.', [[x, y]]);
        else if (at(x + DIRS[into][0], y + DIRS[into][1]) !== 'q') add('error', 'The secret\'s wall slides straight on into its room: the cell beyond it must be the secret room.', [[x, y]]);
    });
    const Q = find('q'), leak = Q.filter(([x, y]) => DIRS.some(([dx, dy]) => { const ch = at(x + dx, y + dy); return isOpen(ch) && ch !== 'q'; }));
    if (leak.length) add('error', 'The secret room opens on to a hall: it would be found without its wall.', leak);
    if (Q.length && !C.length) add('error', 'A secret room with no cracked wall (c) can never be entered.', Q);

    /* the dream's lanes */
    const far = [];
    for (let i = 0; i < N * N; i++) {
        const x = i % N, y = (i / N) | 0, ch = cells[i];
        if (!isOpen(ch) || ch === 'q' || L[i]) continue;
        if (!DIRS.some(([dx, dy]) => inMap(x + dx, y + dy) && L[idx(x + dx, y + dy)])) far.push([x, y]);
    }
    if (far.length) add('warn', `${far.length} floor cell${far.length > 1 ? 's are' : ' is'} more than a step from a lane: the dream cannot fetch what lies there.`, far);
    if (S.length === 1) {
        const seen = new Uint8Array(N * N), q = [idx(S[0][0], S[0][1])];
        seen[q[0]] = 1;
        for (let h = 0; h < q.length; h++) for (const [dx, dy] of DIRS) {
            const x = q[h] % N + dx, y = ((q[h] / N) | 0) + dy;
            if (inMap(x, y) && L[idx(x, y)] && !seen[idx(x, y)]) { seen[idx(x, y)] = 1; q.push(idx(x, y)); }
        }
        const cut = [];
        for (let i = 0; i < N * N; i++) if (L[i] && !seen[i] && cells[i] !== 'q' && cells[i] !== 'c') cut.push([i % N, (i / N) | 0]);
        if (cut.length) add('warn', `${cut.length} lane cell${cut.length > 1 ? 's are' : ' is'} cut off from the start's lanes, even with every door open.`, cut);
    }

    /* things */
    const items = m.things.filter(t => ['gold', 'key', 'map'].includes(t.t)).length;
    if (items > MAX.items) add('error', `${items} things to pick up: the game keeps ${MAX.items}.`);
    m.things.forEach(t => {
        const [x, y] = thingCell(t);
        if (!isOpen(at(x, y))) add('error', `A ${label(DATA, t).toLowerCase()} stands in stone.`, [[x, y]]);
        else if (DOORS.has(at(x, y))) add('warn', `A ${label(DATA, t).toLowerCase()} stands in a doorway.`, [[x, y]]);
    });
    const restF = H.map(c => frontOf(cells, c[0], c[1])).filter(Boolean);
    m.things.filter(t => t.t === 'spawn' || t.t === 'dead').forEach(t => {
        const [x, y] = thingCell(t);
        if (t.t === 'spawn' && restF.some(f => Math.abs(f[0] - x) + Math.abs(f[1] - y) < 6)) add('warn', 'A spawn point within six steps of a place to mend: the budget sets nothing there.', [[x, y]]);
        if (t.t === 'spawn' && S.length === 1 && Math.abs(S[0][0] - x) + Math.abs(S[0][1] - y) < 8) add('warn', 'A spawn point within eight steps of the start: the game leaves these empty.', [[x, y]]);
        if (t.k) {
            const k = kindOf(DATA, t.k);
            if (k && k.debut > m.level) add('info', `${k.name} is first met on level ${k.debut}; here it comes on level ${m.level}, before its time.`, [[x, y]]);
            if (k && (k.unique || k.id === 'MOB_LING' || k.id === 'MOB_SAC' || k.id === 'MOB_GHOST')) add('warn', `${k.name} is not set by hand: the game raises it itself.`, [[x, y]]);
        }
    });
    const b = budget(DATA, m);
    if (b.headsMost > MAX.heads) add('error', `Up to ${b.headsMost} of the dead: the budget sets ${MAX.heads} at most.`);
    if (b.expect > b.budget * 1.25) add('warn', `The dead cost about ${b.expect.toFixed(1)} against a budget of ${b.budget.toFixed(1)} for level ${m.level}.`);

    /* stamps: ports that must join, stamps over each other or off the map */
    m.stamps.forEach((st, si) => {
        const rows = stampRows(DATA, st), h = rows.length, w = rows[0].length;
        if (st.x < 1 || st.y < 1 || st.x + w > N - 1 || st.y + h > N - 1) add('error', `The stamp ${st.name} runs off the map.`, [[Math.max(0, st.x), Math.max(0, st.y)]]);
        if (m.lattice && stampDef(DATA, st.name).kind !== 'secret' && (st.x % 4 !== 1 || st.y % 4 !== 1)) add('warn', `The stamp ${st.name} is off the lattice: its ports miss the halls' middle lanes.`, [[st.x, st.y]]);
        for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
            const g = rows[y][x];
            if (g !== 'P') continue;
            const out = DIRS.map(([dx, dy]) => [x + dx, y + dy]).filter(([ax, ay]) => ax < 0 || ay < 0 || ax >= w || ay >= h);
            if (!out.some(([ax, ay]) => isOpen(at(st.x + ax, st.y + ay)))) add('warn', `A port of ${st.name} that must join opens on to stone.`, [[st.x + x, st.y + y]]);
        }
        const clash = [];
        for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
            const i = idx(st.x + x, st.y + y);
            if (inMap(st.x + x, st.y + y) && comp.owner[i] !== si) clash.push([st.x + x, st.y + y]);
        }
        if (clash.length) add('warn', `The stamp ${st.name} lies under a later stamp in ${clash.length} cell${clash.length > 1 ? 's' : ''}.`, clash);
    });
    comp.marks.filter(k => 'abct'.includes(k.g)).length &&
        add('info', 'The stamps\' altar, brazier, loot and torch marks (a, b, c, t) are not laid on drawn levels yet; their spawn marks (s) are exported.');
    return out;
}
function label(DATA, t) {
    switch (t.t) {
    case 'gold': return ({ IT_COINS: 'Coins', IT_CHALICE: 'Chalice', IT_GEMS: 'Gems', IT_CROWN: 'Crown', IT_CHEST: 'Chest' })[t.k] || 'Gold';
    case 'key': return KEY_NAMES[t.k][0] + KEY_NAMES[t.k].slice(1).toLowerCase() + ' key';
    case 'dead': { const k = kindOf(DATA, t.k); return k ? k.name[0] + k.name.slice(1).toLowerCase() : 'One of the dead'; }
    case 'spawn': { const k = t.k && kindOf(DATA, t.k); return 'Spawn point' + (k ? ': ' + k.name.toLowerCase() : ''); }
    case 'mark': return ['Bound coffer', 'Cage', 'Thing that wants a spell'][t.k] || 'Mark';
    case 'switch': return 'Switch';
    case 'gseal': return 'Gate\'s seal';
    case 'map': return 'The map';
    }
    return t.t;
}

const API = { N, DIRS, DOORS, WALLS, LANE_OF, BASE_OF, RUNS, RANKS, ELEM_FAV, GOLD_KINDS, KEY_NAMES, MARK_NAMES, MAX, idx, inMap, isOpen,
              blank, clone, xformRows, stampRows, stampDef, compose, laneAt, lanes, importDrawn, exportDrawn, bounds, frontOf, dream,
              levelInfo, rankCost, budget, elemDefault, check, label, thingCell };
if (typeof module !== 'undefined' && module.exports) module.exports = API; else root.MapCore = API;
})(typeof window !== 'undefined' ? window : globalThis);
