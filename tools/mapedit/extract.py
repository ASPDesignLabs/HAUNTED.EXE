#!/usr/bin/env python3
"""The map editor's data (docs/PLAN-mapedit.md), read from the game's source so the two never drift: the stamps, the
creature kinds (name, first level, top rank, cost), the level budgets, the gold's values and the room types.
Run from the repository's root: python3 -I tools/mapedit/extract.py > tools/mapedit/data.json"""
import json, re, sys

def read(p):
    with open(p, encoding="utf-8") as f: return f.read()

src = "src/crawl/"
stamp = read(src + "crawl_stamp_data.c")
defs = read(src + "game/defs.c")
budget = read(src + "game/budget.c")
gen = read(src + "game/generate.c")

# stamps: { "name", ST_KIND, w, h, minstep, maxstep, weight, runs, "row\n" "row" ... }
stamps = []
for m in re.finditer(r'\{\s*"(\w+)",\s*(ST_\w+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),((?:\s*"[^"]*")+)\s*\}', stamp):
    rows = "".join(re.findall(r'"([^"]*)"', m.group(9))).split("\\n")
    w, h = int(m.group(3)), int(m.group(4))
    if len(rows) != h or any(len(r) != w for r in rows): sys.exit("stamp %s: rows do not match %dx%d" % (m.group(1), w, h))
    stamps.append({"name": m.group(1), "kind": m.group(2)[3:].lower(), "w": w, "h": h, "minstep": int(m.group(5)),
                   "maxstep": int(m.group(6)), "weight": int(m.group(7)), "runs": int(m.group(8)), "rows": rows})

# kinds, in the enum's order
consts = {k: int(v) for k, v in re.findall(r"#define (\w+) (\d+)\b", "".join(read(src + f) for f in ("game/defs.c", "game/state.c", "crawl.h")))}
enum = re.search(r"enum \{ (MOB_SKEL[^}]*)\}", defs).group(1)
mobs = [t.strip() for t in re.sub(r"/\*.*?\*/", "", enum, flags=re.S).split(",") if t.strip()]
cost = {k: float(v) for k, v in re.findall(r"\[(MOB_\w+)\] = ([\d.]+)f", re.search(r"KIND_COST\[NKINDS\] = \{(.*?)\};", budget, re.S).group(1))}
kinds = []
for i, k in enumerate(mobs):
    m = re.search(r"\[%s\] = \{ \"([^\"]+)\".*?\n\s*\w+, \w+, \w+, \w+, -?\w+, (\w+), RK_(\w+), (\d), ([^,]+)," % k, defs, re.S)
    if not m: sys.exit("kind %s not found" % k)
    flags = m.group(5)
    kinds.append({"id": k, "n": i, "name": m.group(1), "debut": consts.get(m.group(2)) if not m.group(2).isdigit() else int(m.group(2)), "top": m.group(3), "gild": int(m.group(4)),
                  "cost": cost.get(k), "unique": "KF_UNIQUE" in flags, "uncounted": "KF_UNCOUNTED" in flags})

runb = [[float(x) for x in re.findall(r"([\d.]+)f", row)] for row in
        re.findall(r"\{([^{}]*)\}", re.search(r"RUNB\[4\]\[10\] = \{(.*?)\};", budget, re.S).group(1))]
