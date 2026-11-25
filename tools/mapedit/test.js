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
/* every room type the game has is in one group of the editor's list, by run or any run's */
{
    const grouped = C.ROOM_RUNS.flat().concat(C.ROOM_ANY);
    DATA.rooms.forEach(r => { if (grouped.filter(g => g === r).length !== 1) fail(`room type ${r} is in ${grouped.filter(g => g === r).length} of the list's groups`); });
    grouped.forEach(g => { if (!DATA.rooms.includes(g)) fail(`the list names ${g}, which the game does not have`); });
    console.log(`rooms by run: ${C.ROOM_RUNS.map(r => r.length).join(' + ')} + ${C.ROOM_ANY.length} of any run = ${DATA.rooms.length} types`);
}
console.log(bad ? `${bad} FAILED` : 'all passed');
process.exit(bad ? 1 : 0);
