#!/usr/bin/env python3
"""The stamps' generation sweep (docs/PLAN-stamps.md, section 6, track B).

Generates a level for each (seed, level) with the debug harness and reads what it printed (CRAWL_MAP): did it finish, did the stamps
fall back, can the player reach the exit, every key, every locked door, every secret room and what lies behind it.

    python3 tools/stampsweep.py [--seeds 1-200] [--levels 1,5,10,11,20,21,30,31,40] [--host build/host_debug] [--nostamps] [-j N]

--nostamps runs the same levels with CRAWL_NOSTAMPS=1 (today's generator), for the numbers to be set beside. Reachability is worked out
here from the printed map, keys aside (a locked door is passable) and with the secret walls shut, then open. A stamp overlapping a vault,
a wing, a trial's room or the arena is not tested directly: the generator's rule is that it never is, and a vault or key it had covered
would show here as a door or a key not reached.
"""
import argparse, collections, concurrent.futures as cf, os, re, subprocess, sys

FLOOR = set(" .okS@+$mxXDL")
SECRET_ROOM = set(":&")
WALLISH = set("#tOpf?")


def parse_range(s):
    out = []
    for part in s.split(","):
        if "-" in part:
            a, b = part.split("-"); out += list(range(int(a), int(b) + 1))
        else:
            out.append(int(part))
    return out


