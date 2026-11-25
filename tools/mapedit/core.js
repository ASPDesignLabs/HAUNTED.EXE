/* The map editor's logic (docs/PLAN-mapedit.md): the map, the stamps laid on it, the drawn level's letters both ways, the checks,
 * the budgets and the dream's walk. No drawing here, so node can test it (tools/mapedit/test.js). DATA is what extract.py read
 * from the game's source. */
(function (root) {
'use strict';
const N = 45;                                   /* MAPN */
const DIRS = [[1, 0], [0, 1], [-1, 0], [0, -1]];   /* the game's DX, DY: east, south, west, north */
const FRONT_ORDER = [3, 1, 2, 0];               /* drawn_front: north, south, west, east first */
const DOORS = new Set(['D', 'G', 'C', '=', 'B']);   /* B: barred till an event opens it (docs/PLAN-events.md) */
const WALLS = new Set(['#', 'X', 'h', 'c']);    /* stone, and the three things set in it */
const LANE_OF = { '.': '+', 'F': 'f', 'I': 'i', 'q': 'Q' };
const BASE_OF = { '+': '.', 'f': 'F', 'i': 'I', 'Q': 'q' };
const RUNS = ['THE CRYPTS', 'THE CATACOMBS', 'THE HOLLOWS', 'THE DEEP'];
/* the room types by the run whose generator lays them (pick_rtype, by G.look); the halls, her hall and the grave plot are any run's */
const ROOM_RUNS = [
    ['RT_OSSUARY', 'RT_BURIAL', 'RT_CHAPTER', 'RT_KNIGHTS', 'RT_UNDER'],
    ['RT_SAINTS', 'RT_CISTERN', 'RT_NAVE', 'RT_CANAL', 'RT_SPINNERS'],
    ['RT_GALLERY', 'RT_STOPE', 'RT_CHARNEL', 'RT_FOSSIL', 'RT_CHAPEL'],
    ['RT_GEODE', 'RT_GLASS', 'RT_CINDER', 'RT_VENT', 'RT_THRONE'],
];
const ROOM_ANY = ['RT_HALL', 'RT_MERCHANT', 'RT_PLOT'];
const RANKS = ['I', 'II', 'III', 'IV', 'V', 'GILDED'];
const ELEM_CHANCE = [0, 0.15, 0.25, 0.35, 0.45, 0];   /* elem_roll's CHANCE by rank (damage.c) */
const ELEM_FAV = [['EL_FIRE', 'EL_SPIRIT'], ['EL_FROST', 'EL_POISON'], ['EL_LIGHT', 'EL_POISON'], ['EL_FIRE', 'EL_SPIRIT']];
const GOLD_KINDS = ['IT_COINS', 'IT_CHALICE', 'IT_GEMS', 'IT_CROWN', 'IT_CHEST'];
const KEY_NAMES = ['GOLD', 'SILVER', 'CRIMSON'];
const MARK_NAMES = ['COFFER', 'CAGE', "THE CLASS'S OWN"];
const MAX = { heads: 48, mobs: 64, items: 96, fire: 96, ice: 96, rooms: 24, rest: 2, secrets: 6, fields: 8, torches: 120 };

const idx = (x, y) => y * N + x;
const inMap = (x, y) => x >= 0 && y >= 0 && x < N && y < N;
const isOpen = ch => !WALLS.has(ch);            /* is_open: a cell one may stand in (a door too) */

function blank(level) {
    return { v: 1, name: 'Untitled level', level: level || 3, face: 0, seed: '', lattice: true, corrupt: false,
             grid: new Array(N * N).fill('#'), lane: new Array(N * N).fill(0), rooms: [], things: [], stamps: [], torches: [], events: [], next: 1 };
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
        const d = DATA && stampDef(DATA, st.name); if (!d) return;
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
    const m = blank(level || D.level || D.n);
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
    const things = [], spawns = [], thingNo = {}, relicList = m.things.filter(t => t.t === 'relic');
    const relicNo = id => relicList.findIndex(r => r.id === id);
    const wakes = wakeGroups(m), groups = torchGroups(m), tags = spotTags(m), fl = n <= 2 ? [] : fields(m, cells);   /* the tutorial's two keep their own fire and ice, as the lessons have them */
    m.things.forEach(t => {
        if (t.t !== 'spawn') thingNo[t.id] = things.length;   /* the index it takes in _THING, for the triggers */
        const x = t.x - b.x0, y = t.y - b.y0, [cx, cy] = thingCell(t), sec = inMap(cx, cy) && cells[idx(cx, cy)] === 'q';
        switch (t.t) {
        case 'gold': things.push(`{ ${sec ? 'DT_SGOLD' : 'DT_GOLD'}, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'key': things.push(`{ DT_KEY, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'dead': things.push(`{ DT_DEAD, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'mark': things.push(`{ DT_MARK, ${t.k}, ${f2(x)}, ${f2(y)} }`); break;
        case 'switch': things.push(`{ DT_SWITCH, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'gseal': things.push(`{ DT_GSEAL, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'map': things.push(`{ DT_MAP, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'compass': things.push(`{ DT_COMPASS, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'relic': things.push(`{ DT_RELIC, ${relicNo(t.id)}, ${f2(x)}, ${f2(y)} }`); break;
        case 'socket': things.push(`{ DT_SOCKET, ${t.accepts ? relicNo(t.accepts) : -1}, ${f2(x)}, ${f2(y)} }`); break;
        case 'lever': things.push(`{ DT_LEVER, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'spot': things.push(`{ DT_SPOT, ${tags.indexOf(t.tag)}, ${f2(x)}, ${f2(y)} }`); break;
        case 'plate': things.push(`{ DT_PLATE, 0, ${f2(x)}, ${f2(y)} }`); break;
        case 'spawn': spawns.push(`{ ${f2(x)}, ${f2(y)}, ${t.k || -1}, ${t.chance == null ? 100 : t.chance}, ${t.echance == null ? -1 : t.echance}, ${t.el || -1}, ${t.rank === '' || t.rank == null ? -1 : 'RK_' + t.rank}${t.wake || t.spotTag ? ', ' + (t.wake ? 1 + wakes.indexOf(t.wake) : 0) : ''}${t.spotTag ? ', ' + (1 + tags.indexOf(t.spotTag)) : ''} }`); break;
        }
    });
    const marks = compose(DATA, m).marks.filter(k => k.g === 's' && cells[idx(k.x, k.y)] !== 'q');
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
        out.push(`/* spawn points: x, y; the kind or -1 the budget's choice; the chance %; the element's chance % or -1 by rank; the element or -1;`);
        out.push(` * the rank or -1 the level's usual; 1 + its wake group (0: filled as the level is laid); 1 + the tag of the spots it is laid at (0: here) */`);
        out.push(`static const DrawnSpawn ${P}_SPAWN[] = {`);
        for (let i = 0; i < spawns.length; i += 2) out.push('    ' + spawns.slice(i, i + 2).join(', ') + ',');
        out.push('};');
    }
    const ev = m.events || [], tor = m.torches || [], acts = [], trigs = [], q = v => JSON.stringify(String(v || '')).replace(/\\u[0-9a-f]{4}/gi, '?');
    const actOut = a => {
        const A = ACTIONS[a.kind]; if (!A) return;
        const arg = A.arg === 'group' ? groups.indexOf(a.group) : A.arg === 'spawn' ? wakes.indexOf(a.group) : A.arg === 'field' ? fl.findIndex(f => f.name === a.field) : A.arg === 'sound' ? (a.sound || 'SFX_DEEPCHIME') : 0;
        const dx = a.door ? a.door[0] - b.x0 : 0, dy = a.door ? a.door[1] - b.y0 : 0;
        acts.push(`{ TA_${a.kind.toUpperCase()}, ${arg}, ${dx}, ${dy}, ${f2(+a.after || 0)}, ${a.kind === 'line' ? q(String(a.text || '').toUpperCase()) : 'NULL'} }`);
    };
    ev.forEach(e => {                           /* its actions, then its actions as it goes back, each run in _ACT */
        const first = acts.length;
        (e.actions || []).forEach(actOut);
        const nact = acts.length - first, back = acts.length;
        (e.back || []).forEach(actOut);
        const nback = acts.length - back;
        let needs = 0; (e.needs || []).forEach(n => { const k = ev.findIndex(o => o.id === n); if (k >= 0) needs |= 1 << k; });
        const tg = e.target != null && thingNo[e.target] != null ? thingNo[e.target] : -1, A = e.area || { x0: b.x0, y0: b.y0, x1: b.x0 - 1, y1: b.y0 - 1 };
        trigs.push(`{ TG_${e.kind.toUpperCase()}, TT_${(e.type || 'once').toUpperCase()}, ${f2(+e.hold || 0)}, ${e.undo === false ? 0 : 1}, ${e.kind === 'enter' || e.kind === 'all' ? -1 : tg}, ${A.x0 - b.x0}, ${A.y0 - b.y0}, ${A.x1 - b.x0}, ${A.y1 - b.y0}, 0x${(needs >>> 0).toString(16)}u, ${first}, ${nact}, ${back}, ${nback} },    /* ${String(e.name || 'untitled').replace(/\*\//g, '')} */`);
    });
    if (tor.length) {
        out.push(`/* torches: x, y of the wall in the drawing; the face (0 east, 1 south, 2 west, 3 north); the group (${groups.map((g, i) => i + ' ' + g.replace(/\*\//g, '')).join(', ')}); lit at the start */`);
        out.push(`static const DrawnTorch ${P}_TORCH[] = {`);
        const tl = tor.map(t => `{ ${t.x - b.x0}, ${t.y - b.y0}, ${t.d}, ${groups.indexOf(t.group)}, ${t.lit ? 1 : 0} }`);
        for (let i = 0; i < tl.length; i += 4) out.push('    ' + tl.slice(i, i + 4).join(', ') + ',');
        out.push('};');
    }
    if (relicList.length) out.push(`static const char *const ${P}_RELIC[] = { ${relicList.map(r => q(String(r.name || 'A RELIC').toUpperCase())).join(', ')} };`);
    if (trigs.length) {
        out.push(`/* triggers: the kind; once, every time, a switch or timed; how long it holds, s (0 till touched again); whether its torches and doors go`);
        out.push(` * back with it; its thing (an index in ${P}_THING) or -1; its box (x0, y0, x1, y1); what it waits on, a bit a trigger; its actions;`);
        out.push(` * the actions as it goes back */`);
        out.push(`static const DrawnTrig ${P}_TRIG[] = {`); trigs.forEach(t => out.push('    ' + t)); out.push('};');
        out.push(`/* actions: the kind; the group or the sound; a door's cell in the drawing; after how long, s; the line */`);
        out.push(`static const DrawnAct ${P}_ACT[] = {`); acts.forEach(a => out.push('    ' + a + ',')); if (!acts.length) out.push('    { TA_LINE, 0, 0, 0, 0.0f, NULL },'); out.push('};');
    }
    const pk = m.things.filter(t => ((t.inRooms || []).length || t.spotTag) && thingNo[t.id] != null);
    if (pk.length) {
        out.push(`/* things the run's dice lay: the thing (an index in ${P}_THING); the rooms it may appear in (a bit a room in ${P}_ROOM: a spot standing`);
        out.push(` * there, else the floor nearest the middle); the tag of the spots it may appear at, or -1 */`);
        out.push(`static const DrawnPick ${P}_PICK[] = {`);
        pk.forEach(t => { let mask = 0; (t.inRooms || []).forEach(ri => { mask |= 1 << ri; }); out.push(`    { ${thingNo[t.id]}, 0x${(mask >>> 0).toString(16)}u, ${t.spotTag ? tags.indexOf(t.spotTag) : -1} },    /* ${label(DATA, t).replace(/\*\//g, '')} */`); });
        out.push('};');
    }
    if (fl.length) {
        out.push(`/* fire and ice: the kind; a cell of it in the drawing (the field is every cell of its kind joined to it); live at the start; its harm,`);
        out.push(` * % of health each half second; the chance of its status (On Fire, a Chilled stack), %; whether it may kill */`);
        out.push(`static const DrawnField ${P}_FIELD[] = {`);
        fl.forEach(f => out.push(`    { ${f.kind === 'fire' ? 'DF_FIRE' : 'DF_ICE'}, ${f.x - b.x0}, ${f.y - b.y0}, ${f.on ? 1 : 0}, ${+f.dmg || 0}, ${+f.chance || 0}, ${f.kills ? 1 : 0} },    /* ${f.name.replace(/\*\//g, '')} */`));
        out.push('};');
    }
    const evTail = trigs.length || tor.length || relicList.length || pk.length || fl.length || tags.length
        ? `, ${tor.length ? P + '_TORCH' : 'NULL'}, ${tor.length}, ${trigs.length ? P + '_TRIG' : 'NULL'}, ${trigs.length}, ${trigs.length ? P + '_ACT' : 'NULL'}, ${trigs.length ? Math.max(1, acts.length) : 0}, ${relicList.length ? P + '_RELIC' : 'NULL'}, ${relicList.length}, ${pk.length ? P + '_PICK' : 'NULL'}, ${pk.length}, ${fl.length ? P + '_FIELD' : 'NULL'}, ${fl.length}` : '';
    out.push(`/* its row in DRAWN[] */`);
    out.push(`    { ${P}_MAP, ${b.w}, ${b.h}, ${b.x0}, ${b.y0}, ${m.face}, ${P}_ROOM, (int)(sizeof ${P}_ROOM / sizeof ${P}_ROOM[0]), { ${vault.join(', ')} },`);
    out.push(`      ${P}_THING, (int)(sizeof ${P}_THING / sizeof ${P}_THING[0]), ${seed}u, ${m.level}, ${spawns.length ? P + '_SPAWN' : 'NULL'}, ${spawns.length ? `(int)(sizeof ${P}_SPAWN / sizeof ${P}_SPAWN[0])` : 0}${evTail} },`);
    return { text: out.join('\n') + '\n', ok: true, rows, b, things, spawns, vault, seed, events: !!evTail };
}

/* What the events are (docs/PLAN-events.md): each trigger's and action's name and the editor's guidance for it */
const TRIGGERS = {
    pickup: { label: 'Picked up', target: 'thing', guide: 'Fires when the player (or the dream) picks up the thing you choose: the compass, a relic, a key, gold, the map. Use it to answer a find: the lights change, a door opens, the dead wake.' },
    place: { label: 'Set in a socket', target: 'socket', guide: 'Fires when a relic is set in the socket you choose. The socket takes only the relic bound to it (or any, if you leave it open). Use it for "bring this back and put it here".' },
    strike: { label: 'Struck with a spell', target: 'mark', guide: 'Fires when a spell lands on the thing you choose: a bound coffer, a cage, a class\'s own, the switch or a seal. Use it for "shoot this to open the way".' },
    enter: { label: 'Walked into', target: 'area', guide: 'Fires the first time the player steps into the box of cells you draw. Use it for a room that answers being found, or an ambush.' },
    use: { label: 'Pulled or pressed', target: 'lever', guide: 'Fires when the player uses the lever you choose (the use key, as a door). Use it for anything worked by hand.' },
    step: { label: 'Stepped on', target: 'plate', guide: 'Fires when the player steps on the pressure plate you choose. Timed, it holds while they stand on it and for its duration after.' },
    all: { label: 'All of these done', target: 'none', guide: 'Fires once every trigger ticked under "Only after" has fired. Use it to join two or more paths: both relics set, both seals struck.' },
};
const ACTIONS = {
    flicker_on: { label: 'Torches flicker to life', arg: 'group', guide: 'The group\'s torches stutter and catch, one after another. The way it lights is the way you mean them to go.' },
    flicker_off: { label: 'Torches flicker out', arg: 'group', guide: 'The group\'s torches gutter and die. Use it to close a path that has served its turn.' },
    light_on: { label: 'Torches light', arg: 'group', guide: 'The group\'s torches light at once, without the stutter.' },
    light_off: { label: 'Torches go out', arg: 'group', guide: 'The group\'s torches go dark at once.' },
    flicker_all: { label: 'Every light flickers', arg: 'none', guide: 'Every light on the level stutters for a moment, as if something passed. A good first beat before the real change.' },
    door_open: { label: 'A door opens', arg: 'door', guide: 'The door you choose opens, a barred door (B) included. Use it to let the player on once the trigger is done.' },
    door_unlock: { label: 'A door unlocks', arg: 'door', guide: 'The door you choose stops being locked or barred, but stays shut till the player opens it.' },
    door_shut: { label: 'A door shuts', arg: 'door', guide: 'The door you choose slams and bars. The dream will not plan through it again unless another action opens it.' },
    wake: { label: 'Spawn points wake', arg: 'spawn', guide: 'Every spawn point given this wake group is filled now, not when the level is laid. Use it for an ambush that answers a find.' },
    field_on: { label: 'A field comes alive', arg: 'field', guide: 'The burning floor you choose catches, or the ice you choose freezes over, with the harm and chances set on it (choose the field on the map to set them).' },
    field_off: { label: 'A field dies down', arg: 'field', guide: 'The burning floor you choose burns low and goes out, or the ice thaws. Safe to cross while it is down.' },
    secret: { label: 'A secret\'s wall slides', arg: 'secret', guide: 'The level\'s secret wall (c) slides open as if found.' },
    line: { label: 'A line on screen', arg: 'text', guide: 'A caption at the top of the view, in capitals, as the game\'s own are ("THE TORCHES TO THE NORTH CATCH"). Keep it short: about 40 characters.' },
    sound: { label: 'A sound', arg: 'sound', guide: 'One of the game\'s own sounds.' },
};
const SOUNDS = ['SFX_DEEPCHIME', 'SFX_SHIMMER', 'SFX_KNOCK', 'SFX_KEYGET', 'SFX_BONES', 'SFX_WAIL'];
const SOUND_NAMES = ['a deep chime', 'a shimmer', 'a knock', 'a key\'s ring', 'bones', 'a wail'];

function frontOf(cells, x, y, secretOk) {       /* drawn_front: a wall's open side, north, south, west, east first; a secret
                                                   room's floor only when nothing else is open and it may be (a place to mend) */
    for (let pass = 0; pass < (secretOk ? 2 : 1); pass++)
        for (const d of FRONT_ORDER) {
            const fx = x + DIRS[d][0], fy = y + DIRS[d][1];
            if (inMap(fx, fy) && isOpen(cells[idx(fx, fy)]) && (pass || cells[idx(fx, fy)] !== 'q')) return [fx, fy];
        }
    return null;
}
/* what a trigger of each kind may watch */
function canWatch(kind, t) {
    switch ((TRIGGERS[kind] || {}).target) {
    case 'thing': return ['gold', 'key', 'map', 'compass', 'relic'].includes(t.t);
    case 'socket': return t.t === 'socket';
    case 'mark': return ['mark', 'switch', 'gseal'].includes(t.t);
    case 'lever': return t.t === 'lever';
    case 'plate': return t.t === 'plate';
    }
    return false;
}
function torchGroups(m) { const g = []; (m.torches || []).forEach(t => { if (t.group && !g.includes(t.group)) g.push(t.group); }); return g; }
function wakeGroups(m) { const g = []; m.things.forEach(t => { if (t.t === 'spawn' && t.wake && !g.includes(t.wake)) g.push(t.wake); }); return g; }
/* Where a thing stands in one play. A thing tagged with rooms appears in one of them, by the run's dice: on its own spot if that room
 * holds it, else on the floor cell nearest the room's middle that is within a step of a lane, not a door, not taken. The game makes
 * the same choice of cell (docs/PLAN-events.md); only the room is the dice's. */
function roomSpot(m, cells, L, t, ri, taken) {
    const r = m.rooms[ri]; if (!r) return null;
    const [cx, cy] = thingCell(t);
    if (cx >= r.x && cy >= r.y && cx < r.x + r.w && cy < r.y + r.h) return [t.x, t.y];
    let best = null, bd = 1e9;
    const mx = r.x + (r.w - 1) / 2, my = r.y + (r.h - 1) / 2;
    for (let y = r.y; y < r.y + r.h; y++) for (let x = r.x; x < r.x + r.w; x++) {
        if (!inMap(x, y)) continue;
        const ch = cells[idx(x, y)];
        if (!isOpen(ch) || DOORS.has(ch) || ch === 'S' || (taken && taken.has(idx(x, y)))) continue;
        if (!L[idx(x, y)] && !DIRS.some(([dx, dy]) => inMap(x + dx, y + dy) && L[idx(x + dx, y + dy)])) continue;
        const d = Math.abs(x - mx) + Math.abs(y - my) + (x + y) * 1e-4;   /* ties go up and left, the same every time */
        if (d < bd) { bd = d; best = [x + 0.5, y + 0.5]; }
    }
    return best;
}
function picks(m) { return m.things.filter(t => (t.inRooms || []).length || t.spotTag); }
function spotTags(m) { const g = []; m.things.forEach(t => { if (t.t === 'spot' && t.tag && !g.includes(t.tag)) g.push(t.tag); }); return g; }
/* the places the dice may lay a thing: one of the spots tagged for it; or for each room it may appear in, each spot standing in that
 * room (a spot of any tag), or, with none there, the floor nearest the room's middle (roomSpot). Each { pos, label } */
function pickOptions(m, cells, L, t) {
    const out = [], spots = m.things.filter(s => s.t === 'spot');
    if (t.spotTag) spots.filter(s => s.tag === t.spotTag).forEach(s => out.push({ pos: [s.x, s.y], label: `the spot at (${Math.floor(s.x)},${Math.floor(s.y)})`, spot: s.id }));
    (t.inRooms || []).forEach(ri => {
        const r = m.rooms[ri]; if (!r) return;
        const here = spots.filter(s => s.x >= r.x && s.y >= r.y && s.x < r.x + r.w && s.y < r.y + r.h);
        if (here.length) here.forEach(s => out.push({ pos: [s.x, s.y], label: `room ${ri + 1}'s spot at (${Math.floor(s.x)},${Math.floor(s.y)})`, spot: s.id, room: ri }));
        else { const p = roomSpot(m, cells, L, t, ri, null); if (p) out.push({ pos: p, label: `room ${ri + 1}`, room: ri }); }
    });
    return out;
}
/* every way the dice can lay the tagged things (an option's index for each): all of them up to 64, else each thing's options in turn */
function layouts(m, cells, L) {
    const P = picks(m); if (!P.length) return [{}];
    if (!cells) { cells = compose(null, m).cells; L = lanes(m, cells); }
    const n = P.map(t => pickOptions(m, cells, L, t).length);
    let all = [{}];
    for (let k = 0; k < P.length; k++) { const nx = []; for (const l of all) for (let o = 0; o < Math.max(1, n[k]); o++) nx.push(Object.assign({}, l, { [P[k].id]: o })); all = nx; if (all.length > 64) break; }
    if (all.length <= 64) return all;
    const one = [{}]; P.forEach((t, k) => { for (let o = 0; o < n[k]; o++) one.push({ [t.id]: o }); }); return one;
}
/* the fire and ice fields: each connected patch of burning floor (F) or ice (I), its settings kept by a cell in it (m.fields) */
const FIELD_DEF = { fire: { dmg: 17, chance: 100, on: true, kills: false }, ice: { dmg: 0, chance: 100, on: true, kills: false } };
function fields(m, cells) {
    const seen = new Uint8Array(N * N), out = [];
    for (let i = 0; i < N * N; i++) {
        const ch = cells[i]; if ((ch !== 'F' && ch !== 'I') || seen[i]) continue;
        const kind = ch === 'F' ? 'fire' : 'ice', q = [i], c = []; seen[i] = 1;
        for (let h = 0; h < q.length; h++) { const j = q[h]; c.push([j % N, (j / N) | 0]); for (const [dx, dy] of DIRS) { const x = j % N + dx, y = ((j / N) | 0) + dy; if (inMap(x, y) && !seen[idx(x, y)] && cells[idx(x, y)] === ch) { seen[idx(x, y)] = 1; q.push(idx(x, y)); } } }
        const set = (m.fields || []).find(f => (!f.kind || f.kind === kind) && c.some(([x, y]) => x === f.x && y === f.y));
        const k = out.filter(f => f.kind === kind).length + 1;
        out.push(Object.assign({ kind, cells: c, x: c[0][0], y: c[0][1], name: (kind === 'fire' ? 'Fire ' : 'Ice ') + k }, FIELD_DEF[kind], set || {}, { kind, cells: c, key: set ? `${set.x},${set.y}` : `${c[0][0]},${c[0][1]}` }));
    }
    return out;
}
function fieldByName(m, cells, name) { return fields(m, cells).find(f => f.name === name); }
const CELL_S = 1 / 2.1;                         /* the dream's pace: CRUISE, cells a second (defs.c); slower round corners */
const TRIG_TYPES = {
    once: { label: 'Once', guide: 'Fires the first time, and what it does stays done.' },
    every: { label: 'Every time', guide: 'Fires each time it is touched. Its actions run again each time: good for a line, a sound or the dead waking.' },
    toggle: { label: 'Switch', guide: 'The first touch turns it on, the next turns it back: torches and doors go back as they were. A duration makes it go back by itself after that many seconds.' },
    timed: { label: 'Timed', guide: 'On for the duration you set, then back as it was: a door held open while the player runs for it, torches that burn for a while.' },
};
/* an action's way back, for a trigger that returns: torches and doors as they were; the rest have none */
const UNDO = { field_on: 'field_off', field_off: 'field_on', flicker_on: 'flicker_off', flicker_off: 'flicker_on', light_on: 'light_off', light_off: 'light_on', door_open: 'door_shut', door_unlock: 'door_shut', door_shut: 'door_open' };
/* The dream's walk: lanes only; a key opens its doors, the seal the gate; a relic carried to the socket that takes it, one at a time; a
 * trigger fires when its thing is reached and what it waits on has fired, and its actions open and shut doors, for good or for a time.
 * The time is the dream's pace, so a timed door shut before it gets there says so. A copy of the dream's rules (goals.c), not the
 * game's own code. opts.layout: the room each tagged thing appears in. */
function dream(DATA, m, opts) {
    opts = opts || {};
    const { cells } = compose(DATA, m), L = lanes(m, cells), evs = m.events || [], lay = opts.layout || {};
    const st = { keys: [0, 0, 0], gate: false, secret: !!opts.secrets, doors: new Map(), until: new Map(), carry: null, fired: new Map(), on: new Set(), placed: new Set(), now: 0 };
    const pos = new Map(), taken = new Set();
    m.things.forEach(t => { if (!(t.id in lay)) { pos.set(t.id, [t.x, t.y]); taken.add(idx(...thingCell(t))); } });
    m.things.forEach(t => {
        if (!(t.id in lay)) return;
        const o = pickOptions(m, cells, L, t)[lay[t.id]], p = o ? o.pos : [t.x, t.y];
        pos.set(t.id, p); taken.add(idx(Math.floor(p[0]), Math.floor(p[1])));
    });
    m.things.forEach(t => { if (t.t === 'spot') pos.delete(t.id); });
    const cellOf = t => { const p = pos.get(t.id) || [t.x, t.y]; return [Math.floor(p[0]), Math.floor(p[1])]; };
    const doorOpen = (i, arrive) => { if (!st.doors.get(i)) return false; const u = st.until.get(i); return u == null || arrive < u; };
    const pass = (x, y, arrive) => {
        if (!inMap(x, y)) return false;
        const i = idx(x, y), ch = cells[i];
        if (!L[i]) return false;
        if (st.doors.has(i)) return doorOpen(i, arrive);
        if (ch === 'G') return !!st.keys[0];
        if (ch === 'C') return !!st.keys[2];
        if (ch === '=') return st.gate;
        if (ch === 'B') return false;
        if (ch === 'c' || ch === 'q') return st.secret;
        return true;
    };
    let start = null;
    for (let i = 0; i < N * N; i++) if (cells[i] === 'S') { start = [i % N, (i / N) | 0]; break; }
    const res = { route: [], events: [], ok: false, reached: new Set(), missed: [], unfired: [], exit: null, steps: 0, lights: [], fieldLog: [], pos, layout: lay, seconds: 0 };
    (m.torches || []).forEach(t => res.lights.push({ at: 0, group: t.group, on: !!t.lit, init: true }));
    if (!start) { res.events.push({ at: 0, msg: 'There is no start (S): the dream has nowhere to begin.' }); return res; }
    let exit = null;
    for (let i = 0; i < N * N; i++) if (cells[i] === 'X') { exit = frontOf(cells, i % N, (i / N) | 0); }
    res.exit = exit;
    const at = () => res.route.length - 1, secs = () => st.now.toFixed(1) + ' s';
    const ready = e => (e.needs || []).every(n => st.fired.has(n));
    const trigOf = (kind, id) => evs.filter(e => e.kind === kind && e.target === id);
    const returns = [];                         /* triggers due to go back: { e, t } */
    const doAct = (a, e, back) => {
        const A = ACTIONS[a.kind]; if (!A) return;
        let msg = A.label;
        const timed = !back && (e.type === 'timed' || (e.type === 'toggle' && +e.hold > 0)) && e.undo !== false;
        if (timed && A.arg === 'door' && a.door && a.kind !== 'door_shut') res.timedDoors = (res.timedDoors || 0) + 1;
        if (A.arg === 'group') { msg += `: ${a.group || '(no group)'}`; res.lights.push({ at: at(), group: a.group, on: a.kind === 'flicker_on' || a.kind === 'light_on' }); }
        if (A.arg === 'door' && a.door) {
            const i = idx(a.door[0], a.door[1]), open = a.kind !== 'door_shut';
            st.doors.set(i, open);
            if (open && timed) st.until.set(i, st.now + (+a.after || 0) + +e.hold); else st.until.delete(i);
            msg += ` at (${a.door[0]},${a.door[1]})`;
            if (open && timed && cur) {             /* how far it is, against how long it holds */
                const d = bfs(cur[0], cur[1], true).dist[i];
                if (d >= 0) msg += `, ${d} cells off: about ${(d * CELL_S).toFixed(1)} s at the dream's pace, and it holds ${(+e.hold).toFixed(1)} s`;
                if (d >= 0 && d * CELL_S > +e.hold + (+a.after || 0)) { res.events.push({ at: at(), msg: '  ' + msg + '.', warn: true }); return; }
            }
        }
        if (a.kind === 'secret') st.secret = true;
        if (A.arg === 'field') { msg += `: ${a.field || '(no field)'}`; res.fieldLog.push({ at: at(), field: a.field, on: a.kind === 'field_on' }); }
        if (A.arg === 'spawn') msg += `: ${a.group || '(no group)'}`;
        if (a.kind === 'line') msg += `: "${a.text || ''}"`;
        if (a.after) msg += ` (after ${a.after} s)`;
        res.events.push({ at: at(), msg: '  ' + msg + '.' });
    };
    const goBack = e => {
        st.on.delete(e.id);
        res.events.push({ at: at(), msg: `Trigger "${e.name || 'untitled'}" goes back, at ${secs()}.`, trig: e.id });
        if (e.undo !== false) (e.actions || []).slice().reverse().forEach(a => {
            const u = UNDO[a.kind]; if (!u) return;
            if (a.door) {                       /* a door shut again: did it shut on the dream's way? */
                const i = idx(a.door[0], a.door[1]);
                if (u === 'door_shut' && !res.route.some(([x, y]) => idx(x, y) === i) && st.doors.get(i)) {
                    res.events.push({ at: at(), msg: `  The door at (${a.door[0]},${a.door[1]}) shuts before the dream has been through it.`, warn: true });
                    res.shut = (res.shut || 0) + 1;
                }
            }
            doAct(Object.assign({}, a, { kind: u, after: 0 }), e, true);
        });
        (e.back || []).forEach(a => doAct(a, e, true));
    };
    const fire = e => {
        if (!ready(e)) return;
        const n = st.fired.get(e.id) || 0, type = e.type || 'once';
        if (type === 'once' && n) return;
        if (type === 'toggle' && st.on.has(e.id)) { goBack(e); return; }
        if (type === 'timed' && st.on.has(e.id)) return;
        st.fired.set(e.id, n + 1);
        res.events.push({ at: at(), msg: `Trigger "${e.name || 'untitled'}" fires${n ? ' again' : ''}, at ${secs()}.`, trig: e.id });
        (e.actions || []).forEach(a => doAct(a, e, false));
        if (type === 'toggle' || type === 'timed') {
            st.on.add(e.id);
            if (+e.hold > 0) returns.push({ e, t: st.now + +e.hold });
        }
        evs.forEach(o => { if (o.kind === 'all' && !st.fired.has(o.id) && (o.needs || []).length && ready(o)) fire(o); });
    };
    const relics = m.things.filter(t => t.t === 'relic'), sockets = m.things.filter(t => t.t === 'socket');
    const takes = (s, r) => !s.accepts || s.accepts === r.id;
    const near = c => [[0, 0], ...DIRS].map(([dx, dy]) => [c[0] + dx, c[1] + dy]);
    const sec = c => inMap(c[0], c[1]) && cells[idx(c[0], c[1])] === 'q';
    /* what it wants now; again: the triggers it would touch a second time, when nothing else is left */
    const wants = again => {
        const w = [];
        m.things.forEach(t => {
            const c = cellOf(t);
            if (sec(c) && !st.secret) return;
            if (['lever', 'plate', 'mark', 'switch', 'gseal'].includes(t.t)) {
                const tr = trigOf(t.t === 'lever' ? 'use' : t.t === 'plate' ? 'step' : 'strike', t.id).filter(ready);
                const fresh = tr.filter(e => !st.fired.has(e.id)), redo = tr.filter(e => st.fired.has(e.id) && e.type && e.type !== 'once' && !st.on.has(e.id) && (st.fired.get(e.id) || 0) < 4);
                if (fresh.length || (again && redo.length)) w.push({ cells: t.t === 'plate' ? [c] : near(c), t, why: 'touch' });
                else if ((t.t === 'gseal' || t.t === 'switch') && !res.reached.has(t.id) && !tr.length && !trigOf('strike', t.id).length) w.push({ cells: near(c), t, why: 'take' });
                return;
            }
            if (res.reached.has(t.id)) return;
            if (['gold', 'key', 'map', 'compass'].includes(t.t)) {
                const tr = trigOf('pickup', t.id);
                if (tr.length && !tr.every(ready)) return;
                w.push({ cells: near(c), t, why: 'take' });
            } else if (t.t === 'relic') {
                if (st.carry || !sockets.some(s => !st.placed.has(s.id) && takes(s, t))) return;
                const tr = trigOf('pickup', t.id);
                if (tr.length && !tr.every(ready)) return;
                w.push({ cells: near(c), t, why: 'relic' });
            } else if (t.t === 'socket') {
                if (!st.carry || st.placed.has(t.id) || !takes(t, st.carry)) return;
                const tr = trigOf('place', t.id);
                if (tr.length && !tr.every(ready)) return;
                w.push({ cells: near(c), t, why: 'socket' });
            }
        });
        evs.forEach(e => {
            if (e.kind !== 'enter' || st.fired.has(e.id) || !ready(e) || !e.area) return;
            const c = [];
            for (let y = e.area.y0; y <= e.area.y1; y++) for (let x = e.area.x0; x <= e.area.x1; x++) c.push([x, y]);
            w.push({ cells: c, e, why: 'enter' });
        });
        return w;
    };
    const arrive = (x, y, again) => {
        evs.forEach(e => {
            if (e.kind !== 'enter' || !e.area || x < e.area.x0 || x > e.area.x1 || y < e.area.y0 || y > e.area.y1) return;
            if (!st.fired.has(e.id) || (e.type === 'every' && !st.inArea.has(e.id))) fire(e);
        });
        st.inArea = new Set(evs.filter(e => e.kind === 'enter' && e.area && x >= e.area.x0 && x <= e.area.x1 && y >= e.area.y0 && y <= e.area.y1).map(e => e.id));
        for (const w of wants(again)) {
            if (!w.t || !w.cells.some(c => c[0] === x && c[1] === y)) continue;
            const t = w.t, [tx, ty] = cellOf(t), where = `(${tx},${ty})`;
            if (w.why === 'touch') {
                res.reached.add(t.id);
                const verb = t.t === 'lever' ? 'Pulls the lever' : t.t === 'plate' ? 'Steps on the plate' : `Strikes ${label(DATA, t).toLowerCase()}`;
                res.events.push({ at: at(), msg: `${verb} at ${where}.` });
                if (t.t === 'gseal' && !st.gate) { st.gate = true; res.events.push({ at: at(), msg: '  The gate opens.' }); }
                trigOf(t.t === 'lever' ? 'use' : t.t === 'plate' ? 'step' : 'strike', t.id).forEach(fire);
            } else if (w.why === 'take') {
                res.reached.add(t.id);
                if (t.t === 'key') { st.keys[t.k] = 1; res.events.push({ at: at(), msg: `Takes the ${KEY_NAMES[t.k].toLowerCase()} key at ${where}.` }); }
                else if (t.t === 'gseal') { st.gate = true; res.events.push({ at: at(), msg: `Strikes the gate's seal at ${where}: the gate opens.` }); }
                else if (t.t === 'map') res.events.push({ at: at(), msg: `Takes the map at ${where}.` });
                else if (t.t === 'switch') res.events.push({ at: at(), msg: `Strikes the switch at ${where}.` });
                else if (t.t === 'compass') res.events.push({ at: at(), msg: `Takes the compass at ${where}.` });
                trigOf('pickup', t.id).forEach(fire);
            } else if (w.why === 'relic') {
                res.reached.add(t.id); st.carry = t;
                res.events.push({ at: at(), msg: `Takes up ${t.name || 'the relic'} at ${where}.` });
                trigOf('pickup', t.id).forEach(fire);
            } else if (w.why === 'socket') {
                res.reached.add(t.id); st.placed.add(t.id);
                res.events.push({ at: at(), msg: `Sets ${st.carry.name || 'the relic'} in ${t.name || 'the socket'} at ${where}.` });
                st.carry = null;
                trigOf('place', t.id).forEach(fire);
            }
        }
    };
    st.inArea = new Set();
    const bfs = (sx, sy, timeless) => {         /* timeless: as things stand now, however long the walk */
        const dist = new Int32Array(N * N).fill(-1), prev = new Int32Array(N * N).fill(-1), q = [idx(sx, sy)];
        dist[idx(sx, sy)] = 0;
        for (let h = 0; h < q.length; h++) {
            const i = q[h], x = i % N, y = (i / N) | 0;
            for (const [dx, dy] of DIRS) {
                const nx = x + dx, ny = y + dy;
                if (!inMap(nx, ny) || dist[idx(nx, ny)] >= 0 || !pass(nx, ny, timeless ? st.now : st.now + (dist[i] + 1) * CELL_S)) continue;
                dist[idx(nx, ny)] = dist[i] + 1; prev[idx(nx, ny)] = i; q.push(idx(nx, ny));
            }
        }
        return { dist, prev };
    };
    const pathTo = (prev, i) => { const p = []; while (i >= 0) { p.push([i % N, (i / N) | 0]); i = prev[i]; } return p.reverse(); };
    const tick = () => {                        /* time passes a step; what is due goes back */
        st.now += CELL_S;
        returns.sort((a, b) => a.t - b.t);
        while (returns.length && returns[0].t <= st.now) { const r = returns.shift(); if (st.on.has(r.e.id)) goBack(r.e); }
    };
    let cur = null;
    cur = start; res.route.push(cur.slice());
    arrive(cur[0], cur[1], false);
    const step = (c, again) => {
        const was = cells[idx(c[0], c[1])];
        tick();
        cur = c; res.route.push(c.slice());
        if (was === 'G' || was === 'C') res.events.push({ at: at(), msg: `Opens the ${was === 'G' ? 'gold' : 'crimson'} door at (${c[0]},${c[1]}).` });
        arrive(c[0], c[1], again);
    };
    const go = again => {
        const { dist, prev } = bfs(cur[0], cur[1]);
        let best = null, bd = 1e9;
        for (const w of wants(again)) for (const [x, y] of w.cells) {
            if (!inMap(x, y)) continue;
            const d = dist[idx(x, y)];
            if (d > 0 && d < bd) { bd = d; best = idx(x, y); }
        }
        if (best === null) return false;
        pathTo(prev, best).slice(1).forEach(c => step(c, again));
        return true;
    };
    const toExit = () => {
        if (!exit) return false;
        const { dist, prev } = bfs(cur[0], cur[1]);
        if (dist[idx(exit[0], exit[1])] < 0) return false;
        pathTo(prev, idx(exit[0], exit[1])).slice(1).forEach(c => step(c, false));
        return true;
    };
    let tries = 0;
    for (let guard = 0; guard < 800; guard++) {
        if (go(false)) continue;
        if (exit && bfs(cur[0], cur[1]).dist[idx(exit[0], exit[1])] >= 0) break;
        if (returns.length && tries < 6) {          /* stuck while something is due to go back: it waits for it */
            returns.sort((a, b) => a.t - b.t);
            const w = returns[0].t - st.now;
            if (w > 0) { res.events.push({ at: at(), msg: `Waits ${w.toFixed(1)} s.` }); st.now = returns[0].t - CELL_S / 2; }
            tick();
        }
        if (tries++ < 6 && go(true)) continue;  /* stuck: try a switch or a timed trigger again, and run */
        if (returns.length && tries < 6) continue;
        break;
    }
    if (toExit()) { res.events.push({ at: at(), msg: `Reaches the exit at (${exit[0]},${exit[1]}), at ${secs()}.` }); res.ok = true; }
    else res.events.push({ at: at(), msg: exit ? 'Cannot reach the exit from here: the level cannot be finished.' : 'There is no exit (X) with an open side.' });
    if (st.carry) res.events.push({ at: at(), msg: `Still carries ${st.carry.name || 'a relic'}: no socket would take it.` });
    res.missed = m.things.filter(t => ['gold', 'key', 'map', 'gseal', 'switch', 'compass', 'relic', 'socket'].includes(t.t) && !res.reached.has(t.id));
    res.unfired = evs.filter(e => !st.fired.has(e.id));
    res.steps = res.route.length - 1; res.seconds = st.now;
    return res;
}
/* the dream through every way the dice can lay the tagged things: the first that cannot finish, or the first of them all */
function dreamAll(DATA, m, opts) {
    const cm = compose(DATA, m).cells, L = layouts(m, cm, lanes(m, cm)), runs = L.map(l => dream(DATA, m, Object.assign({}, opts || {}, { layout: l })));
    const bad = runs.filter(r => !r.ok);
    const r = bad[0] || runs[0];
    r.layouts = runs.length; r.failed = bad.length;
    return r;
}
/* whether a field is live at a step of the walk: as set, then as the last action on it says */
function fieldLive(res, f, step) { let on = !!f.on; (res ? res.fieldLog : []).forEach(l => { if (l.at <= step && l.field === f.name) on = l.on; }); return on; }
/* whether a torch is lit at a step of the walk: as placed, then as the last action on its group says */
function torchLit(res, t, step) { let on = !!t.lit; (res ? res.lights : []).forEach(l => { if (!l.init && l.at <= step && l.group === t.group) on = l.on; }); return on; }

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
    X.forEach(c => {
        const f = frontOf(cells, c[0], c[1]);
        if (!f) add('error', 'The exit has no open side to be used from.', [c]);
        else if (!L[idx(f[0], f[1])]) add('error', `The exit is used from (${f[0]},${f[1]}), which is off the dream's lanes: it can never leave.`, [f]);
    });
    const H = find('h');
    if (H.length > MAX.rest) add('warn', `There are ${H.length} places to mend: the game keeps ${MAX.rest}.`, H.slice(MAX.rest));
    H.forEach(c => {
        const f = frontOf(cells, c[0], c[1], true);
        if (!f) add('error', 'A place to mend has no open side to be used from.', [c]);
        else if (!L[idx(f[0], f[1])]) add('warn', `A place to mend is used from (${f[0]},${f[1]}), off the dream's lanes: the dream cannot mend there.`, [f]);
    });

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
    const evDoor = new Set(); for (const e of (m.events || [])) for (const a of (e.actions || []).concat(e.back || [])) if (a.door) evDoor.add(a.door.join());
    const byEvent = gates.filter(g => evDoor.has(g.join())), hint = byEvent.length ? ' An event opens or shuts the one at ' + byEvent.map(g => '(' + g + ')').join(' and ') + ': a door an event works is a Barred door (B), not a gate.' : '';
    if (gates.length > 1) add('error', 'There is more than one gate: the game keeps one.' + hint, gates);
    if (gates.length && !seals.length) add('error', 'The gate has no seal to open it.' + (gates.length > 1 ? '' : hint), gates);
    if (seals.length && !gates.length) add('warn', 'There is a gate\'s seal but no gate.', seals.map(thingCell));
    const fire = cells.filter(c => c === 'F').length, ice = cells.filter(c => c === 'I').length;
    if (fire > MAX.fire) add('error', `${fire} cells of burning floor: a level keeps ${MAX.fire}.`, find('F').slice(MAX.fire));
    if (ice > MAX.ice) add('error', `${ice} cells of ice: a level keeps ${MAX.ice}.`, find('I').slice(MAX.ice));
    if (ice && !m.things.some(t => t.t === 'switch') && !(m.events || []).some(e => (e.actions || []).some(a => a.kind === 'field_off'))) add('warn', 'There is ice but no switch and no trigger to thaw it.', find('I'));

    /* rooms */
    if (m.rooms.length > MAX.rooms) add('error', `${m.rooms.length} rooms: a drawn level keeps ${MAX.rooms}.`);
    else if (m.rooms.length > 12) add('info', `${m.rooms.length} rooms: the game keeps 12 today; 24 once the events' game side is in (docs/PLAN-events.md).`);
    m.things.forEach(t => {
        (t.inRooms || []).forEach(ri => {
            if (!m.rooms[ri]) add('error', `${label(DATA, t)} may appear in a room that is gone.`, [thingCell(t)]);
            else if (!roomSpot(m, cells, L, t, ri, null)) add('error', `${label(DATA, t)} may appear in room ${ri + 1}, which has no floor the dream can reach.`, [[m.rooms[ri].x, m.rooms[ri].y]]);
        });
        if (t.spotTag && !m.things.some(s => s.t === 'spot' && s.tag === t.spotTag)) add('error', `${label(DATA, t)} appears at a spot tagged "${t.spotTag}", and no spot has that tag.`, [thingCell(t)]);
        if (t.t === 'spot' && !isOpen(at(...thingCell(t)))) add('error', 'A spot stands in stone.', [thingCell(t)]);
        if ((t.inRooms || []).length === 1 && !t.spotTag) add('info', `${label(DATA, t)} has one room to appear in: it is always there.`, [thingCell(t)]);
    });
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
    if (C.length > MAX.secrets) add('error', `${C.length} secrets: a level keeps ${MAX.secrets}.`, C.slice(MAX.secrets));
    else if (C.length > 1) add('info', `${C.length} secrets, each its own room: the game takes one a drawn level today; more once the events' game side is in (docs/PLAN-events.md).`, C);
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
        if (t.spotTag) return;                  /* it stands where its spots are, not where it was set down */
        if (!isOpen(at(x, y))) add('error', `A ${label(DATA, t).toLowerCase()} stands in stone.`, [[x, y]]);
        else if (DOORS.has(at(x, y))) add('warn', `A ${label(DATA, t).toLowerCase()} stands in a doorway.`, [[x, y]]);
    });
    const restF = H.map(c => frontOf(cells, c[0], c[1], true)).filter(Boolean);
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
    /* the fire and ice fields */
    const fl = fields(m, cells);
    if (fl.length > MAX.fields) add('error', `${fl.length} fields of fire and ice: a level keeps ${MAX.fields}.`);
    (m.fields || []).forEach(f => { if (!fl.some(x => x.key === `${f.x},${f.y}`)) add('info', `The settings for "${f.name || 'a field'}" stand on a cell no longer burning or iced: they are not used.`, [[f.x, f.y]]); });
    if (new Set(fl.map(f => f.name)).size < fl.length) add('error', 'Two fields share a name: give each its own, so a trigger knows which it means.');
    fl.forEach(f => { if (f.dmg > 50) add('warn', `${f.name} takes ${f.dmg}% of health a half second: two steps on it could end a run.`, [f.cells[0]]); });
    /* the events (docs/PLAN-events.md) */
    const ev = m.events || [], tor = m.torches || [], byId = id => m.things.find(t => t.id === id);
    if (ev.length > 32) add('error', `${ev.length} triggers: a level holds 32 at most (what a trigger waits on is a bit each).`);
    if (tor.length > MAX.torches) add('warn', `${tor.length} torches hung: the game keeps the first ${MAX.torches}, in the order they were hung.`, tor.slice(MAX.torches).map(t => [t.x, t.y]));
    tor.forEach(t => {
        const f = [t.x + DIRS[t.d][0], t.y + DIRS[t.d][1]];
        if (WALLS.has(at(t.x, t.y)) === false || at(t.x, t.y) !== '#') add('error', 'A torch must hang on plain stone.', [[t.x, t.y]]);
        else if (!isOpen(at(f[0], f[1]))) add('error', 'A torch faces into stone: turn it toward the floor.', [[t.x, t.y]]);
        if (!t.group) add('info', 'A torch with no group: no event can light or snuff it.', [[t.x, t.y]]);
    });
    const relics = m.things.filter(t => t.t === 'relic'), sockets = m.things.filter(t => t.t === 'socket');
    relics.forEach(r => { if (!sockets.some(s => !s.accepts || s.accepts === r.id)) add('warn', `${label(DATA, r)} has no socket that takes it.`, [thingCell(r)]); });
    sockets.forEach(s => { if (s.accepts && !relics.some(r => r.id === s.accepts)) add('error', `${label(DATA, s)} is bound to a relic that is no longer on the map.`, [thingCell(s)]); });
    if (m.things.filter(t => t.t === 'compass').length > 1) add('warn', 'More than one compass: the second is gold in its place, as a held compass is.');
    const groups = torchGroups(m), wakes = wakeGroups(m);
    ev.forEach(e => {
        const nm = `"${e.name || 'untitled'}"`, T = TRIGGERS[e.kind];
        if (!T) { add('error', `Trigger ${nm} has no kind.`); return; }
        if (T.target !== 'none' && T.target !== 'area') {
            const t = byId(e.target);
            const what = { socket: 'socket', mark: 'thing to strike', lever: 'lever', plate: 'pressure plate', thing: 'thing to pick up' }[T.target];
            if (!t) add('error', `Trigger ${nm} has nothing to watch: choose its ${what}.`);
            else if (!canWatch(e.kind, t)) add('error', `Trigger ${nm} watches ${label(DATA, t).toLowerCase()}, which is not a ${what}.`, [thingCell(t)]);
            if (t && e.kind === 'pickup' && (e.needs || []).length) add('info', `Trigger ${nm} waits on others, but the player may take its thing first; then it never fires. The dream waits.`, [thingCell(t)]);
        }
        const ty = e.type || 'once';
        if (ty === 'timed' && !(+e.hold > 0)) add('error', `Trigger ${nm} is timed but has no duration.`);
        if ((ty === 'toggle' || ty === 'timed') && e.kind === 'pickup') add('warn', `Trigger ${nm} goes back, but a thing picked up stays picked up: only its torches and doors return.`);
        if ((ty === 'toggle' || ty === 'timed') && e.kind === 'place') add('warn', `Trigger ${nm} goes back, but a relic once set stays set: only its torches and doors return.`);
        if (e.kind === 'enter' && !e.area) add('error', `Trigger ${nm} has no box to walk into: draw it on the map.`);
        if (e.kind === 'all' && !(e.needs || []).length) add('error', `Trigger ${nm} waits on nothing: tick what it joins under "Only after".`);
        (e.needs || []).forEach(n => { if (!ev.some(o => o.id === n)) add('error', `Trigger ${nm} waits on a trigger that is gone.`); });
        if (!(e.actions || []).length) add('warn', `Trigger ${nm} does nothing yet: give it an action.`);
        (e.actions || []).forEach(a => {
            const A = ACTIONS[a.kind]; if (!A) return;
            if (A.arg === 'group' && !groups.includes(a.group)) add('error', `Trigger ${nm}: "${A.label}" names a torch group no torch is in.`);
            if (A.arg === 'spawn' && !wakes.includes(a.group)) add('error', `Trigger ${nm}: "${A.label}" names a wake group no spawn point is in.`);
            if (A.arg === 'door' && (!a.door || !DOORS.has(at(a.door[0], a.door[1])))) add('error', `Trigger ${nm}: "${A.label}" has no door chosen.`, a.door ? [a.door] : []);
            if (A.arg === 'field' && !fl.some(f => f.name === a.field)) add('error', `Trigger ${nm}: "${A.label}" names no field on the map.`);
            if (A.arg === 'secret' && !find('c').length) add('error', `Trigger ${nm}: there is no secret's wall to slide.`);
            if (a.kind === 'line' && String(a.text || '').length > 44) add('warn', `Trigger ${nm}: its line is ${String(a.text).length} characters; the view holds about 40.`);
        });
    });
    const seen = new Set(), loop = id => { if (seen.has(id)) return true; seen.add(id); const e = ev.find(o => o.id === id); const r = e && (e.needs || []).some(loop); seen.delete(id); return r; };
    ev.forEach(e => { if (loop(e.id)) add('error', `Trigger "${e.name || 'untitled'}" waits, through others, on itself: it can never fire.`); });
    find('B').forEach(c => { if (!ev.some(e => (e.actions || []).some(a => (a.kind === 'door_open' || a.kind === 'door_unlock') && a.door && a.door[0] === c[0] && a.door[1] === c[1]))) add('warn', 'A barred door no event opens: it stays shut for good.', [c]); });
    if (ev.length || tor.length) add('info', 'This level has events: its export needs the game\'s side of them (docs/PLAN-events.md, steps 3 to 5) before it compiles.');
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
    case 'compass': return 'The compass';
    case 'relic': return 'Relic' + (t.name ? ': ' + t.name.toLowerCase() : '');
    case 'socket': return 'Socket' + (t.name ? ': ' + t.name.toLowerCase() : '');
    case 'spot': return 'Spot' + (t.tag ? ': ' + t.tag : '');
    case 'lever': return 'Lever' + (t.name ? ': ' + t.name.toLowerCase() : '');
    case 'plate': return 'Pressure plate' + (t.name ? ': ' + t.name.toLowerCase() : '');
    }
    return t.t;
}

/* The owner's example (docs/PLAN-events.md): the compass in a hub; taking it flickers every light and wakes path A's torches to the
 * north; the bone found there and set in the hub's socket snuffs path A and lights path B to the east, whose barred door opens on
 * the exit. Path C to the south lights itself when its room is walked into. */
function sampleEvents() {
    const m = blank(3); m.name = 'The compass (an example of events)';
    const fill = (x0, y0, x1, y1, ch) => { for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) m.grid[idx(x, y)] = ch; };
    fill(5, 17, 11, 27, '.');        /* the start's room */
    fill(12, 22, 16, 22, '.');       /* its way to the hub */
    fill(17, 17, 27, 27, '.');       /* the hub */
    fill(22, 12, 22, 16, '.'); fill(17, 5, 27, 11, '.');      /* path A, north, and the bone's room */
    fill(28, 22, 31, 22, '.'); m.grid[idx(32, 22)] = 'B'; fill(33, 17, 39, 27, '.');   /* path B, east, its barred door, the exit's room */
    fill(22, 28, 22, 32, '.'); fill(17, 33, 27, 39, '.');     /* path C, south, a room of gold */
    m.grid[idx(6, 22)] = 'S'; m.grid[idx(40, 22)] = 'X'; m.grid[idx(12, 22)] = 'D';
    m.rooms.push({ x: 5, y: 17, w: 7, h: 11, rt: 'RT_OSSUARY', vault: -1 }, { x: 17, y: 17, w: 11, h: 11, rt: 'RT_CHAPTER', vault: -1 },
        { x: 17, y: 5, w: 11, h: 7, rt: 'RT_BURIAL', vault: -1 }, { x: 33, y: 17, w: 7, h: 11, rt: 'RT_KNIGHTS', vault: -1 },
        { x: 17, y: 33, w: 11, h: 7, rt: 'RT_UNDER', vault: -1 });
    let id = 1; const T = o => { const t = Object.assign({ id: id++ }, o); m.things.push(t); return t; };
    const compass = T({ t: 'compass', x: 22.5, y: 22.5 });
    const bone = T({ t: 'relic', name: 'A bone key', x: 22.5, y: 7.5 });
    const socket = T({ t: 'socket', name: 'The empty reliquary', accepts: bone.id, x: 18.625, y: 18.625 });
    T({ t: 'gold', k: 'IT_CHALICE', x: 18.375, y: 6.375 }); T({ t: 'gold', k: 'IT_GEMS', x: 26.625, y: 10.625 });
    T({ t: 'gold', k: 'IT_CHEST', x: 22.5, y: 37.5 }); T({ t: 'gold', k: 'IT_COINS', x: 18.375, y: 34.375 }); T({ t: 'gold', k: 'IT_CROWN', x: 38.375, y: 26.375 });
    T({ t: 'spawn', k: 'MOB_SKEL', chance: 100, x: 25.5, y: 6.5 }); T({ t: 'spawn', k: '', chance: 75, x: 26.5, y: 37.5 });
    T({ t: 'spawn', k: 'MOB_SKEL', chance: 100, rank: 'II', wake: 'the reliquary\'s guard', x: 36.5, y: 18.5 });
    m.torches = [
        { x: 21, y: 13, d: 0, group: 'path A', lit: false }, { x: 23, y: 15, d: 2, group: 'path A', lit: false },
        { x: 29, y: 21, d: 1, group: 'path B', lit: false }, { x: 31, y: 23, d: 3, group: 'path B', lit: false },
        { x: 21, y: 30, d: 0, group: 'path C', lit: false }, { x: 23, y: 32, d: 2, group: 'path C', lit: false },
        { x: 16, y: 21, d: 1, group: 'the hub', lit: true }, { x: 16, y: 23, d: 3, group: 'the hub', lit: true },
    ];
    m.events = [
        { id: 'e1', name: 'The compass taken', kind: 'pickup', target: compass.id, needs: [],
          note: 'Every light flickers when the compass is picked up, and the torches down one path (north) catch: the way it wants you to go.',
          actions: [{ kind: 'flicker_all' }, { kind: 'sound', sound: 'SFX_DEEPCHIME' }, { kind: 'flicker_on', group: 'path A', after: 0.8 }] },
        { id: 'e2', name: 'The bone set', kind: 'place', target: socket.id, needs: ['e1'],
          note: 'They come back with the bone and set it; the north path gutters out and the east path flickers to life, its bar lifting.',
          actions: [{ kind: 'flicker_off', group: 'path A' }, { kind: 'flicker_on', group: 'path B', after: 1.0 }, { kind: 'door_open', door: [32, 22], after: 1.5 },
                    { kind: 'wake', group: 'the reliquary\'s guard', after: 1.5 }, { kind: 'line', text: 'SOMETHING STIRS TO THE EAST', after: 1.0 }] },
        { id: 'e3', name: 'The south room found', kind: 'enter', area: { x0: 17, y0: 33, x1: 27, y1: 39 }, needs: [],
          note: 'The south room lights itself when it is found: a reward off the path, not the way on.',
          actions: [{ kind: 'light_on', group: 'path C' }] },
    ];
    for (let y = 1; y < N - 1; y++) for (let x = 1; x < N - 1; x++)   /* lanes through the wide rooms, so nothing is out of the dream's reach */
        if (m.grid[idx(x, y)] === '.' && m.rooms.some(r => x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h) && (x % 4 === 0 || y % 4 === 0)) m.lane[idx(x, y)] = 1;
    m.next = id; m.slot = 3;
    return m;
}

const API = { ROOM_RUNS, ROOM_ANY, fieldLive, spotTags, pickOptions, fields, fieldByName, FIELD_DEF, canWatch, TRIG_TYPES, UNDO, roomSpot, picks, layouts, dreamAll, TRIGGERS, ACTIONS, SOUNDS, SOUND_NAMES, torchGroups, wakeGroups, torchLit, sampleEvents, N, DIRS, DOORS, WALLS, LANE_OF, BASE_OF, RUNS, RANKS, ELEM_FAV, GOLD_KINDS, KEY_NAMES, MARK_NAMES, MAX, idx, inMap, isOpen,
              blank, clone, xformRows, stampRows, stampDef, compose, laneAt, lanes, importDrawn, exportDrawn, bounds, frontOf, dream,
              levelInfo, rankCost, budget, elemDefault, check, label, thingCell };
if (typeof module !== 'undefined' && module.exports) module.exports = API; else root.MapCore = API;
})(typeof window !== 'undefined' ? window : globalThis);
