#!/usr/bin/env python3
"""Read one of the owner's level drawings (docs/PLAN-tutorial.md) cell by cell.

    python3 -I tools/tutorial_read.py <drawing.png> [x0 y0 pitch cols rows]

Without the numbers it finds the grid from the thick black walls: their lines fall on the
drawing's grid, a cell apart. It prints the cells (# stone, . floor, F the burning floor), every
coloured or black line on an edge between two open cells (doors, key doors, triggers, thin
walls), the lines on the edge of stone that aren't plain wall (the fountain, the exit), and the
dots (E the dead, G gold, Y the gold key's letter, C the crimson's, M magenta, g an
 elemental mark: a square an interaction, a circle a switch), in cells: a whole number is a grid line, so a dot
at 6.0, 10.0 stands on the corner of four cells. What it reads is a first reading, to be checked
against the drawing by eye: media/sheets/tutorial/level1-read.png was made the same way.
Needs Pillow."""
import sys
from collections import deque
from PIL import Image

def colour(p):
    r, g, b = p
    if r < 35 and g < 35 and b < 35: return 'black'
    if r > 230 and 140 < g < 190 and b < 40: return 'fire'
    if r < 60 and g > 210 and b > 230: return 'ice'
    if r < 10 and g < 130 and b > 150: return 'ice edge'
    if r > 130 and 60 < g < 110 and b < 80: return 'door'
    if r > 200 and g > 200 and b < 80: return 'gold door'
    if r > 220 and g < 60 and b < 40: return 'crimson door'
    if r > 220 and g < 60 and b > 200: return 'magenta'
    if r < 80 and g > 180 and b < 100: return 'fountain'
    if r < 90 and 130 < g < 205 and b > 200: return 'exit'
    if 180 < r < 210 and 180 < g < 210 and 180 < b < 210: return 'elemental'
    if r > 200 and g < 70: return 'red'
    if 110 < r < 150 and 100 < g < 140 and b > 150: return 'trigger 2'
    if r < 60 and 110 < g < 150 and 70 < b < 110: return 'trigger 3'
    return None

def floor_like(p):                                   # tan (level 1) or peach (level 2)
    r, g, b = p
    return r > 160 and g > 130 and b > 100 and r > b + 25 and not (r > 230 and g < 190 and b < 60)

