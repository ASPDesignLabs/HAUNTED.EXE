#!/usr/bin/env python3
"""The probe's page (docs/PLAN-mobile.md, step 0): build/web made over into a page for a host that wraps the page in its
own document and passes no query string, such as a private page on claude.ai. Its index.html loses its <html>, <head>
and <body> (the host gives them), its own viewport and charset (the host's stand), the canonical link and the icon; it
gains the probe from the start (window.HAUNTED_PROBE, "take" unless told "perf"), the version the <html> tag carried,
the fonts as data URIs, and the project page's address for its link home. The game's files are copied beside it.

    make web && python3 tools/probe_page.py [build/probe] [take|perf]
"""
import base64, os, re, shutil, sys

SRC = "build/web"
OUT = sys.argv[1] if len(sys.argv) > 1 else "build/probe"
MODE = sys.argv[2] if len(sys.argv) > 2 else "take"
HOME = "https://aspdesignlabs.github.io/HAUNTED.EXE/"

page = open(os.path.join(SRC, "index.html"), encoding="utf-8").read()
version = re.search(r'data-version="([^"]*)"', page).group(1)

def cut(pattern, text, count=1):
    new, n = re.subn(pattern, "", text, count=count, flags=re.I | re.S)
    if n != count:
        sys.exit(f"probe_page: expected {count} of {pattern!r}, found {n}")
    return new

page = cut(r"<!doctype html>\s*", page)
page = cut(r"<html[^>]*>\s*", page)
page = cut(r"</html>\s*", page)
page = cut(r"<head>\s*", page)
page = cut(r"</head>\s*", page)
page = cut(r"<body>\s*", page)
page = cut(r"</body>\s*", page)
page = cut(r'<meta charset="utf-8">\s*', page)
page = cut(r'<meta name="viewport"[^>]*>\s*', page)
page = cut(r'<link rel="canonical"[^>]*>\s*', page)
page = cut(r'<link rel="icon"[^>]*>\s*', page)
page = re.sub(r"<title>[^<]*</title>", "<title>HAUNTED.EXE Phone Probe</title>", page, count=1)

for name in ("Px437_IBM_VGA_8x16.ttf", "Px437_IBM_BIOS.ttf"):     # fonts inline, whatever the host makes of a font file
    data = base64.b64encode(open(os.path.join(SRC, "fonts", name), "rb").read()).decode()
    before = page
    page = page.replace(f'url("fonts/{name}")', f'url("data:font/ttf;base64,{data}")')
    if page == before:
        sys.exit(f"probe_page: no url for {name}")

page = page.replace('href="../"', f'href="{HOME}"', 1)
flag = (f'<script>document.documentElement.dataset.version = "{version}"; '
        f'window.HAUNTED_PROBE = "{MODE}";</script>\n')
page = page.replace('<script src="haunted-core.js"></script>', flag + '<script src="haunted-core.js"></script>', 1)

os.makedirs(OUT, exist_ok=True)
open(os.path.join(OUT, "index.html"), "w", encoding="utf-8").write(page)
for f in ("haunted.js", "audio.js", "haunted-core.js", "haunted-core.wasm"):
    shutil.copy(os.path.join(SRC, f), os.path.join(OUT, f))
print(f"{OUT}: the probe's page for {version} ({MODE}), {len(page)} bytes of page")
