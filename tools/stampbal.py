#!/usr/bin/env python3
"""The stamps' balance runs (docs/PLAN-stamps.md, section 6, track B): the autopilot (the dream) on the same seeds in this build and in an
older one, side by side.

    python3 tools/stampbal.py --old build/bal/b216/build/host_debug [--new build/host_debug] [--seeds 1,3,5,...] [--secs 3000]
                              [--logs build/bal/logs] [--reuse]

--old is a debug harness built from the commit to weigh against (a `git worktree`, `make build/host_debug` in it). Each seed runs the dream
for --secs of its own time in each build (about 4 s of ours for every 100 of its time); logs go to --logs, and --reuse reads logs already
there. The harness ORs the seed with 1, so seeds are given odd. Numbers: levels cleared, deaths, the median seconds a level took (from
`cleared`, so a level the dream never left is not counted), secret rooms found ("A ROOM FOUND") and the gold at the last state line.
Eight seeds carry noise of about a quarter either way (CLAUDE.md): read the means, not one seed.
"""
import argparse, concurrent.futures as cf, os, re, statistics, subprocess, sys


def run(host, seed, secs, path, reuse):
    if not (reuse and os.path.exists(path)):
        env = dict(os.environ, CRAWL_SEED=str(seed))
        with open(path, "w") as f:
            try:
                subprocess.run([host, "apl", "crawl", str(secs)], env=env, stdout=f, stderr=subprocess.STDOUT, timeout=secs)
            except subprocess.TimeoutExpired:
                f.write("\nTIMEOUT\n")
    return path


def read(path):
    t = open(path, errors="replace").read()
    cleared = [(int(a), float(b)) for a, b in re.findall(r"EVENT cleared level (\d+) t=([\d.]+)", t)]
    died = re.findall(r"EVENT died level (\d+) t=([\d.]+)", t)
    last = re.findall(r"^lvl=(\d+) t=([\d.]+).*?gold=(\d+)", t, re.M)
    return dict(cleared=len(cleared), died=len(died), times=[b for _, b in cleared], rooms=len(re.findall(r"A ROOM FOUND", t)),
                level=int(last[-1][0]) if last else 0, gold=int(last[-1][2]) if last else 0, timeout="TIMEOUT" in t, crash=bool(re.search(r"Segmentation|runtime error|AddressSanitizer", t)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--old", required=True); ap.add_argument("--new", default="build/host_debug")
    ap.add_argument("--seeds", default=",".join(str(i) for i in range(1, 32, 2))); ap.add_argument("--secs", type=int, default=3000)
    ap.add_argument("--logs", default="build/bal/logs"); ap.add_argument("--reuse", action="store_true")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 2)
    a = ap.parse_args()
    os.makedirs(a.logs, exist_ok=True)
    seeds = [int(s) for s in a.seeds.split(",")]
    jobs = [(w, h, s) for s in seeds for w, h in (("old", a.old), ("new", a.new))]
    with cf.ThreadPoolExecutor(a.j) as ex:
        paths = list(ex.map(lambda j: run(j[1], j[2], a.secs, "%s/%s_%d.log" % (a.logs, j[0], j[2]), a.reuse), jobs))
    R = {(w, s): read(p) for (w, h, s), p in zip(jobs, paths)}
    print("seed |  old: cleared died level gold rooms |  new: cleared died level gold rooms")
    for s in seeds:
        o, n = R[("old", s)], R[("new", s)]
        print("%4d |  %6d %4d %5d %5d %5d |  %6d %4d %5d %5d %5d%s" % (s, o["cleared"], o["died"], o["level"], o["gold"], o["rooms"],
              n["cleared"], n["died"], n["level"], n["gold"], n["rooms"], "  CRASH" if n["crash"] or o["crash"] else ""))
    print("\n%-34s %10s %10s" % ("over %d seeds, %d s each" % (len(seeds), a.secs), "old", "new"))
    for label, f in (("levels cleared (mean)", lambda r: r["cleared"]), ("deaths (total)", lambda r: r["died"]), ("seeds with a death", lambda r: r["died"] > 0),
                     ("level reached (mean)", lambda r: r["level"]), ("secret rooms found (mean)", lambda r: r["rooms"]), ("gold at the end (mean)", lambda r: r["gold"])):
        v = {w: [f(R[(w, s)]) for s in seeds] for w in ("old", "new")}
        agg = (lambda x: sum(x)) if "total" in label or "seeds with" in label else (lambda x: statistics.mean(x))
        print("%-34s %10.1f %10.1f" % (label, agg(v["old"]), agg(v["new"])))
    for w in ("old", "new"):
        ts = [t for s in seeds for t in R[(w, s)]["times"]]
        print("median seconds a level took (%s): %s over %d levels" % (w, "%.0f" % statistics.median(ts) if ts else "-", len(ts)))
    print("timeouts or crashes: " + str(sum(1 for r in R.values() if r["timeout"] or r["crash"])))


if __name__ == "__main__":
    main()
