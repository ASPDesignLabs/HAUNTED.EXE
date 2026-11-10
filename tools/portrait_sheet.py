"""The portraits' numbers (docs/PLAN-portraits.md) as the owner settled them in the Sheet on 10 November (its revision 68): every slayer achievement,
read from KINDS in src/scene_crawl.c, the Spectre's and the Wraith's own, a feat for each kind, and the bosses', with the gold
each pays once a life. Writes docs/portraits/ranks.csv and bosses.csv, a snapshot of the Sheet to build from; the Sheet on
claude.ai is where they are changed, and this is brought in step with it by hand.

    python3 tools/portrait_sheet.py [--grid | --md]     (--grid: the plan's table, a kind a row; --md: a row an unlock)
"""
import csv, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = open(os.path.join(ROOT, 'src/scene_crawl.c')).read()
blk = src[src.index('static const Kind KINDS[] = {'):]
blk = blk[:blk.index('\n};')]
ROW = re.compile(r'\[(MOB_\w+)\] = \{ "([^"]*)", "([^"]*)", "[^"]*",\s*\w+, \w+, \w+, \w+, (-?\w+), (\d+), (RK_\w+), (\d), ([^,]+),')
xpb = src[src.index('static const int KIND_XP[] = {'):]
XP = {k: int(v) for k, v in re.findall(r'\[(MOB_\w+)\] = (\d+)', xpb[:xpb.index('};')])}
RK = ['RK_I', 'RK_II', 'RK_III', 'RK_IV', 'RK_V', 'RK_GILDED']
ROMAN = ['I', 'II', 'III', 'IV', 'V', 'G', 'Feat']
WORD = ['', 'ROTTEN ', 'COLD ', 'BLOODY ', 'BLACK ', 'GILDED ']
STEP = {'I': 0, 'II': 1, 'III': 2, 'IV': 3, 'V': 4, 'G': 3.5, 'Feat': 2}   # gilded just below black; a feat pays as rank III
NEED = [20, 25, 20, 15, 10]                              # the owner's (Q15), the game's 60, 40, 25, 15, 8 till built
GILD = 5                                                 # the gilded (3 in the game till built)
START = [100, 200, 300]                                  # the owner's (the plan first had 250, 500, 750)
SHARE = 0.6                                              # each rank step adds 60% of the start; strength up to 60% more
ROUND = 10

# the suggested rows the owner took: the two that cannot be slain, and a feat for each kind (name, needed, condition)
SPECTRE = [('I', 5, 'LAID TO REST'), ('II', 15, 'A CHILL LIFTED'), ('III', 40, 'THE HALLS GONE QUIET')]
WRAITH = [('I', 10, 'DRIVEN BACK', 'Drive it back 10 times'), ('II', 40, 'KEPT AT BAY', 'Drive it back 40 times'),
          ('III', 100, 'IT LEARNS FEAR', 'Drive it back 100 times'),
          ('IV', 1, 'THE WRAITH UNBOUND', 'The Wraith Unbound slain (its boss achievement)')]
