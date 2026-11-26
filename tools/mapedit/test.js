/* The map editor's round trip (docs/PLAN-mapedit.md): each of drawn.c's levels read in and written out must come back as it was,
 * letter for letter and thing for thing; then its checks and the dream's walk. node tools/mapedit/test.js */
'use strict';
const fs = require('fs'), path = require('path');
const C = require('./core.js');
const DATA = JSON.parse(fs.readFileSync(path.join(__dirname, 'data.json'), 'utf8'));
let bad = 0;
const fail = msg => { bad++; console.log('FAIL ' + msg); };
for (const D of DATA.drawn) {
    const m = C.importDrawn(D, D.n), e = C.exportDrawn(DATA, m, D.n);
    if (e.b.w !== D.w || e.b.h !== D.h || e.b.x0 !== D.ox || e.b.y0 !== D.oy) fail(`level ${D.n}: box ${e.b.w}x${e.b.h} at (${e.b.x0},${e.b.y0}), was ${D.w}x${D.h} at (${D.ox},${D.oy})`);
    e.rows.forEach((r, y) => { if (r !== D.rows[y]) fail(`level ${D.n} row ${y}: "${r}" was "${D.rows[y]}"`); });
    const want = D.things.map(t => `{ DT_${t.what}, ${t.kind}, ${t.x.toFixed(2)}f, ${t.y.toFixed(2)}f }`);
    if (want.length !== e.things.length) fail(`level ${D.n}: ${e.things.length} things, was ${want.length}`);
    want.forEach((w, i) => { if (w !== e.things[i]) fail(`level ${D.n} thing ${i}: ${e.things[i]} was ${w}`); });
    if (e.vault.join() !== D.vault.join()) fail(`level ${D.n}: vaults ${e.vault} were ${D.vault}`);
    if (e.seed !== D.seed) fail(`level ${D.n}: seed ${e.seed} was ${D.seed}`);
    const rooms = D.rooms.map(r => `    { ${r.x}, ${r.y}, ${r.w}, ${r.h}, ${r.rt} },`);
    rooms.forEach(r => { if (!e.text.includes(r)) fail(`level ${D.n}: room line missing: ${r}`); });
    const row = `{ DRAWN${D.n}_MAP, ${D.w}, ${D.h}, ${D.ox}, ${D.oy}, ${D.face}, DRAWN${D.n}_ROOM`;
    if (!e.text.includes(row)) fail(`level ${D.n}: DRAWN row differs`);
    if (!e.text.includes(`${D.seed}u, ${D.level}, NULL, 0 },`)) fail(`level ${D.n}: the row's level and spawn points differ`);
    const ch = C.check(DATA, m), dr = C.dream(DATA, m), b = C.budget(DATA, m);
    console.log(`level ${D.n}: round trip ${bad ? 'with failures' : 'exact'}; ${ch.filter(c => c.sev === 'error').length} errors, ${ch.filter(c => c.sev === 'warn').length} warnings, ${ch.filter(c => c.sev === 'info').length} notes`);
    ch.forEach(c => console.log(`  ${c.sev.padEnd(5)} ${c.msg}${c.cells.length ? ' ' + JSON.stringify(c.cells.slice(0, 4)) : ''}`));
    console.log(`  dream: ${dr.ok ? 'finishes' : 'does NOT finish'} in ${dr.steps} steps; missed ${dr.missed.length}: ${dr.missed.map(t => C.label(DATA, t)).join(', ')}`);
    console.log(`  budget ${b.budget.toFixed(1)} (${b.L.runName}, step ${b.L.step + 1}); the dead ${b.expect.toFixed(2)}, ${b.headsMost} heads; gold ${b.gold.open} in ${b.gold.n}, secret ${b.gold.secret} in ${b.gold.ns}`);
    const ds = C.dream(DATA, m, { secrets: true });
    console.log(`  dream with the secret pressed: missed ${ds.missed.length}`);
    if (!dr.ok) fail(`level ${D.n}: the dream does not finish`);
}
/* the owner's example of events (docs/PLAN-events.md): it finishes, every trigger fires in order; without the door's action it cannot */
{
    const m = C.sampleEvents(), ch = C.check(DATA, m), d = C.dream(DATA, m);
    const errs = ch.filter(c => c.sev === 'error'), warns = ch.filter(c => c.sev === 'warn');
    console.log(`events sample: ${errs.length} errors, ${warns.length} warnings; dream ${d.ok ? 'finishes' : 'does NOT finish'} in ${d.steps} steps, ${d.unfired.length} triggers unfired`);
    warns.concat(errs).forEach(c => console.log('  ' + c.sev + ' ' + c.msg));
    if (errs.length || warns.length) fail('the events sample has errors or warnings');
    if (!d.ok || d.unfired.length || d.missed.length) fail('the events sample should finish with every trigger fired and nothing missed');
    const order = d.events.filter(e => e.trig).map(e => e.trig).join();
    if (order !== 'e1,e3,e2') fail(`the triggers fired in the order ${order}`);
    const cut = C.sampleEvents(); cut.events[1].actions = cut.events[1].actions.filter(a => a.kind !== 'door_open');
    const dc = C.dream(DATA, cut);
    if (dc.ok) fail('without the door opening, the sample should not finish');
    if (!C.check(DATA, cut).some(c => c.msg.includes('barred door no event opens'))) fail('the check should say the barred door never opens');
    const wrong = C.sampleEvents(); wrong.things.find(t => t.t === 'socket').accepts = 999;
    if (!C.check(DATA, wrong).some(c => c.sev === 'error' && c.msg.includes('no longer on the map'))) fail('a socket bound to a missing relic should be an error');
    if (C.dream(DATA, wrong).ok) fail('with the socket bound to nothing there, the sample should not finish');
    const x = C.exportDrawn(DATA, m, 3).text;
    ['DT_COMPASS', 'DT_RELIC, 0', 'DT_SOCKET, 0', 'DRAWN3_TORCH', 'DRAWN3_TRIG', 'DRAWN3_ACT', 'DRAWN3_RELIC', 'TG_PICKUP', 'TG_PLACE', 'TG_ENTER', 'TA_FLICKER_ON', 'TA_DOOR_OPEN']
        .forEach(k => { if (!x.includes(k)) fail('the export lacks ' + k); });
}
/* trigger types and rooms (docs/PLAN-events.md): a lever on a timed trigger holds the barred door; too short and the door shuts
 * before the dream gets there, long enough and it finishes; the bone in one of two rooms, every layout tried */
{
    const timed = hold => {
        const m = C.sampleEvents();
        m.events[1].actions = m.events[1].actions.filter(a => a.kind !== 'door_open');
        m.things.push({ id: 900, t: 'lever', x: 26.5, y: 26.5 });
        m.events.push({ id: 'e9', name: 'The lever', kind: 'use', target: 900, type: 'timed', hold, needs: ['e2'], actions: [{ kind: 'door_open', door: [32, 22] }] });
        return m;
    };
    const short = C.dreamAll(DATA, timed(1)), long = C.dreamAll(DATA, timed(30));
    console.log(`timed lever: 1 s ${short.ok ? 'finishes' : 'cannot finish'} (${short.shut || 0} shut on the way), 30 s ${long.ok ? 'finishes' : 'cannot finish'} in ${Math.round(long.seconds)} s`);
    if (short.ok || !short.shut) fail('a 1 s door 10 cells away should shut before the dream gets there, and say so');
    if (!long.ok) fail('a 30 s door should be reached');
    if (C.check(DATA, timed(0)).every(c => !c.msg.includes('timed but has no duration'))) fail('a timed trigger with no duration should be an error');
    const tog = timed(30); tog.events[3].type = 'toggle'; tog.events[3].hold = 4;
    const dt = C.dream(DATA, tog);
    if (!dt.events.some(e => /goes back/.test(e.msg))) fail('a switch with a duration should go back by itself');
    const pick = C.sampleEvents(); pick.things.find(t => t.t === 'relic').inRooms = [2, 4];
    const L = C.layouts(pick), dp = C.dreamAll(DATA, pick);
    console.log(`the bone in one of two rooms: ${L.length} layouts, ${dp.failed} fail`);
    if (L.length !== 2 || dp.failed) fail('both rooms for the bone should finish');
    const at4 = C.dream(DATA, pick, { layout: { [pick.things.find(t => t.t === 'relic').id]: 1 } }).pos.get(pick.things.find(t => t.t === 'relic').id);
    if (!(at4[1] > 33 && at4[1] < 40)) fail('in room 5 the bone should lie in the south room, not ' + at4);
    if (!C.exportDrawn(DATA, pick, 3).text.includes('DRAWN3_PICK')) fail('the export lacks the picks');
}
/* spots and fields: a thing at one of three tagged spots, every one tried; a fire field set, a trigger that puts it out; the whole
 * level written out as the fixture the game's side builds against (tools/mapedit/fixtures/events_full.c) */
{
    const m = C.sampleEvents();
    m.name = 'Every part of the events (a test level, not for play)';
    m.things.push({ id: 801, t: 'spot', tag: 'the compass\'s places', x: 20.5, y: 20.5 }, { id: 802, t: 'spot', tag: 'the compass\'s places', x: 24.5, y: 24.5 },
                  { id: 803, t: 'spot', tag: 'the compass\'s places', x: 8.5, y: 20.5 });
    m.things.find(t => t.t === 'compass').spotTag = 'the compass\'s places';
    m.things.push({ id: 804, t: 'plate', x: 22.5, y: 14.5 }, { id: 805, t: 'lever', x: 26.5, y: 26.5 });
    m.things.push({ id: 806, t: 'spawn', k: '', chance: 100, spotTag: 'the compass\'s places', x: 0.5, y: 0.5 });
    for (let y = 33; y <= 35; y++) for (let x = 23; x <= 25; x++) m.grid[C.idx(x, y)] = 'F';
    m.fields = [{ x: 23, y: 33, name: 'The south embers', on: true, dmg: 10, chance: 50, kills: false }];
    m.events.push({ id: 'e4', name: 'The plate', kind: 'step', target: 804, type: 'timed', hold: 6, needs: [], actions: [{ kind: 'light_on', group: 'path C' }], back: [{ kind: 'sound', sound: 'SFX_KNOCK' }] });
    m.events.push({ id: 'e5', name: 'The lever', kind: 'use', target: 805, type: 'toggle', hold: 0, needs: [], actions: [{ kind: 'field_off', field: 'The south embers' }] });
    const ch = C.check(DATA, m), errs = ch.filter(c => c.sev === 'error');
    const opts = C.pickOptions(m, C.compose(DATA, m).cells, C.lanes(m, C.compose(DATA, m).cells), m.things.find(t => t.t === 'compass'));
    const d = C.dreamAll(DATA, m), f = C.fields(m, C.compose(DATA, m).cells);
    console.log(`spots and fields: ${errs.length} errors; the compass has ${opts.length} places; ${d.layouts} layouts, ${d.failed} fail; ${f.length} field: ${f.map(x => x.name + ' ' + x.dmg + '%').join(', ')}`);
    errs.forEach(c => console.log('  error ' + c.msg));
    if (errs.length) fail('the full test level has errors');
    if (opts.length !== 3 || d.failed) fail('the compass should have three places, all finishing');
    if (f.length !== 1 || f[0].name !== 'The south embers' || f[0].dmg !== 10) fail('the field should keep its settings');
    const x = C.exportDrawn(DATA, m, 3).text;
    ['DT_SPOT, 0', 'DT_PLATE', 'DT_LEVER', 'DRAWN3_FIELD', 'DF_FIRE', 'TA_FIELD_OFF', 'TG_STEP', 'TG_USE', 'TT_TIMED', 'TT_TOGGLE', 'DRAWN3_PICK'].forEach(k => { if (!x.includes(k)) fail('the full export lacks ' + k); });
    fs.mkdirSync(path.join(__dirname, 'fixtures'), { recursive: true });
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'events_full.c'), x);
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'events_full.json'), JSON.stringify(m));
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'events_sample.c'), C.exportDrawn(DATA, C.sampleEvents(), 3).text);
}
/* a stamp laid and turned: the shapes stay whole */
const s = DATA.stamps[0];
for (let t = 0; t < 8; t++) {
    const r = C.xformRows(s.rows, t), n = r.join('').replace(/#/g, '').length;
    if (n !== s.rows.join('').replace(/#/g, '').length) fail(`stamp turn ${t} lost cells`);
}
/* hazards (docs/PLAN-events.md, 2c): three of the Crypts' on the sample (level 3), one of each way of going off; their checks clear,
 * the export's arrays and row, and a trigger that wakes one; written out as the fixture the game's side builds against */
{
    const m = C.sampleEvents(), cells = C.compose(DATA, m).cells, open = [], walls = [];
    for (let y = 1; y < C.N - 1; y++) for (let x = 1; x < C.N - 1; x++) {
        const c = cells[C.idx(x, y)];
        if (c === '.' || c === '+') open.push([x, y]);
        else if (c === '#' && C.DIRS.some(([dx, dy]) => '.+'.includes(cells[C.idx(x + dx, y + dy)]))) walls.push([x, y]);
    }
    const w = walls[5], f = C.DIRS.map(([dx, dy]) => [w[0] + dx, w[1] + dy]).find(([x, y]) => '.+'.includes(cells[C.idx(x, y)]));
    m.hazards = [
        { kind: 'HZ_ROCK', name: 'The roof', ax: open[3][0], ay: open[3][1], cells: [open[3], open[4], open[5]], zone: true, clock: true, every: 6, give: 1, first: 2, harm: 20 },
        { kind: 'HZ_BREATH', name: 'The niche', ax: w[0], ay: w[1], cells: [f], clock: false, enter: true, box: { x0: f[0] - 1, y0: f[1] - 1, x1: f[0] + 1, y1: f[1] + 1 }, warn: 1.5, chance: 60 },
        { kind: 'HZ_VOTIVE', name: 'The candles', ax: open[10][0], ay: open[10][1], cells: [open[10], open[11], open[12]], clock: false, on: false, kills: true },
    ];
    m.events[0].actions.push({ kind: 'haz_on', haz: 'The candles', after: 1 });
    const ch = C.check(DATA, m), errs = ch.filter(c => c.sev === 'error'), hzw = ch.filter(c => c.sev !== 'info' && /hazard|The roof|The niche|The candles/.test(c.msg));
    errs.concat(hzw).forEach(c => console.log('  ' + c.sev + ' ' + c.msg));
    if (errs.length || hzw.length) fail('the hazards sample has errors or warnings about its hazards');
    const x = C.exportDrawn(DATA, m, 3).text;
    ['DRAWN3_HAZ[]', 'DRAWN3_HZCELL[]', 'HZ_ROCK', 'HZ_BREATH', 'HH_ENTER', 'TA_HAZ_ON, 2,', 'DRAWN3_HAZ, 3, DRAWN3_HZCELL, 7'].forEach(k => { if (!x.includes(k)) fail('the hazards export lacks ' + k); });
    const gone = JSON.parse(JSON.stringify(m)); gone.hazards.pop();
    if (!C.check(DATA, gone).some(c => c.sev === 'error' && /names no hazard/.test(c.msg))) fail('a trigger naming a hazard that is gone should be an error');
    const stone = JSON.parse(JSON.stringify(m)); stone.hazards[0].cells.push(w);
    if (!C.check(DATA, stone).some(c => c.sev === 'error' && /of stone/.test(c.msg))) fail('a hazard over stone should be an error');
    console.log(`hazards: ${errs.length} errors; the export ${x.length} bytes with ${m.hazards.length} hazards; the Crypts offer ${Object.values(C.hazardsOf(DATA, 3)).filter(Array.isArray).flat().join(' ')}`);
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'hazards.c'), x);
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'hazards.json'), JSON.stringify(m));
}
/* every kind a run offers has the editor's words for it */
DATA.hazards.fixed.flat().concat(DATA.hazards.arise.flat(), DATA.hazards.sapper).forEach(k => { if (!C.HAZ_INFO[k]) fail(`no words for ${k}`); });
/* the rolls: five layouts of creatures and of gold over the sample level, each on free floor, the creatures within the budget and
 * away from the start, the same again for the same seed */
{
    const m = C.sampleEvents(), cells = C.compose(DATA, m).cells, s0 = cells.indexOf('S'), sx = s0 % C.N, sy = (s0 / C.N) | 0;
    const R = C.rolls(DATA, m, 'dead', 12345), G = C.rolls(DATA, m, 'gold', 12345);
    R.forEach((L, k) => {
        if (L.spent > L.left + 1e-9) fail(`roll ${k}: the dead cost ${L.spent} of ${L.left}`);
        L.items.forEach(t => { const x = Math.floor(t.x), y = Math.floor(t.y); if (!'.+'.includes(cells[C.idx(x, y)])) fail(`roll ${k}: a creature on '${cells[C.idx(x, y)]}'`);
            });
    });
    G.forEach((L, k) => L.items.forEach(t => { const c = cells[C.idx(Math.floor(t.x), Math.floor(t.y))]; if (!'.+'.includes(c)) fail(`gold roll ${k} on '${c}'`); }));
    if (JSON.stringify(C.rolls(DATA, m, 'dead', 12345)) !== JSON.stringify(R)) fail('the same seed should roll the same');
    if (new Set(R.map(L => JSON.stringify(L.items))).size < 4) fail('five rolls should mostly differ');
    console.log(`rolls: creatures ${R.map(L => L.items.length + ' (' + L.spent.toFixed(2) + '/' + L.left.toFixed(2) + ')').join(', ')}; gold ${G.map(L => L.items.length + ' worth ' + L.value).join(', ')}`);
}
/* lights (docs/PLAN-lights.md): their array and the row's two values, the hazards' places written empty before them when there are none */
{
    const m = C.sampleEvents(), cells = C.compose(DATA, m).cells, open = [];
    for (let i = 0; i < C.N * C.N; i++) if (cells[i] === '.') open.push([i % C.N, (i / C.N) | 0]);
    m.lights = [{ style: 'DL_CHANDELIER', x: open[2][0], y: open[2][1] }, { style: 'DL_BRAZIER', x: open[9][0], y: open[9][1] }, { style: 'DL_CORONA', x: open[20][0], y: open[20][1] }];
    const x = C.exportDrawn(DATA, m, 3).text, errs = C.check(DATA, m).filter(c => c.sev === 'error');
    ['DRAWN3_LIGHT[]', '{ DL_CHANDELIER,', 'NULL, 0, NULL, 0, DRAWN3_LIGHT, 3 }'].forEach(k => { if (!x.includes(k)) fail('the lights export lacks ' + k); });
    if (errs.length) fail('the lights sample has errors: ' + errs.map(c => c.msg).join('; '));
    const bad = JSON.parse(JSON.stringify(m)); bad.lights.push({ style: 'DL_LANTERN', x: 0, y: 0 });
    if (!C.check(DATA, bad).some(c => c.sev === 'error' && /not over open floor/.test(c.msg))) fail('a light in stone should be an error');
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'lights.c'), x);
    console.log(`lights: ${m.lights.length} in the export, ${errs.length} errors`);
}
/* the editor's light styles are the game's, every one */
{
    const ed = C.LIGHT_STYLES.map(s => s[0]);
    if (ed.length !== DATA.lights.length || DATA.lights.some(k => !ed.includes(k))) fail(`the light styles differ: the game's ${DATA.lights.join(' ')}`);
}
/* every room type the game has is in one group of the editor's list, by run or any run's */
{
    const grouped = C.ROOM_RUNS.flat().concat(C.ROOM_ANY);
    DATA.rooms.forEach(r => { if (grouped.filter(g => g === r).length !== 1) fail(`room type ${r} is in ${grouped.filter(g => g === r).length} of the list's groups`); });
    grouped.forEach(g => { if (!DATA.rooms.includes(g)) fail(`the list names ${g}, which the game does not have`); });
    console.log(`rooms by run: ${C.ROOM_RUNS.map(r => r.length).join(' + ')} + ${C.ROOM_ANY.length} of any run = ${DATA.rooms.length} types`);
}
/* side passages (docs/PLAN-side.md): each built in read and written back letter for letter; a new one, its export read back; the
 * crypt's side doorway; the reader refusing what the game would. The files under fixtures/ are for the harness's HOST_SIDE. */
{
    const SC = require('./side.js'), ids = DATA.kinds.map(k => k.id), tn = (DATA.tune || []).map(k => k.name);
    if (tn.length < 30) fail(`the tuning table read ${tn.length} names`);
    for (const s of DATA.sides) {
        const r = SC.parse(s.text, ids, tn);
        if (r.error) { fail(`side passage ${s.n}: line ${r.line}: ${r.error}`); continue; }
        const out = SC.exportText(r.map);
        if (out !== s.text) { const a = s.text.split('\n'), b = out.split('\n'), i = a.findIndex((l, k) => l !== b[k]); fail(`side passage ${s.n}: line ${i + 1} "${b[i]}" was "${a[i]}"`); }
        const errs = SC.check(r.map).filter(c => c.sev === 'error');
        if (errs.length) fail(`side passage ${s.n}: ${errs[0].msg}`);
        fs.writeFileSync(path.join(__dirname, 'fixtures', `side_${s.n}.txt`), out);
    }
    const m = SC.blank(40, 14);
    SC.add(m, { k: 'plat', x: 10, y: 8, w: 2, dx: 0, dy: -1, dist: 4.5, speed: 1.25, loop: true, wait: 0.75, at: 0.5 });
    SC.add(m, { k: 'flow', x: 20, y: 9, w: 3, h: 2, on: 2.5, off: 6, at: 1 });
    SC.add(m, { k: 'fall', x: 15, y: 8, delay: 0.3, back: 2 }); SC.add(m, { k: 'fall', x: 16, y: 8, delay: SC.FALL_DELAY, back: SC.FALL_BACK });
    SC.add(m, { k: 'mob', x: 25, y: 10, kind: 'MOB_SKEL', rank: 'RK_II' }); SC.add(m, { k: 'switch', x: 30, y: 10, thing: 4, toggle: true });
    SC.add(m, { k: 'gold', x: 12, y: 10, it: 'IT_CROWN' }); SC.add(m, { k: 'key', x: 13, y: 10, c: 2 });
    SC.add(m, { k: 'gate', x: 18, y: 9, w: 1, h: 2, open: false, tag: 'g1' }); SC.add(m, { k: 'plate', x: 6, y: 10, tag: 'p1' });
    SC.add(m, { k: 'current', x: 20, y: 5, w: 4, h: 1, dx: 1, dy: 0, speed: 3, stopped: true, poison: true, tag: 'w1' }); m.things.find(t => t.k === 'key').loose = true;
    SC.add(m, { k: 'haz', kind: 'HZ_FIRE', x: 30, y: 10, cells: [[30, 10], [31, 10]], zone: true, safe: true, clock: { every: 4, give: 1, first: 2 }, enter: { x0: 28, y0: 9, x1: 31, y1: 10 }, harm: 20, chance: -1, warn: 1.5, stays: 6, tag: 't1' });
    m.trigs = [{ src: 'p1', mode: 'HOLD', acts: [{ tag: 'w1', verb: 'ON' }, { tag: 'g1', verb: 'FLIP' }] }, { src: 'p1', mode: 'TIMED', secs: 2.5, acts: [{ crypt: 7 }] }]; m.tune = { walk: 80, jump: 250 };
    const t1 = SC.exportText(m), back = SC.parse(t1, ids, tn);
    if (back.error) fail(`a new passage's export: line ${back.line}: ${back.error}`);
    else if (SC.exportText(back.map) !== t1) fail("a new passage's export does not read back the same");
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'side_new.txt'), t1);
    [['SIDE 2\n', 'version'], ['SIDE 1\nSIZE 10 10\n', 'SIZE wants'], ['SIDE 1\nSIZE 20 10\nROW "#"\n', 'a ROW is'],
     ['SIDE 1\nSIZE 20 10\n' + 'ROW "....................."\n', 'a ROW is'], ['SIDE 1\nSIZE 20 10\n' + 'ROW "...................."\n'.repeat(10), 'no doorway'],
     ['SIDE 1\nSIZE 20 10\n' + 'ROW "...................."\n'.repeat(10) + 'PLAT 1 1 3 0 0 4 2\n', "way is"]].forEach(([t, want]) => {
        const r = SC.parse(t, ids); if (!r.error || !r.error.includes(want)) fail(`the reader took a bad passage (wanted "${want}"): ${r.error || 'no refusal'}`); });
    const cm = C.blank(3); cm.things.push({ id: 1, t: 'sidedoor', num: 2, link: 5, x: 10.5, y: 10.5 });
    for (let x = 9; x <= 13; x++) cm.grid[C.idx(x, 11)] = '+'; cm.grid[C.idx(9, 11)] = 'S'; cm.grid[C.idx(13, 10)] = 'X';   /* a hall, its start and exit */
    const cx = C.exportDrawn(DATA, cm, 3).text;
    if (!cx.includes('{ DT_SIDEDOOR, 37, ')) fail("the crypt's side doorway did not export as DT_SIDEDOOR 37");
    const sd = cx.match(/\{ DT_SIDEDOOR, \d+, (-?[\d.]+)f, (-?[\d.]+)f \}/);
    if (!sd || +sd[1] < 0 || +sd[2] < 0) fail(`a side doorway above the floor's box exported outside the map: ${sd && sd[0]}`);
    fs.writeFileSync(path.join(__dirname, 'fixtures', 'side_crypt.c'), cx);
    if (C.check(DATA, cm).some(c => c.msg.includes('side doorway'))) fail("a side doorway in a wall by the floor was refused");
    cm.things[0].x = 10.5; cm.things[0].y = 11.5;
    if (!C.check(DATA, cm).some(c => c.msg.includes('not in stone'))) fail("a side doorway on the floor was not refused");
    console.log(`side passages: ${DATA.sides.length} built in read and written back; a new one's export read back; the side doorway`);
}
/* 26 November: timed switches, ledges and falling blocks off at the start, powerups: the fixture read and written back; the reader
 * refusing what the game would; a ledge off with nothing to lay it warned of */
{
    const SC = require('./side.js'), ids = DATA.kinds.map(k => k.id);
    const txt = fs.readFileSync(path.join(__dirname, 'fixtures', 'side_works2.txt'), 'utf8'), r = SC.parse(txt, ids);
    if (r.error) fail(`side_works2: line ${r.line}: ${r.error}`);
    else {
        if (SC.exportText(r.map) !== txt) fail('side_works2 does not read back letter for letter');
        const cs = SC.check(r.map); if (cs.length) fail('side_works2: ' + cs[0].msg);
        const sw = r.map.things.find(t => t.k === 'switch'), lg = r.map.things.find(t => t.k === 'ledge'), fl = r.map.things.find(t => t.k === 'fall');
        if (!sw || sw.timer !== 8) fail("side_works2: the lever's timer was not read");
        if (!lg || lg.w !== 6 || !lg.stopped || !fl || !fl.stopped) fail('side_works2: the ledge or the block was not read as off');
        if (r.map.things.filter(t => t.k === 'power').map(t => t.kind + t.secs).join() !== 'RAPID10,MIGHT12,WARD8') fail('side_works2: the powerups were not read');
        const lone = Object.assign({}, r.map, { trigs: [] });
        if (!SC.check(lone).some(c => /never there/.test(c.msg))) fail('a ledge off with no trigger should be warned of');
    }
    const head = 'SIDE 1\nNAME "X"\nSIZE 20 10\n' + Array.from({ length: 10 }, (_, y) => `ROW "${(y === 0 || y === 9 ? '#' : '#' + ','.repeat(18) + '#').padEnd(20, '#')}"`).join('\n') + '\nDOOR 1 8 0\n';
    [['POWER 4 8 HASTE', /RAPID, MIGHT or WARD/], ['POWER 4 8 WARD 200', /120 seconds/], ['LEDGE 15 4 9', /off the map/], ['SWITCH 4 8 -1 TIMER 500', /0 to 120/]].forEach(([l, re]) => {
        const q = SC.parse(head + l + '\n', ids); if (!q.error || !re.test(q.error)) fail(`"${l}" should be refused: ${q.error || 'it was read'}`);
    });
    console.log('side passages: timed switches, ledges and blocks off at the start, powerups');
}
/* run I's traps in a passage (docs/PLAN-side.md, 2e): the fixture read and written back; every emitter option; the censer's clearance;
 * the PREVIEW's run: wax settling, the bowl swinging, the haze kept to its reach */
{
    const SC = require('./side.js'), ids = DATA.kinds.map(k => k.id);
    const txt = fs.readFileSync(path.join(__dirname, 'fixtures', 'side_traps1.txt'), 'utf8'), r = SC.parse(txt, ids);
    if (r.error) fail(`side_traps1: line ${r.line}: ${r.error}`);
    else {
        if (SC.exportText(r.map) !== txt) fail('side_traps1 does not read back letter for letter');
        const errs = SC.check(r.map).filter(c => c.sev === 'error'); if (errs.length) fail('side_traps1: ' + errs[0].msg);
        const m = r.map, hz = k => m.things.find(t => t.k === 'haz' && t.kind === k);
        const v = hz('HZ_VOTIVE'); Object.assign(v, { emit: { rate: 40, speed: 1.5, spread: 10, life: 5 }, dir: 45 });
        const q = hz('HZ_RELIQ'); q.radial = 2; const c = hz('HZ_CENSER'); Object.assign(c, { len: 6, hang: 0.5, swing: 1.25 });
        const t2 = SC.exportText(m), back = SC.parse(t2, ids);
        ['EMIT 40 1.5 10 5.0 DIR 45', 'RADIAL 2', 'LEN 6 HANG 0.5 SWING 1.25'].forEach(k => { if (!t2.includes(k)) fail('the emitter export lacks ' + k); });
        if (back.error || SC.exportText(back.map) !== t2) fail('the emitters do not read back the same');
        const short = JSON.parse(JSON.stringify(c)); short.len = 2;
        if (!SC.censerCheck(m, short).some(([s, msg]) => s === 'error' && /three at the least/.test(msg))) fail('a censer on two tiles of chain should be refused');
        const deep = JSON.parse(JSON.stringify(c)); deep.len = 9;
        if (!SC.censerCheck(m, deep).some(([s, msg]) => s === 'error' && /strikes stone/.test(msg))) fail('a censer swinging into the floor should be refused');
        const lone = JSON.parse(JSON.stringify(v)); lone.cells = [[v.x, v.y + 2]];
        if (!SC.check(Object.assign({}, m, { things: [lone] })).some(c => /which way to tip/.test(c.msg))) fail('a sconce with nothing painted aside should be refused');
        const run = (t, s) => { const S = SC.fxSim(m, t, 5); for (let i = 0; i < s * 120; i++) S.step(1 / 120); return S; };
        const sv = run(hz('HZ_VOTIVE'), 5), sc = run(c, 1.1), sq = run(q, 4);
        if (!sv.parts.some(p => p.rest)) fail('the preview: no wax settled');
        if (!sc.ball || Math.abs(sc.ball[0] - SC.censerGeom(c).px) > SC.censerGeom(c).r) fail('the preview: the censer is not on its circle');
        const far = sq.parts.filter(p => Math.hypot(p.x - (q.x * 16 + 8), p.y - ((q.y + 1) * 16 - 10)) > (2 + 0.5) * 16 + 8);
        if (!sq.parts.length || far.length) fail(`the preview: the haze ${sq.parts.length} with ${far.length} past its reach`);
        console.log(`run I's traps: read back; emitters written and read; the censer's clearance refused short and into stone; preview wax ${sv.parts.filter(p => p.rest).length} settled, haze ${sq.parts.length}`);
    }
}
/* run II's (2e): the fixture read and written back; each one's PREVIEW makes its own; a web or a pool with no cells refused */
{
    const SC = require('./side.js'), ids = DATA.kinds.map(k => k.id);
    const txt = fs.readFileSync(path.join(__dirname, 'fixtures', 'side_traps2.txt'), 'utf8'), r = SC.parse(txt, ids);
    if (r.error) fail(`side_traps2: line ${r.line}: ${r.error}`);
    else {
        if (SC.exportText(r.map) !== txt) fail('side_traps2 does not read back letter for letter');
        const errs = SC.check(r.map).filter(c => c.sev === 'error'); if (errs.length) fail('side_traps2: ' + errs[0].msg);
        const want = { HZ_BILE: 'bile', HZ_SLUICE: 'water', HZ_GALV: 'spark', HZ_SILT: 'gas', HZ_SAINT: 'tear' }, got = [];
        r.map.things.filter(t => t.k === 'haz' && want[t.kind]).forEach(t => {
            const S = SC.fxSim(r.map, t, 4); let n = 0;
            for (let i = 0; i < 4 * 120; i++) { S.step(1 / 120); n = Math.max(n, S.parts.filter(p => p.kind === want[t.kind]).length); }
            got.push(`${want[t.kind]} ${n}`);
            if (!n) fail(`the preview of ${t.kind} made no ${want[t.kind]}`);
        });
        ['HZ_SNARE', 'HZ_GALV', 'HZ_SILT'].forEach(k => { const t = JSON.parse(JSON.stringify(r.map.things.find(o => o.kind === k))); t.cells = [];
            if (!SC.check(Object.assign({}, r.map, { things: [t] })).some(c => c.sev === 'error' && /no cells/.test(c.msg))) fail(`${k} with no cells should be refused`); });
        console.log(`run II's traps: read back; previews ${got.join(', ')}; a web, a pool and a mud with no cells refused`);
    }
}
console.log(bad ? `${bad} FAILED` : 'all passed');
process.exit(bad ? 1 : 0);
