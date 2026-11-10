"""The portraits' sheet (docs/PLAN-portraits.md): every slayer achievement, read from KINDS in src/scene_crawl.c, with the gold
each rank's portrait would pay by the plan's formula. Writes docs/portraits/achievements.xlsx (live formulas: change the
starting gold, the share a rank adds or a kind's strength and the gold follows) and prints the plan's table as Markdown.

    python3 tools/portrait_sheet.py [--md | --grid]     (--md: the rows; --grid: a kind a row)
"""
import re, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = open(os.path.join(ROOT, 'src/scene_crawl.c')).read()
blk = src[src.index('static const Kind KINDS[] = {'):]
blk = blk[:blk.index('\n};')]
ROW = re.compile(r'\[(MOB_\w+)\] = \{ "([^"]*)", "([^"]*)", "[^"]*",\s*\w+, \w+, \w+, \w+, (-?\w+), (\d+), (RK_\w+), (\d), ([^,]+),')
xpb = src[src.index('static const int KIND_XP[] = {'):]
XP = {k: int(v) for k, v in re.findall(r'\[(MOB_\w+)\] = (\d+)', xpb[:xpb.index('};')])}
RK = ['RK_I', 'RK_II', 'RK_III', 'RK_IV', 'RK_V', 'RK_GILDED']
ROMAN = ['I', 'II', 'III', 'IV', 'V', 'G']
WORD = ['', 'ROTTEN ', 'COLD ', 'BLOODY ', 'BLACK ', 'GILDED ']
STEP = [0, 1, 2, 3, 4, 3.5]                              # gilded: just below black, as its frame is
NEED = [60, 40, 25, 15, 8]
START = [250, 500, 750]
SHARE = 0.6                                              # each rank adds 60% of the start; strength up to 60% more

kinds = []
for m, name, plural, fam, debut, top, gild, flags in ROW.findall(blk):
    kinds.append(dict(mob=m, name=name, plural=plural, family=fam, debut=int(debut), top=RK.index(top), gild=int(gild),
                      flags=flags, xp=XP.get(m, 0)))
order = []                                               # the bestiary's order: by first level, then by number
for lv in range(1, 41):
    for k in kinds:
        if k['debut'] == lv and 'KF_PAGE' in k['flags']: order.append(k)
# thirds of the 22 pages: the spiderling goes with its mother (7, 8, 7)
for i, k in enumerate(order): k['third'] = 0 if i < 7 else (1 if i < 15 else 2)

def slayer_need(k, r):
    if 'KF_SPIRIT' in k['flags'] or 'KF_UNCOUNTED' in k['flags']: return 0
    if k['family'] == '-1': return 60 if r == 0 else 0
    if r == 5: return 3 if k['gild'] else 0
    low = 1 if (k['debut'] - 1) // 10 % 4 == 3 else 0      # the Deep has no rank I
    return 0 if r < low or r > k['top'] else NEED[r - low]

for t in range(3):                                       # strength: the kind's worth (KIND_XP) within its third, 0 to 1
    ks = [k for k in order if k['third'] == t and any(slayer_need(k, r) for r in range(6))]
    lo, hi = min(k['xp'] for k in ks), max(k['xp'] for k in ks)
    for k in ks: k['str'] = round((k['xp'] - lo) / (hi - lo), 2) if hi > lo else 0.0

def gold(k, r):
    return int(START[k["third"]] * (1 + SHARE * (STEP[r] + k["str"])) / 10.0 + 0.5 + 1e-9) * 10   # half up, as MROUND

rows = []
for i, k in enumerate(order):
    for r in range(6):
        n = slayer_need(k, r)
        if not n: continue
        word = 'PALE ' if k['family'] == 'PF_CHITIN' and r == 4 else WORD[r]
        rows.append(dict(order=i + 1, kind=k['name'], third=k['third'] + 1, rank=ROMAN[r], step=STEP[r], title=word + k['plural'],
                         need=n, xp=k['xp'], str=k['str'], gold=gold(k, r)))

if '--grid' in sys.argv:                                 # a kind a row: each rank's kills and gold
    print('| # | Kind | Third | Strength | I | II | III | IV | V | Gilded | All |')
    print('|---|---|---|---|---|---|---|---|---|---|---|')
    for i, k in enumerate(order):
        cells, tot = [], 0
        for r in (0, 1, 2, 3, 4, 5):
            x = [y for y in rows if y['kind'] == k['name'] and y['rank'] == ROMAN[r]]
            if x: cells.append('%d: %s' % (x[0]['need'], format(x[0]['gold'], ','))); tot += x[0]['gold']
            else: cells.append('-')
        print('| %d | %s | %d | %s | %s | %s |' % (i + 1, k['name'].title(), k['third'] + 1, '%.2f' % k['str'] if 'str' in k else '-', ' | '.join(cells), format(tot, ',') if tot else '-'))
    sys.exit(0)