FEAT = {'SKELETON': ('IT NEVER THREW', 20, 'Slay 20 before their first throw'), 'BAT': ('SHAKEN OFF', 20, 'Shake off 20 latched bats'),
        'WALL SKULL': ('BEFORE THE SCREAM', 20, 'Silence 20 before they scream'),
        'SHIELD-BEARER': ('THROUGH THE SHIELD', 20, 'Slay 20 after breaking the shield'),
        'SPIDER': ('STILL IN ITS WEB', 20, 'Slay 20 before they drop'), 'SHAMBLER': ('BACK IN THE GROUND', 20, 'Slay 20 before they stand'),
        'DROWNED': ('NOT A DROP', 10, 'Slay 10 without their burst reaching you'),
        'BONE HOUND': ('BOTH OF THEM', 10, 'Slay 10 pairs within five seconds'), 'HUNTING SPIDER': ('OFF THE CEILING', 20, 'Slay 20 overhead'),
        'GHOUL': ('AT ITS MEAL', 20, 'Slay 20 while feeding'), 'SKELETON KNIGHT': ('IN FULL PLATE', 10, 'Slay 10 before they shed any plate'),
        'BROOD MOTHER': ('NONE HATCHED', 5, 'Slay 5 before a sac hatches'), 'SPIDERLING': ('THE WHOLE BROOD', 200, 'Slay 200'),
        'SHADE': ('FACED DOWN', 10, 'Slay 10 while they fade from your light'), 'EMBER IMP': ('QUENCHED', 25, 'Slay 25 by Frost'),
        'DEACON': ('NONE RISE', 10, 'Slay 10 before any rises for him'), 'GARGOYLE': ('STILL STONE', 10, 'Slay 10 within two seconds of waking'),
        'WHIPPER-IN': ('NO HORN', 5, 'Slay 5 before his horn'), 'SEXTON': ('UNRUNG', 10, 'Slay 10 with his bell never rung'),
        'LIGHTLESS': ('IN THE DARK', 10, 'Slay 10 with the lantern shuttered')}
BOSSES = [('THE ILL-FATED KNIGHT', 1500), ('THE WANDERING SPIDER', 2500), ('THE BLACK DEACON', 3500), ('THE WRAITH UNBOUND', 4500)]

kinds = []
for m, name, plural, fam, debut, top, gild, flags in ROW.findall(blk):
    kinds.append(dict(mob=m, name=name, plural=plural, family=fam, debut=int(debut), top=RK.index(top), gild=int(gild),
                      flags=flags, xp=XP.get(m, 0)))
order = []                                               # the bestiary's order: by first level, then by number
for lv in range(1, 41):
    for k in kinds:
        if k['debut'] == lv and 'KF_PAGE' in k['flags']: order.append(k)
for i, k in enumerate(order): k['third'] = 0 if i < 7 else (1 if i < 15 else 2)   # 7, 8, 7: the spiderling with its mother

def slayer_need(k, r):                                   # as the game's will read once built (Q15)
    if 'KF_SPIRIT' in k['flags'] or 'KF_UNCOUNTED' in k['flags']: return 0
    if k['family'] == '-1': return NEED[0] if r == 0 else 0  # off the ladder: one rank, rank I's count
    if r == 5: return GILD if k['gild'] else 0
    low = 1 if (k['debut'] - 1) // 10 % 4 == 3 else 0      # the Deep has no rank I
    return 0 if r < low or r > k['top'] else NEED[r - low]

for t in range(3):                                       # strength: the kind's worth (KIND_XP) within its third, 0 to 1
    ks = [k for k in order if k['third'] == t and any(slayer_need(k, r) for r in range(6))]
    lo, hi = min(k['xp'] for k in ks), max(k['xp'] for k in ks)
    for k in ks: k['str'] = round((k['xp'] - lo) / (hi - lo), 2) if hi > lo else 0.0
for k in order:
    if k['name'] == 'SPECTRE': k['str'] = 0.0
    if k['name'] == 'WRAITH': k['str'] = 1.0

def gold(third, rank, strength):                         # half up, as the Sheet's ROUND
    return int(START[third] * (1 + SHARE * (STEP[rank] + strength)) / ROUND + 0.5 + 1e-9) * ROUND
# the owner's Q9 and Q14: a kind of one rank pays as its third's rank V; the Wraith's fourth gem (the Wraith Unbound's fall) lights
# but pays nothing, the boss's own portrait paying for it
PAYS_AS = {('SHADE', 'I'): 'V'}
GEM_ONLY = {('WRAITH', 'IV')}

