"""CATACOMB's palette as a PNG, a ramp to a row: python3 palette_png.py in.csv out.png
Rows 1-15 are the ramps (indices 16r+1 .. 16r+16); the last row is black (0), white (241), gold (242-254) and the mice's eye (255)."""
import csv, sys
from PIL import Image, ImageDraw
rows = list(csv.DictReader(open(sys.argv[1])))
NAMES = ["STONE", "WARM", "BONE", "BRICK", "MOSS", "WOOD", "IRON", "SKIN", "GHOST", "PURPLE", "BLOOD", "FLOOR", "CEIL", "FLAME", "GLOW", "BLACK/WHITE/GOLD"]
S, LW, TH = 36, 130, 24
im = Image.new("RGB", (LW + 16 * S + 8, TH + 16 * S + 8), (16, 16, 20))
d = ImageDraw.Draw(im)
dim = (170, 170, 180)
for c in range(16): d.text((LW + c * S + 14, 6), "%d" % c, fill=dim)
def swatch(r, c, rec):
    x, y = LW + c * S, TH + r * S
    col = (int(rec["r8"]), int(rec["g8"]), int(rec["b8"]))
    d.rectangle([x, y, x + S - 2, y + S - 2], fill=col)
    lum = 0.3 * col[0] + 0.59 * col[1] + 0.11 * col[2]
    d.text((x + 4, y + 11), rec["index"], fill=(0, 0, 0) if lum > 110 else (255, 255, 255))
for r in range(16):
    d.text((6, TH + r * S + 11), NAMES[r], fill=dim)
    idx = [16 * r + 1 + c for c in range(16)] if r < 15 else [0] + list(range(241, 256))
    for c, i in enumerate(idx): swatch(r, c, rows[i])
im.save(sys.argv[2])
