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
/* a stamp laid and turned: the shapes stay whole */
const s = DATA.stamps[0];
for (let t = 0; t < 8; t++) {
    const r = C.xformRows(s.rows, t), n = r.join('').replace(/#/g, '').length;
    if (n !== s.rows.join('').replace(/#/g, '').length) fail(`stamp turn ${t} lost cells`);
}
console.log(bad ? `${bad} FAILED` : 'all passed');
process.exit(bad ? 1 : 0);