rows = []
def add(k, rank, need, title, cond, source, note=''):
    pays = (k['name'], rank) not in GEM_ONLY
    rows.append(dict(kind=k['name'].title(), rank=rank, need=need, gold=gold(k['third'], PAYS_AS.get((k['name'], rank), rank), k['str']),
                     title=title, cond=cond, source=source, third=k['third'] + 1, str=k['str'], xp=k['xp'], note=note, pays=pays))
for k in order:
    nm = k['name']
    if nm == 'SPECTRE':
        for r, n, t in SPECTRE: add(k, r, n, t, 'Lay %d to rest (only Spirit reaches them)' % n, 'Suggested')
        continue
    if nm == 'WRAITH':
        for r, n, t, c in WRAITH: add(k, r, n, t, c, 'Suggested')
        continue
    for r in range(6):
        n = slayer_need(k, r)
        if not n: continue
        word = 'PALE ' if k['family'] == 'PF_CHITIN' and r == 4 else WORD[r]
        cond = 'Slay %d gilded' % n if r == 5 else ('Slay %d' % n if k['family'] == '-1' else 'Slay %d at rank %s' % (n, ROMAN[r]))
        add(k, ROMAN[r], n, word + k['plural'], cond, 'In the game')
    f = FEAT[nm]
    add(k, 'Feat', f[1], f[0], f[2], 'Suggested')

if '--grid' in sys.argv:
    print('| # | Kind | Third | Strength | I | II | III | IV | V | Gilded | Feat | All |')
    print('|---|---|---|---|---|---|---|---|---|---|---|---|')
    for i, k in enumerate(order):
        cells, tot = [], 0
        for r in ROMAN:
            x = [y for y in rows if y['kind'] == k['name'].title() and y['rank'] == r]
            if x and not x[0]['pays']: cells.append('%d: gem only' % x[0]['need'])
            elif x: cells.append('%d: %s' % (x[0]['need'], format(x[0]['gold'], ','))); tot += x[0]['gold']
            else: cells.append('-')
        print('| %d | %s | %d | %.2f | %s | %s |' % (i + 1, k['name'].title(), k['third'] + 1, k['str'], ' | '.join(cells), format(tot, ',')))
    tot = [sum(x['gold'] for x in rows if x['third'] == t and x['pays']) for t in (1, 2, 3)]
    b = sum(g * (1 + 0.2 * s) for _, g in BOSSES for s in range(6))
    print('\nrows %d; by third %s; all %s; bosses %s' % (len(rows), [format(v, ',') for v in tot], format(sum(tot), ','), format(int(b), ',')))
    sys.exit(0)
if '--md' in sys.argv:
    for x in rows: print('| %s | %s | %d | %s | %s | %s |' % (x['kind'], x['rank'], x['need'], format(x['gold'], ','), x['title'], x['cond']))
    sys.exit(0)
with open(os.path.join(ROOT, 'docs/portraits/ranks.csv'), 'w', newline='') as f:
    w = csv.writer(f)
    w.writerow(['Kind', 'Rank', 'Slain', 'Gold', 'Achievement', 'Condition', 'Pays', 'Source', 'Third', 'Strength', 'Kind XP'])
    for x in rows: w.writerow([x['kind'], x['rank'], x['need'], x['gold'], x['title'], x['cond'], 'TRUE' if x['pays'] else 'FALSE', x['source'], x['third'], x['str'], x['xp']])
with open(os.path.join(ROOT, 'docs/portraits/bosses.csv'), 'w', newline='') as f:
    w = csv.writer(f)
    w.writerow(['Achievement', 'Deed', 'Gold', 'Pays', 'Condition'])
    for name, g in BOSSES:
        w.writerow([name, 'Slain', g, 'TRUE', 'Slain on the way down'])
        for s in range(1, 6): w.writerow([name, 'Slain at step %d' % s, int(g * (1 + 0.2 * s)), 'TRUE', 'At the Accursed Portal, challenge step %d' % s])
print('docs/portraits/ranks.csv: %d rows; bosses.csv: %d' % (len(rows), 24))
