#!/usr/bin/env python3
"""Frames of one creature from the game at several distances, side by side (docs/InitialArtPass.md).

  python3 tools/stage.py <kind> <out.png> [--rank r] [--level n] [--dark f] [--fog] [--sub s] [--dists 2.5,4,6,9,13,18]

kind is the MOB_* number. Needs build/host_debug. Each frame is the game's own 320x200, doubled; a strip of them
with the distance under each. --dark 0 is no light at all, --fog the thickest fog."""
import argparse, os, subprocess, sys, tempfile
from PIL import Image, ImageDraw

ap = argparse.ArgumentParser()
ap.add_argument("kind", type=int); ap.add_argument("out")
ap.add_argument("--rank"); ap.add_argument("--level", default="5"); ap.add_argument("--seed", default="5")
ap.add_argument("--dark"); ap.add_argument("--fog", action="store_true"); ap.add_argument("--sub")
ap.add_argument("--dists", default="2.5,4,6,9,13,18"); ap.add_argument("--t", default="6")
a = ap.parse_args()
host = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", "host_debug")
frames = []
with tempfile.TemporaryDirectory() as d:
    for i, dist in enumerate(a.dists.split(",")):
        env = dict(os.environ, CRAWL_SEED=a.seed, CRAWL_LEVEL=a.level, CRAWL_STAGE=str(a.kind), CRAWL_STAGEDIST=dist)
        if a.rank is not None: env["CRAWL_RANK"] = a.rank
        if a.dark is not None: env["CRAWL_STAGEDARK"] = a.dark
        if a.fog: env["CRAWL_STAGEFOG"] = "1"
        if a.sub is not None: env["CRAWL_STAGESUB"] = a.sub
        subprocess.run([host, "shot", "crawl", a.t, os.path.join(d, "f%d" % i), "2"], env=env, cwd=d,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        name = [f for f in os.listdir(d) if f.startswith("f%d_" % i)][0]
        frames.append((dist, Image.open(os.path.join(d, name)).convert("RGB")))
w, h = frames[0][1].size
out = Image.new("RGB", (len(frames) * (w + 6) - 6, h + 20), (0, 0, 0))
dr = ImageDraw.Draw(out)
for i, (dist, im) in enumerate(frames):
    out.paste(im, (i * (w + 6), 0))
    dr.text((i * (w + 6) + 6, h + 4), "%s cells" % dist, fill=(200, 200, 200))
out.save(a.out)
print(a.out)