def find_grid(px, w, h):
    """the thick black lines' middles, and the pitch between them"""
    def runs(counts):
        out, start = [], None
        for i, n in enumerate(counts + [0]):
            if n and start is None: start = i
            if not n and start is not None: out.append((start + i - 1) / 2); start = None
        return out
    black = lambda p: p[0] < 25 and p[1] < 25 and p[2] < 25
    cols = runs([1 if sum(black(px[x, y]) for y in range(h)) > 100 else 0 for x in range(w)])
    rows = runs([1 if sum(black(px[x, y]) for x in range(w)) > 100 else 0 for y in range(h)])
    gaps = sorted(b - a for s in (cols, rows) for a, b in zip(s, s[1:]) if b - a > 20)
    small = [g for g in gaps if g < gaps[0] * 1.4]  # a cell apart; the rest are whole multiples
    p0 = small[len(small) // 2]
    x0, y0 = cols[0], rows[0]
    ncols = round((cols[-1] - x0) / p0); nrows = round((rows[-1] - y0) / p0)
    pitch = ((cols[-1] - x0) / ncols + (rows[-1] - y0) / nrows) / 2
    return x0, y0, pitch, ncols, nrows

def main():
    im = Image.open(sys.argv[1]).convert('RGB'); px = im.load(); W, H = im.size
    if len(sys.argv) >= 7: x0, y0, P, NC, NR = float(sys.argv[2]), float(sys.argv[3]), float(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6])
    else: x0, y0, P, NC, NR = find_grid(px, W, H)
    print('grid: x0 %.1f y0 %.1f pitch %.2f, %d x %d cells' % (x0, y0, P, NC, NR))
    def cell_kind(c, r):
        tally = {}
        for fx in (0.2, 0.35, 0.65, 0.8):
            for fy in (0.2, 0.35, 0.65, 0.8):
                p = px[int(x0 + P * (c + fx)), int(y0 + P * (r + fy))]
                tally[p] = tally.get(p, 0) + 1
        r_, g_, b_ = max(tally, key=tally.get)
        if colour((r_, g_, b_)) == 'fire': return 'F'
        if colour((r_, g_, b_)) == 'ice': return 'I'
        if floor_like((r_, g_, b_)): return '.'
        return '#'
    grid = [[cell_kind(c, r) for c in range(NC)] for r in range(NR)]
    print('    ' + ''.join(str(c % 10) for c in range(NC)))
    for r in range(NR): print('%3d %s' % (r, ''.join(grid[r])))
    g = lambda c, r: grid[r][c] if 0 <= c < NC and 0 <= r < NR else '#'
    def edge(c, r, vert):
        t = {}
        for i in range(2, 19):
            f = i / 20
            x, y = (int(round(x0 + P * c)), int(y0 + P * (r + f))) if vert else (int(x0 + P * (c + f)), int(round(y0 + P * r)))
            best = None
            for d in (-2, -1, 0, 1, 2):
                k = colour(px[x + d, y] if vert else px[x, y + d])
                if k and k != 'black': best = k
                elif k == 'black' and best is None: best = 'black'
            if best: t[best] = t.get(best, 0) + 1
        return {k: v for k, v in t.items() if v >= 6}
    print('\nedges (V: the west edge of cell c,r; H: its north edge):')
    for vert in (True, False):
        for r in range(NR + (0 if vert else 1)):
            for c in range(NC + (1 if vert else 0)):
                a, b = (g(c - 1, r) if vert else g(c, r - 1)), g(c, r)
                if a == '#' and b == '#': continue
                t = edge(c, r, vert)
                for k in ('fire', 'ice', 'ice edge'): t.pop(k, None)   # inside the burning floor or the ice
                if a != '#' and b != '#' and t: print('  between open cells', 'V' if vert else 'H', c, r, t)
                elif (a == '#') != (b == '#') and set(t) - {'black', 'red'}: print('  on stone', 'V' if vert else 'H', c, r, t)
    def dot(p):
        r_, g_, b_ = p
        if r_ > 200 and g_ < 60 and 40 < b_ < 120: return 'E'
        if r_ > 220 and 100 < g_ < 160 and 130 < b_ < 190: return 'G'
        if r_ > 220 and g_ > 200 and b_ < 60: return 'Y'
        if 180 < r_ < 210 and 180 < g_ < 210 and 180 < b_ < 210: return 'g'   # an elemental switch or interaction
        if r_ > 220 and g_ < 60 and b_ > 200: return 'M'
        if r_ > 220 and g_ < 60 and b_ < 40: return 'C'                     # crimson: the crimson key's letter
        return None
    seen, blobs = set(), []
    for y in range(int(y0), min(H, int(y0 + P * NR))):
        for x in range(int(x0), min(W, int(x0 + P * NC))):
            k = dot(px[x, y])
            if not k or (x, y) in seen: continue
            st, pts = [(x, y)], []
            seen.add((x, y))
            while st:
                a, b = st.pop(); pts.append((a, b))
                for da, db in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    q = (a + da, b + db)
                    if q not in seen and 0 <= q[0] < W and 0 <= q[1] < H and dot(px[q]) == k: seen.add(q); st.append(q)
            if len(pts) < 60: continue
            cx = sum(p[0] for p in pts) / len(pts); cy = sum(p[1] for p in pts) / len(pts)
            blobs.append((k, (cx - x0) / P, (cy - y0) / P, len(pts)))
    print('\ndots (in cells; size in pixels, a large one may mean something):')
    for k, cx, cy, n in sorted(blobs, key=lambda b: (b[0], b[2], b[1])):
        if 0 <= cx < NC and 0 <= cy < NR and g(int(cx), int(cy)) != '#' or k == 'Y':
            print('  %s %5.2f %5.2f  n=%d' % (k, cx, cy, n))

if __name__ == '__main__':
    main()
