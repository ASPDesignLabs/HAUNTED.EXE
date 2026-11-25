"""The cage lantern's dappled light, as a sheet of frames for the owner (docs/PLAN-lights.md, section 4).

    python3 -I tools/dapple_sheet.py [out.png]       (default media/sheets/lights/cage_dapple.png; needs build/host_debug, Pillow)

The editor's fixture tools/mapedit/fixtures/lights.c is read (never changed) and two cage lanterns hung in its south room, one by
the west wall and one over the open floor; the export is laid as each run's (levels 3, 13, 23, 33) in build/dapple/. The harness
stands a person before each lantern and takes a frame with the spots (the game as it is) and without (CRAWL_LIGHTLOOK=0); three
left is held off (CRAWL_THREELEFT=-1) so the flames are the run's own. Each row is a run: the four frames at the game's own size,
then the lantern's corner of the two with spots enlarged three times beside the same without.
"""
import os
import subprocess
import sys
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'media/sheets/lights/cage_dapple.png')
WORK = os.path.join(ROOT, 'build/dapple')
RUNS = [(3, 'THE CRYPTS'), (13, 'THE CATACOMBS'), (23, 'THE HOLLOWS'), (33, 'THE DEEP')]
VIEWS = [('wall', '21.5,34.0,159', 'BY A WALL'), ('floor', '23.0,34.8,70', 'OVER THE FLOOR')]
CROP = {'wall': (110, 50, 216, 130), 'floor': (72, 80, 178, 160)}   # the lantern's corner in the 320x200 frame, 106x80: three times over, a column


def export(level):
    t = open(os.path.join(ROOT, 'tools/mapedit/fixtures/lights.c')).read()
    a = '{ DL_CHANDELIER, 14, 0 }, { DL_BRAZIER, 21, 0 }, { DL_CORONA, 21, 1 },'
    assert t.count(a) == 1 and t.count('DRAWN3_LIGHT, 3 }') == 1 and t.count('0x7D1A0003u, 3,') == 1
    t = t.replace(a, a + ' { DL_CAGE, 12, 30 }, { DL_CAGE, 19, 32 },').replace('DRAWN3_LIGHT, 3 }', 'DRAWN3_LIGHT, 5 }')
    t = t.replace('0x7D1A0003u, 3,', '0x7D1A0003u, %d,' % level)
    p = os.path.join(WORK, 'L%d.c' % level)
    open(p, 'w').write(t)
    return p


def frame(level, drawn, name, at, look):
    stem = os.path.join(WORK, 'L%d_%s_%d' % (level, name, look))
    env = dict(os.environ, HOST_DRAWN=drawn, CRAWL_SEED='1', CRAWL_RUNID='7', CRAWL_TITLE='0', CRAWL_STAGE='0', CRAWL_STAGEDIST='99',
               CRAWL_THREELEFT='-1', CRAWL_LIGHTLOOK=str(look), CRAWL_TPAT=at, HOST_INPUT='1:+H;4.3:-H')
    subprocess.run([os.path.join(ROOT, 'build/host_debug'), 'shot', 'crawl', '9', stem], env=env, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    im = Image.open(stem + '_009.00.png').convert('RGB')
    return im.resize((320, 200), Image.NEAREST) if im.size != (320, 200) else im   # the harness writes it three times over


def main():
    os.makedirs(WORK, exist_ok=True)
    pad, lab, head = 8, 130, 96
    rowh = 160 + 12 + 240 + 24
    W = pad + lab + 4 * (320 + pad) + pad
    H = head + len(RUNS) * (rowh + pad) + pad
    sheet = Image.new('RGB', (W, H), (16, 14, 20))
    d = ImageDraw.Draw(sheet)
    ink, gold, dim = (216, 208, 189), (233, 194, 76), (150, 140, 128)
    d.text((pad, 8), "THE CAGE LANTERN'S DAPPLED LIGHT, FOR THE OWNER'S LOOK (PLAN-LIGHTS, STEP 4)", fill=gold)
    for i, l in enumerate([
            "FRAMES FROM THE GAME (THE HARNESS), A DRAWN LEVEL WITH TWO CAGE LANTERNS, IN EACH RUN. THE LIGHT THROUGH THE CAGE'S HOLES",
            "FALLS IN SPOTS ON THE WALLS AND THE FLOOR ABOUT IT; THE CONE UNDER IT AND THE EAVE OVER IT SHADE WHAT IS STRAIGHT BELOW AND",
            "ABOVE. DRAWING ONLY: THE LIGHT THE DEAD SEE BY IS THE LANTERN'S PLAIN LIGHT, AS 'WITHOUT' SHOWS IT.",
            "EACH ROW: BY A WALL AND OVER THE FLOOR, WITHOUT AND WITH, AT THE GAME'S OWN SIZE; THEN THE LANTERN'S CORNER ENLARGED."]):
        d.text((pad, 28 + 12 * i), l, fill=ink)
    y = head
    for level, run in RUNS:
        drawn = export(level)
        d.text((pad, y + 70), run, fill=ink)
        d.text((pad, y + 84), 'LEVEL %d' % level, fill=dim)
        x = pad + lab
        crops = []
        for name, at, label in VIEWS:
            for look in (0, 1):
                im = frame(level, drawn, name, at, look)
                sheet.paste(im.crop((0, 0, 320, 160)), (x, y + 12))
                d.text((x, y), '%s, %s' % (label, 'WITH THE SPOTS' if look else 'WITHOUT'), fill=ink if look else dim)
                x += 320 + pad
                c = im.crop(CROP[name])
                crops.append(c.resize((c.width * 3, c.height * 3), Image.NEAREST))
        x = pad + lab
        for c in crops:
            sheet.paste(c, (x, y + 160 + 24))
            x += 320 + pad
        d.text((pad + lab, y + 160 + 13), 'ENLARGED THREE TIMES: THE LANTERN AND WHAT IT LIGHTS, IN THE SAME FOUR', fill=dim)
        y += rowh + pad
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    sheet.save(OUT)
    print('%s: %dx%d' % (OUT, W, H))


main()
