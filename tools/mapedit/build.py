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
# PLAY (docs/PLAN-mapedit.md, part 3): the game's browser build (make web) in play/ beside the page, its page given a few lines so it
# takes its level from the editor around it, keeps its saves apart and plays CATACOMB alone. Published as the artifact's files.
web = "build/web"
play = os.path.join(os.path.dirname(out) or ".", "play")
if not os.path.exists(os.path.join(web, "haunted-core.wasm")):
    print("no browser build in build/web (make web): PLAY will have no game to open")
else:
    import shutil
    if os.path.isdir(play): shutil.rmtree(play)
    shutil.copytree(web, play)
    shim = """<script>
/* the world editor's PLAY (docs/PLAN-mapedit.md, part 3): the level comes from the editor around this page */
window.HVS_PREFIX = "haunted.exe/world-editor/";
window.HVS_FORCE = { haunt: 0, crawl: 1 };
window.HVS_WANT_LEVEL = parent !== window;
addEventListener("message", (e) => {
  if (e.source !== parent || !e.data || e.data.type !== "hvs-level" || typeof e.data.text !== "string") return;
  window.HVS_LEVEL = e.data.text;
  const note = document.createElement("p");
  note.className = "note";
  note.textContent = "THE WORLD EDITOR'S LEVEL: " + String(e.data.name || "").toUpperCase() + ", LEVEL " + e.data.level + ". A NEW RUN BEGINS THERE.";
  const press = document.getElementById("press");
  if (press) press.parentNode.insertBefore(note, press);
});
if (parent !== window) parent.postMessage({ type: "hvs-play-ready" }, location.origin && location.origin !== "null" ? location.origin : "*");
</script>
"""
    idx = os.path.join(play, "index.html")
    html = open(idx, encoding="utf-8").read()
    tag = '<script src="haunted-core.js"></script>'
    if tag not in html: sys.exit("the game's page has changed: no " + tag)
    html = html.replace(tag, shim + tag).replace("<title>HAUNTED.EXE &middot; Play</title>", "<title>HAUNTED.EXE &middot; World Editor Play</title>")
    open(idx, "w", encoding="utf-8").write(html)
    print("%s: the game, %d files" % (play, sum(len(f) for _, _, f in os.walk(play))))
