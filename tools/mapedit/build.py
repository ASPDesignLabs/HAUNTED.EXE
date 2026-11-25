#!/usr/bin/env python3
"""Builds the world editor's page (the map editor) (docs/PLAN-mapedit.md): editor.html with core.js and the game's data put in, one file, as the
artifact is published. Run from the repository's root: python3 -I tools/mapedit/build.py [out]"""
import json, os, subprocess, sys
here = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1] if len(sys.argv) > 1 else "build/mapedit/haunted-world-editor.html"
data = subprocess.run([sys.executable, "-I", os.path.join(here, "extract.py")], check=True, capture_output=True, text=True).stdout
json.loads(data)
with open(os.path.join(here, "data.json"), "w", encoding="utf-8") as f: f.write(data)
page = open(os.path.join(here, "editor.html"), encoding="utf-8").read()
core = open(os.path.join(here, "core.js"), encoding="utf-8").read()
if "</script" in core or "</script" in data: sys.exit("a closing script tag would end the page's script early")
page = page.replace("/*@CORE@*/", core).replace("/*@DATA@*/", data)
os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
with open(out, "w", encoding="utf-8") as f: f.write(page)
print("%s: %d bytes" % (out, len(page)))
