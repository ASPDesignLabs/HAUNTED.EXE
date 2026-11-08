#!/usr/bin/env python3
"""eyesheet.py <harness> <workdir> <out-prefix>: the eyes of creatures born to an element (docs/PLAN-combat.md 7.4, B10),
from the game's own renderer (CRAWL_STAGE, CRAWL_ELEM, CRAWL_EYEFLARE in the debug build), for the owner. Page 1: every kind
that may be born to one, at rank III, as today and in each element. Page 2: a skeleton at ranks II-V, as today, born, and
flaring as its attack carries the element. Page 3: the motes in motion. Needs Pillow, as make cards does:
  make build/host_debug && python3 tools/eyesheet.py build/host_debug build/eyes build/eyes/elemeyes"""
import os, subprocess, sys
from PIL import Image, ImageChops, ImageDraw

HOST, WORK, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(WORK, exist_ok=True)
KINDS = [(0, 'SKELETON'), (2, 'BAT'), (4, 'SHIELD-BEARER'), (5, 'SPIDER'), (6, 'SHAMBLER'), (7, 'DROWNED'), (8, 'BONE HOUND'),
         (10, 'HUNTING SPIDER'), (11, 'GHOUL'), (12, 'SKELETON KNIGHT'), (13, 'BROOD MOTHER'), (14, 'SPIDERLING'),
         (19, 'GARGOYLE'), (20, 'WHIPPER-IN')]
ELEMS = ['FIRE', 'FROST', 'LIGHTNING', 'POISON', 'SPIRIT']
RANKS = ['I', 'II', 'III', 'IV', 'V']
T = 18.0
SCALE = 3


def frame(kind, rank, elem, flare=False, dist=1.7, t=None):
    """a 320x200 frame: kind at rank (0-based), born to elem (0 none, 1-5), flaring or not, at time t"""
    t = T if t is None else t
    name = f'k{kind}_r{rank}_e{elem}_f{int(flare)}_d{dist}_t{t:.2f}'
    path = os.path.join(WORK, name)
    png = f'{path}_{t:06.2f}.png'
    if not os.path.exists(png):
        env = dict(os.environ, CRAWL_SEED='3', CRAWL_STAGE=str(kind), CRAWL_STAGEDIST=str(dist), CRAWL_RANK=str(rank))
        env.pop('CRAWL_ELEM', None); env.pop('CRAWL_EYEFLARE', None)
        if elem: env['CRAWL_ELEM'] = str(elem)
        if flare: env['CRAWL_EYEFLARE'] = '1'
        subprocess.run([HOST, 'shot', 'crawl', f'{t}', path, '1'], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    return Image.open(png).convert('RGB')


def head_box(kind, rank, w=60, h=40):
    """where the eyes are: the box round what changes when the kind is born to an element"""
    a, bb = frame(kind, rank, 0), None
    for e in (2, 1, 5, 3, 4):                                      # whichever element it may be born to
        bb = ImageChops.difference(a, frame(kind, rank, e)).getbbox()
        if bb: break
    cx, cy = ((bb[0] + bb[2]) // 2, (bb[1] + bb[3]) // 2) if bb else (160, 60)
    x0 = max(0, min(320 - w, cx - w // 2)); y0 = max(0, min(200 - h, cy - h // 2 + 4))
    return (x0, y0, x0 + w, y0 + h)


def cell(img, box):
    return img.crop(box).resize(((box[2] - box[0]) * SCALE, (box[3] - box[1]) * SCALE), Image.NEAREST)


def sheet(rows, cols, title, note, name):
    """rows: [(label, [image per column])]"""
    cw, ch = rows[0][1][0].size
    lw, gap, top = 170, 6, 64
    W = lw + len(cols) * (cw + gap) + gap
    H = top + len(rows) * (ch + gap) + 40
    im = Image.new('RGB', (W, H), (12, 12, 14))
    d = ImageDraw.Draw(im)
    d.text((10, 8), title, fill=(230, 210, 140))
    d.text((10, 24), note, fill=(170, 170, 170))
    for c, t in enumerate(cols):
        d.text((lw + c * (cw + gap) + gap + 4, top - 16), t, fill=(220, 220, 220))
    for r, (label, cells) in enumerate(rows):
        y = top + r * (ch + gap)
        d.text((10, y + ch // 2 - 6), label, fill=(220, 220, 220))
        for c, img in enumerate(cells):
            im.paste(img, (lw + c * (cw + gap) + gap, y))
    im.save(name)
    print(name, im.size)


WEAK = {0: 'PHYSICAL', 2: 'POISON', 4: 'LIGHTNING', 5: 'FIRE', 6: 'FIRE', 7: 'LIGHTNING', 8: 'FROST', 10: 'FROST', 11: 'SPIRIT',
        12: 'LIGHTNING', 13: 'POISON', 14: 'POISON', 19: 'PHYSICAL', 20: 'SPIRIT'}


def never(img):
    d = ImageDraw.Draw(img)
    d.rectangle([0, img.size[1] - 14, img.size[0], img.size[1]], fill=(40, 10, 10))
    d.text((4, img.size[1] - 13), 'ITS WEAKNESS: NEVER', fill=(230, 140, 140))
    return img


rows = []
for k, nm in KINDS:
    box = head_box(k, 2)
    rows.append((nm, [cell(frame(k, 2, e), box) if not e or WEAK[k] != ELEMS[e - 1] else never(cell(frame(k, 2, e), box)) for e in range(6)]))
sheet(rows, ['AS TODAY (III)'] + ELEMS, 'BORN TO AN ELEMENT: THE EYES, AT RANK III (docs/PLAN-combat.md 7.4, B10)',
      'The element takes the colour, the rank keeps its form (III: the slit). Each born one flickers and lets a mote rise now and then.',
      OUT + '-kinds.png')

rows = []
box = None
for r in range(1, 5):
    box = head_box(0, r)
    cells = [cell(frame(0, r, 0), box)] + [cell(frame(0, r, e), box) for e in range(1, 6)] + [cell(frame(0, r, e, True), box) for e in range(1, 6)]
    rows.append(('SKELETON, RANK ' + RANKS[r], cells))
sheet(rows, ['AS TODAY'] + ELEMS + [e + ', STRIKING' for e in ELEMS],
      'A SKELETON BORN TO EACH ELEMENT, RANK BY RANK, CALM AND STRIKING WITH IT',
      'Rank I is never born to one. The ranks keep their forms (II a glow beside, III a slit, IV a streak, V a flare). Striking with it, they flare in its colour (today: white).',
      OUT + '-ranks.png')

rows = []
box = head_box(0, 2)
for e in range(1, 6):
    rows.append((ELEMS[e - 1], [cell(frame(0, 2, e, t=T + 0.12 * j), box) for j in range(6)]))
sheet(rows, [f'+{0.12 * j:.2f} S' for j in range(6)], 'IN MOTION: A RANK III SKELETON, SIX FRAMES 0.12 S APART',
      'Each element stirs its own way: fire rises, frost falls white, lightning sparks at the side, poison drips, spirit sways up; the eyes flicker.',
      OUT + '-motion.png')
