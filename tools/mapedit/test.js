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
/* every room type the game has is in one group of the editor's list, by run or any run's */
{
    const grouped = C.ROOM_RUNS.flat().concat(C.ROOM_ANY);
    DATA.rooms.forEach(r => { if (grouped.filter(g => g === r).length !== 1) fail(`room type ${r} is in ${grouped.filter(g => g === r).length} of the list's groups`); });
    grouped.forEach(g => { if (!DATA.rooms.includes(g)) fail(`the list names ${g}, which the game does not have`); });
    console.log(`rooms by run: ${C.ROOM_RUNS.map(r => r.length).join(' + ')} + ${C.ROOM_ANY.length} of any run = ${DATA.rooms.length} types`);
}
console.log(bad ? `${bad} FAILED` : 'all passed');
process.exit(bad ? 1 : 0);