items = [t.strip() for t in re.search(r"enum \{ (IT_COINS[^}]*)\}", defs).group(1).split(",")]
vals = [int(v) for v in re.search(r"ITEM_VALUE\[\] = \{([^}]*)\}", defs).group(1).split(",")]
rts = [t.strip() for t in re.search(r"enum \{ (RT_HALL.*?)NRT \}", gen, re.S).group(1).split(",") if t.strip()]
# the tutorial's drawn levels, as the round trip's test and the editor's examples
drawn_src = read(src + "game/drawn.c")
drawn = []
for m in re.finditer(r"\{ (DRAWN(\d+)_MAP), (\d+), (\d+), (\d+), (\d+), (\d+), DRAWN\d+_ROOM, [^,]+, \{ (-?\d+), (-?\d+), (-?\d+) \},\s*DRAWN\d+_THING, [^,]+, (0x[0-9A-Fa-f]+)u, (\d+), (\w+), (\d+) \}", drawn_src):
    n = m.group(2)
    rows = re.findall(r'"([^"]*)",\s*/\*', re.search(r"DRAWN%s_MAP\[\d+\] = \{(.*?)\};" % n, drawn_src, re.S).group(1))
    things = [{"what": a[3:], "kind": b, "x": float(c), "y": float(d)} for a, b, c, d in
              re.findall(r"\{ (DT_\w+), (\w+), ([\d.]+)f, ([\d.]+)f \}", re.search(r"DRAWN%s_THING\[\] = \{(.*?)\};" % n, drawn_src, re.S).group(1))]
    rooms = [{"x": int(a), "y": int(b), "w": int(c), "h": int(d), "rt": e} for a, b, c, d, e in
             re.findall(r"\{ (\d+), (\d+), (\d+), (\d+), (RT_\w+) \}", re.search(r"DRAWN%s_ROOM\[\] = \{(.*?)\};" % n, drawn_src, re.S).group(1))]
    drawn.append({"n": int(n), "w": int(m.group(3)), "h": int(m.group(4)), "ox": int(m.group(5)), "oy": int(m.group(6)),
                  "face": int(m.group(7)), "vault": [int(m.group(8)), int(m.group(9)), int(m.group(10))], "seed": m.group(11), "level": int(m.group(12)),
                  "rows": rows, "things": things, "rooms": rooms})
    if len(rows) != int(m.group(4)) or any(len(r) != int(m.group(3)) for r in rows): sys.exit("drawn level %s: rows do not match" % n)
if not drawn: sys.exit("no drawn levels found")
elems = [t.strip() for t in re.search(r"enum \{ (EL_FIRE[^}]*)\}", read(src + "crawl_gear.h")).group(1).split(",") if t.strip() != "NELEM"]
# the hazards (docs/PLAN-events.md, 2c): their names in order, each run's fixed, arising and sapping ones, and each one's element
haz_src = read(src + "game/hazards.c")
hz_names = [t.strip() for t in re.sub(r"/\*.*?\*/", "", re.search(r"enum \{ (HZ_ROCK.*?)NHZ \}", read(src + "game/state.c"), re.S).group(1), flags=re.S).split(",") if t.strip()]
def hz_rows(name):
    body = re.search(name + r" = \{(.*?)\};", haz_src, re.S).group(1)
    return [[k for k in re.findall(r"HZ_\w+", row)] for row in re.findall(r"\{([^{}]*)\}", body)]
hz_sap = re.findall(r"HZ_\w+", re.search(r"HZ_SAPPER\[4\] = \{(.*?)\};", haz_src, re.S).group(1))
hz_el = re.findall(r"EL_\w+|-1", re.search(r"HZ_ELEM\[NHZ\] = \{(.*?)\};", haz_src, re.S).group(1))
if len(hz_el) != len(hz_names): sys.exit("HZ_ELEM has %d, the enum %d" % (len(hz_el), len(hz_names)))
hazards = {"names": hz_names, "fixed": hz_rows(r"HZ_FIXED\[4\]\[4\]"), "arise": hz_rows(r"HZ_ARISE\[4\]\[3\]"), "sapper": hz_sap,
           "elem": {k: (None if e == "-1" else e) for k, e in zip(hz_names, hz_el)}}
# the light styles in the game's order (docs/PLAN-lights.md): how web_drawn_lights numbers them
dl = [t.strip() for t in re.search(r"enum \{ (DL_CHANDELIER.*?)NDL \}", read(src + "game/generate.c"), re.S).group(1).split(",") if t.strip()]
# the side passages built in (docs/PLAN-side.md): SIDE_TEXT's texts, as the editor's examples and the round trip's test
side_src = read(src + "game/side_levels.c")
side_body = side_src[side_src.index("{", side_src.index("SIDE_TEXT[]")):side_src.index("};", side_src.index("SIDE_TEXT[]"))]
sides, cur = [], ""
for line in side_body.split("\n"):
    lit = re.match(r'\s*"((?:[^"\\]|\\.)*)"(,?)', line)
    if not lit: continue
    cur += lit.group(1).encode("latin-1").decode("unicode_escape")
    if lit.group(2): sides.append({"n": len(sides), "text": cur}); cur = ""
if not sides or not sides[0]["text"].startswith("SIDE 1"): sys.exit("no side passages found")
json.dump({"drawn": drawn, "sides": sides, "elems": elems, "hazards": hazards, "lights": dl, "stamps": stamps, "kinds": kinds, "budget": runb, "items": dict(zip(items, vals)), "rooms": rts,
           "mapn": int(re.search(r"#define MAPN (\d+)", defs).group(1))}, sys.stdout, indent=None, separators=(",", ":"))