def one(host, seed, level, nostamps, timeout):
    env = dict(os.environ, CRAWL_SEED=str(seed), CRAWL_LEVEL=str(level), CRAWL_STAGE="1", CRAWL_MAP="1", CRAWL_STAMPLOG="1")
    if nostamps:
        env["CRAWL_NOSTAMPS"] = "1"
    try:
        p = subprocess.run([host, "apl", "crawl", "1"], env=env, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return dict(seed=seed, level=level, status="hang")
    text = p.stderr + p.stdout
    r = dict(seed=seed, level=level, status="ok" if p.returncode == 0 else "crash:%d" % p.returncode)
    r["fallback"] = re.findall(r"STAMPS level \d+: (a hall was cut off|the stone cut the level in two); made again", text)
    m = re.search(r"STAMPS level \d+: (\d+) rooms:(.*)", text)
    r["rooms"] = re.findall(r"\[(\w+) x\d+ plan", m.group(2)) if m else []
    r["secret_stamps"] = re.findall(r"chain \d+ door \([^)]*\) (\w+)\]", text)
    lines = text.split("\n")
    idx = next((i for i, l in enumerate(lines) if l.startswith("MAP level")), None)
    if idx is None:
        r["status"] = r["status"] if r["status"] != "ok" else "nomap"
        return r
    rows = [l[4:] for l in lines[idx + 1: idx + 46]]
    if len(rows) < 45 or any(len(x) < 45 for x in rows):
        r["status"] = "badmap"
        return r
    n = 45
    start = next(((x, y) for y in range(n) for x in range(n) if rows[y][x] in "S@"), None)
    if not start:
        r["status"] = "nostart"
        return r

    def flood(passable):
        seen = {start}; st = [start]
        while st:
            x, y = st.pop()
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                a, b = x + dx, y + dy
                if 0 <= a < n and 0 <= b < n and (a, b) not in seen and passable(rows[b][a]):
                    seen.add((a, b)); st.append((a, b))
        return seen
    closed = flood(lambda c: c in FLOOR)                                      # secret rooms shut
    opened = flood(lambda c: c in FLOOR or c in SECRET_ROOM or c == "?")      # and open
    cells = [(x, y) for y in range(n) for x in range(n) if rows[y][x] in FLOOR or rows[y][x] in SECRET_ROOM]
    ch = lambda p: rows[p[1]][p[0]]

    def near(p, S):
        x, y = p
        return any((x + dx, y + dy) in S for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    exits = [p for p in cells if ch(p) == "x"]
    r["exit_ok"] = bool(exits) and all(p in closed for p in exits)
    r["no_exit"] = not exits
    keys = [p for p in cells if ch(p) == "k"]
    r["keys"] = len(keys); r["keys_bad"] = sum(1 for p in keys if p not in opened)
    locked = [p for p in cells if ch(p) == "L"]
    r["locked"] = len(locked); r["locked_bad"] = sum(1 for p in locked if p not in closed)
    entr = [(x, y) for y in range(n) for x in range(n) if rows[y][x] == "?"]
    r["entr"] = len(entr); r["entr_bad"] = sum(1 for p in entr if not near(p, closed))
    secret_cells = [p for p in cells if ch(p) in SECRET_ROOM]
    bad_cells = [p for p in secret_cells if p not in opened]
    r["secret_cells_bad"] = len(bad_cells); r["bad_cells"] = bad_cells[:6]
    doors = [p for p in cells if ch(p) == "&"]
    r["door_loose"] = [p for p in doors if not any(rows[p[1] + dy][p[0] + dx] == ":" for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)) if 0 <= p[0] + dx < n and 0 <= p[1] + dy < n)]
    stranded = [p for p in cells if p not in opened and ch(p) not in SECRET_ROOM]
    r["stranded"] = len(stranded)
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", default="1-200"); ap.add_argument("--levels", default="1,5,10,11,20,21,30,31,40")
    ap.add_argument("--host", default="build/host_debug"); ap.add_argument("--nostamps", action="store_true")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 2); ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--list", action="store_true", help="print each level that fails")
    a = ap.parse_args()
    seeds, levels = parse_range(a.seeds), parse_range(a.levels)
    jobs = [(s, l) for s in seeds for l in levels]
    res = []
    with cf.ThreadPoolExecutor(a.j) as ex:
        for r in ex.map(lambda sl: one(a.host, sl[0], sl[1], a.nostamps, a.timeout), jobs):
            res.append(r)
    tot = len(res)
    bad = collections.Counter()
    fails = []
    for r in res:
        why = []
        if r["status"] != "ok": why.append(r["status"])
        if r.get("no_exit"): why.append("no exit")
        elif "exit_ok" in r and not r["exit_ok"]: why.append("exit not reached")
        if r.get("keys_bad"): why.append("key not reached")
        if r.get("locked_bad"): why.append("locked door not reached")
        if r.get("entr_bad"): why.append("secret entrance not reached")
        if r.get("door_loose"): why.append("doorway touches no room floor %s" % (r["door_loose"],))
        if r.get("secret_cells_bad"): why.append("secret room/doorway not reached %s" % (r["bad_cells"],))
        for w in why: bad[w.split(" (")[0].split(" [")[0]] += 1
        if why: fails.append((r["seed"], r["level"], why))
    fb = sum(1 for r in res if r.get("fallback"))
    withst = [r for r in res if r.get("rooms")]
    stranded = sorted(r.get("stranded", 0) for r in res)
    print("%d levels (%d seeds x %d levels)%s" % (tot, len(seeds), len(levels), "  [CRAWL_NOSTAMPS=1]" if a.nostamps else ""))
    print("failures: %s" % (dict(bad) if bad else "none"))
    print("made again without stamps: %d levels" % fb)
    print("levels with drawn rooms: %d; rooms a level: %s" % (len(withst), dict(sorted(collections.Counter(len(r["rooms"]) for r in res).items()))))
    print("floor cells stranded (not reached even with secrets open): levels with any %d, median %d, 90th %d, worst %d" %
          (sum(1 for s in stranded if s), stranded[len(stranded) // 2], stranded[int(len(stranded) * .9)], stranded[-1]))
    names = collections.Counter(n for r in res for n in r["rooms"])
    if names:
        print("rooms by stamp: " + ", ".join("%s %d" % kv for kv in names.most_common()))
    sec = collections.Counter(n for r in res for n in r.get("secret_stamps", []))
    if sec:
        print("secret rooms by stamp: " + ", ".join("%s %d" % kv for kv in sec.most_common()))
    if a.list or fails:
        for s, l, w in fails[:60]:
            print("  FAIL seed %d level %d: %s" % (s, l, ", ".join(w)))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
