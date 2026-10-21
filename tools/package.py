#!/usr/bin/env python3
"""Build the release zip: HAUNTED.scr and HAUNTED.EXE, helper scripts, README, previews and the full source."""
import os, sys, zipfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
VERSION = open(os.path.join(ROOT, "VERSION")).read().strip()
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "dist", "HAUNTED-%s-win64.zip" % VERSION)
TOP = "HAUNTED-%s" % VERSION

files = ["dist/HAUNTED.scr", "dist/HAUNTED.EXE", "install.cmd", "test-windowed.cmd", "capture.cmd", "settings.cmd", "uninstall.cmd",
         "README.md", "LICENSE", "VERSION", "Makefile"]
for d in ("docs", "src", "tools", "res", "third_party"):
    for dp, _, fn in os.walk(os.path.join(ROOT, d)):
        for f in fn:
            files.append(os.path.relpath(os.path.join(dp, f), ROOT))

with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for f in sorted(set(files)):
        arc = os.path.basename(f) if f.startswith("dist/") else f
        z.write(os.path.join(ROOT, f), os.path.join(TOP, arc))
print("wrote", out, os.path.getsize(out), "bytes")