if '--md' in sys.argv:
    print('| # | Kind | Third | Rank | Achievement | Slain | Strength | Gold |')
    print('|---|---|---|---|---|---|---|---|')
    for x in rows:
        print('| %d | %s | %d | %s | %s | %d | %.2f | %s |' % (x['order'], x['kind'].title(), x['third'], x['rank'], x['title'], x['need'], x['str'], format(x['gold'], ',')))
    tot = [sum(x['gold'] for x in rows if x['third'] == t) for t in (1, 2, 3)]
    print('\nrows %d; by third %s; all %s' % (len(rows), [format(v, ',') for v in tot], format(sum(tot), ',')))
    for k in order:
        g = [x['gold'] for x in rows if x['kind'] == k['name']]
        print('%s: %d ranks, %s' % (k['name'], len(g), format(sum(g), ',')))
    sys.exit(0)

from openpyxl import Workbook
from openpyxl.styles import Font, PatternFill, Alignment
wb = Workbook()
P = wb.active; P.title = 'Formula'
P['A1'] = 'THE PORTRAITS OF THE SLAIN: WHAT EACH RANK PAYS, ONCE A LIFE'; P['A1'].font = Font(bold=True, size=13)
P['A2'] = 'Gold = start of its third x (1 + share x (rank step + strength)), rounded to the nearest 10. Change the blue cells.'
P['A4'] = 'Start, first third (Skeleton to Spider)'; P['B4'] = START[0]
P['A5'] = 'Start, second third (Drowned to Spiderling)'; P['B5'] = START[1]
P['A6'] = 'Start, third third (Shade to Lightless)'; P['B6'] = START[2]
P['A7'] = 'Share of the start each rank step adds'; P['B7'] = SHARE
P['A8'] = 'Strength counts (1 yes, 0 no)'; P['B8'] = 1
P['A9'] = 'Round to'; P['B9'] = 10
P['A11'] = 'Rank steps: I 0, II 1, III 2, IV 3, V 4, Gilded 3.5 (just below black). With strength on, the weakest kind\'s rank I pays the start and the strongest\'s rank V four times it.'
for r in range(4, 10):
    P.cell(r, 2).fill = PatternFill('solid', fgColor='DDEBF7'); P.cell(r, 2).font = Font(color='1F4E79', bold=True)
P.column_dimensions['A'].width = 46; P.column_dimensions['B'].width = 10
S = wb.create_sheet('Slayers')
head = ['#', 'Kind', 'Third', 'Rank', 'Step', 'Achievement', 'Slain', 'Kind XP', 'Strength', 'Gold']
S.append(head)
for c in range(1, len(head) + 1): S.cell(1, c).font = Font(bold=True)
for i, x in enumerate(rows, start=2):
    S.append([x['order'], x['kind'].title(), x['third'], x['rank'], x['step'], x['title'], x['need'], x['xp'], x['str']])
    S.cell(i, 10).value = ('=MROUND(CHOOSE(C{r},Formula!$B$4,Formula!$B$5,Formula!$B$6)*(1+Formula!$B$7*(E{r}+Formula!$B$8*I{r})),Formula!$B$9)').format(r=i)
    S.cell(i, 9).fill = PatternFill('solid', fgColor='DDEBF7'); S.cell(i, 7).fill = PatternFill('solid', fgColor='DDEBF7')
n = len(rows) + 1
S.cell(n + 2, 6).value = 'All of them, once a life'; S.cell(n + 2, 6).font = Font(bold=True)
S.cell(n + 2, 10).value = '=SUM(J2:J%d)' % n
for t in (1, 2, 3):
    S.cell(n + 2 + t, 6).value = 'Third %d' % t
    S.cell(n + 2 + t, 10).value = '=SUMIF(C2:C%d,%d,J2:J%d)' % (n, t, n)
for col, w in zip('ABCDEFGHIJ', (4, 18, 6, 6, 6, 30, 7, 8, 9, 9)): S.column_dimensions[col].width = w
S.freeze_panes = 'A2'
# the bosses' and the ways' deeds that are kills, and the suggestions, for the owner to weigh
B = wb.create_sheet('Bosses and deeds')
B.append(['Achievement', 'Kind of deed', 'Now', 'Suggested gold (a boss portrait, two faces wide)'])
for c in range(1, 5): B.cell(1, c).font = Font(bold=True)
BOSSES = [('THE ILL-FATED KNIGHT', 1500), ('THE WANDERING SPIDER', 2500), ('THE BLACK DEACON', 3500), ('THE WRAITH UNBOUND', 4500)]
for name, g in BOSSES:
    B.append([name, 'slain', 'an achievement', g])
    for st in range(1, 6): B.append([name, 'slain at challenge step %d' % st, 'an achievement', int(g * (1 + 0.2 * st))])
for name, need in (('CUT OFF THE HEAD', 1), ('SHEPHERD OF NONE', 10), ('THE LEADERLESS DEAD', 50), ('BEFORE THE CRY', 1), ('A HOUSE OF SILENCE', 10)):
    B.append([name, 'a pack\'s leader slain first' if 'HEAD' in name or 'SHEPHERD' in name or 'LEADERLESS' in name else 'a caller slain before it calls', 'an achievement (%d)' % need, 'none: not a portrait\'s'])
for col, w in zip('ABCD', (26, 30, 18, 44)): B.column_dimensions[col].width = w
out = os.path.join(ROOT, 'docs/portraits/achievements.xlsx')
wb.save(out)
print(out, len(rows), 'rows')
