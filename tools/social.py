#!/usr/bin/env python3
"""The multimedia package for social media and the project page: everything is cut from the game as the harness runs it
(`host film`), captioned in the game's own VGA font, and written to media/social/.

    python3 tools/social.py footage            render the raw clips from the harness (build/social/raw/, cached)
    python3 tools/social.py trailer [--music piece]   the trailer in 16:9, 1:1 and 9:16 (video/)
    python3 tools/social.py clips              each scene's own clip, with the game's own sound (clips/)
    python3 tools/social.py gifs               silent loops for X, Discord and the README (gif/)
    python3 tools/social.py images             stills as 16:9, 1:1 and 9:16, and the repository's social preview (images/)
    python3 tools/social.py music              the pieces to choose from, each through the trailer's fight (music-candidates/)
    python3 tools/social.py all                all of it

Needs ffmpeg, Pillow and build/host_debug (make build/host_debug). Nothing here touches the game: the one thing added to
the harness is its `film` command. The footage is deterministic (fixed seeds), so a run gives the same frames again.
British spelling, and the dry voice of the README, in every caption.
"""
import os, subprocess, sys, shutil, hashlib
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
HOST = os.path.join(ROOT, "build", "host_debug")
RAW = os.path.join(ROOT, "build", "social", "raw")
TMP = os.path.join(ROOT, "build", "social", "tmp")
OUT = os.path.join(ROOT, "media", "social")
FONT = os.path.join(ROOT, "third_party", "font", "Px437_IBM_VGA_8x16.ttf")
FPS = 30
CRF = 21        # pixel art with a dither is dear to compress; 21 keeps the 16:9 trailer near 10 MB

ORANGE = (255, 140, 36); GOLD = (232, 194, 90); BLOOD = (210, 58, 38); CYAN = (82, 151, 151)
GREY = (170, 170, 170); WHITE = (255, 255, 255)

# ------------------------------------------------------------------------------------------------------ the footage
# name: (scene, environment, input, from, to). The times are the game's own, from the start of the run.
T = "1:+H;4.3:-H"
CLIPS = {
    "post":    ("haunt", {}, "", 0.5, 12.0),
    "virus":   ("haunt", {}, "", 36.0, 50.0),
    "pumpkin": ("haunt", {}, "", 62.0, 78.0),
    "bbs":     ("haunt", {}, "", 98.0, 114.0),
    "title":   ("crawl", {"CRAWL_SEED": "3"}, T, 5.0, 15.0),
    "hall":    ("crawl", {"CRAWL_SEED": "7", "CRAWL_LEVEL": "10", "CRAWL_TITLE": "0"}, "", 8.0, 20.0),
    "cast":    ("crawl", {"CRAWL_SEED": "3", "CRAWL_TITLE": "0"}, "", 42.0, 56.0),
    "boss":    ("crawl", {"CRAWL_SEED": "7", "CRAWL_LEVEL": "10", "CRAWL_TPBOSS": "2", "CRAWL_TITLE": "0"}, "", 22.0, 50.0),
    "mine":    ("crawl", {"CRAWL_SEED": "7", "CRAWL_LEVEL": "21", "CRAWL_TITLE": "0"}, "", 33.0, 49.0),
}


def run(cmd, **kw):
    r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, **kw)
    if r.returncode:
        sys.exit("failed: %s\n%s" % (" ".join(cmd), r.stderr[-2000:]))
    return r


def ff(*args):
    return run(["ffmpeg", "-loglevel", "error", "-y"] + list(args))


def film(name):
    """Render one clip: RAW/name.rgb (frames), name.wav (sound), name.txt (WxH and the count)."""
    scene, env, inp, t0, t1 = CLIPS[name]
    base = os.path.join(RAW, name)
    if os.path.exists(base + ".txt"):
        return
    os.makedirs(RAW, exist_ok=True)
    if not os.path.exists(HOST):
        sys.exit("build/host_debug is missing: make build/host_debug")
    e = dict(os.environ, **env)
    e["HOST_INPUT"] = inp
    e.pop("CRAWL_RUNID", None)
    r = subprocess.run([HOST, "film", scene, str(t0), str(t1), str(FPS), base], env=e, stdout=subprocess.DEVNULL,
                       stderr=subprocess.PIPE, text=True, cwd=RAW)
    line = [l for l in r.stderr.splitlines() if l.startswith("FILM ")]
    if r.returncode or not line:
        sys.exit("film %s failed:\n%s" % (name, r.stderr[-1500:]))
    _, w, h, n = line[-1].split()
    open(base + ".txt", "w").write("%s %s %s\n" % (w, h, n))
    print("footage %-8s %sx%s, %s frames" % (name, w, h, n))


def info(name):
    film(name)
    w, h, n = open(os.path.join(RAW, name + ".txt")).read().split()
    return int(w), int(h), int(n)


def frame(name, t):
    """The frame t seconds into a clip as a PIL image."""
    w, h, n = info(name)
    i = min(n - 1, int(round(t * FPS)))
    with open(os.path.join(RAW, name + ".rgb"), "rb") as f:
        f.seek(i * w * h * 3)
        return Image.frombytes("RGB", (w, h), f.read(w * h * 3))


# ------------------------------------------------------------------------------------------------------ the layout
ASPECTS = {"16x9": (1920, 1080), "1x1": (1080, 1080), "9x16": (1080, 1920)}


def place(aspect, sw, sh):
    """Where a source frame goes on a canvas: (scale, x, y). Whole-number scales where there is room, so the pixels stay
    square; the 720-wide scene is a half step on a phone, where there is no more width to give it."""
    W, H = ASPECTS[aspect]
    if aspect == "16x9":
        k = 4 if sw <= 320 else 2          # 4x leaves bars top and bottom for the words, clear of the game's own bars
        return k, (W - sw * k) // 2, 150
    if sw <= 320:
        k = 3
    else:
        k = 1.5
    pw, ph = int(sw * k), int(sh * k)
    return k, (W - pw) // 2, (H - ph) // 2 if aspect == "1x1" else 760


def font(scale):
    f = ImageFont.truetype(FONT, 16 * scale)
    return f


def wrap(text, cols):
    out, cur = [], ""
    for w in text.split():
        if cur and len(cur) + 1 + len(w) > cols:
            out.append(cur); cur = w
        else:
            cur = (cur + " " + w).strip()
    if cur:
        out.append(cur)
    return out


def text_px(d, xy, s, fnt, fill, shadow=True, scale=1):
    x, y = xy
    if shadow:
        d.text((x + scale * 2, y + scale * 2), s, font=fnt, fill=(0, 0, 0, 255))
    d.text((x, y), s, font=fnt, fill=fill)


def caption_layer(aspect, caption, sw, sh, small=None, colour=GOLD):
    """The words of a shot as a transparent layer the size of the canvas, drawn as the game draws its own: the VGA font,
    whole pixels. 16:9 puts them on a plate at the top; a square and a phone put them in the bars."""
    W, H = ASPECTS[aspect]
    k, x, y = place(aspect, sw, sh)
    L = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(L)
    d.fontmode = "1"
    if caption:
        s = 3 if aspect == "16x9" else 4
        fnt = font(s)
        lines = wrap(caption.upper(), (W - 120) // (8 * s))
        lh = 16 * s + 8
        if aspect == "16x9":
            ph = lh * len(lines) + 28
            plate = Image.new("RGBA", (W, ph), (0, 0, 0, 175))
            L.paste(plate, (0, 0))
            ty = 14
        elif aspect == "1x1":
            ty = max(30, (y - lh * len(lines)) // 2)
        else:
            ty = 760 - 60 - lh * len(lines) - 80
            hf = font(8)
            tw = len("HAUNTED.EXE") * 64
            text_px(d, ((W - tw) // 2, 300), "HAUNTED.EXE", hf, ORANGE, scale=3)
        for ln in lines:
            tw = len(ln) * 8 * s
            text_px(d, ((W - tw) // 2, ty), ln, fnt, colour, scale=s // 2 or 1)
            ty += lh
    if small:
        s = 2
        fnt = font(s)
        tw = len(small) * 8 * s
        if aspect == "16x9":
            yy = H - 16 * s - 14
            d.rectangle((0, yy - 8, W, H), fill=(0, 0, 0, 175))
        elif aspect == "1x1":
            yy = H - (H - (y + int(sh * k))) // 2 - 16
        else:
            yy = y + int(sh * k) + 80
        text_px(d, ((W - tw) // 2, yy), small, fnt, GREY, scale=1)
    return L


def card(aspect, big, lines, colours=None):
    """A title card: black, the big words in the game's face at a whole-number scale, a few lines under."""
    W, H = ASPECTS[aspect]
    C = Image.new("RGB", (W, H), (0, 0, 0))
    d = ImageDraw.Draw(C)
    d.fontmode = "1"
    s = 8 if aspect == "16x9" else 6
    fnt = font(s)
    big_lines = big.split("|")
    total = len(big_lines) * (16 * s + 12) + 40 + len(lines) * 48
    y = (H - total) // 2
    for b in big_lines:
        tw = len(b) * 8 * s
        text_px(d, ((W - tw) // 2, y), b, fnt, ORANGE, scale=3)
        y += 16 * s + 12
    y += 40
    f2 = font(3 if aspect == "16x9" else 2)
    for i, ln in enumerate(lines):
        col = (colours or [GREY] * len(lines))[i]
        sc = 3 if aspect == "16x9" else 2
        tw = len(ln) * 8 * sc
        text_px(d, ((W - tw) // 2, y), ln, f2, col, shadow=False)
        y += 48
    return C


# ------------------------------------------------------------------------------------------------------ the trailer
# (clip, from within the clip, seconds, caption, small line, colour) or ("card", ...)
SHOTS = [
    ("post",    1.0, 4.0, "1994. A perfectly ordinary 486.", None, GOLD),
    ("virus",   3.0, 5.0, "Something has already been installed.", None, BLOOD),
    ("pumpkin", 4.0, 4.0, "It has been carved for you.", None, ORANGE),
    ("title",   1.0, 4.0, "Hold H for three seconds.", "WINDOWS SCREENSAVER  ·  FREE  ·  GPL-3.0", GOLD),
    ("hall",    3.0, 5.0, "The merchant will see you now.", None, GOLD),
    ("cast",    2.0, 7.0, "It plays itself, until you take over.", None, GOLD),
    ("boss",    8.0, 8.0, "A knight, at the end of ten levels.", None, BLOOD),
    ("mine",    9.0, 6.0, "The dead are kept in the plot.", None, BLOOD),
]
CARD_SECS = 4.0
FADE = 0.2


def seg_video(aspect, clip, t0, dur, caption, small, colour, out):
    w, h, n = info(clip)
    k, x, y = place(aspect, w, h)
    W, H = ASPECTS[aspect]
    layer = os.path.join(TMP, "cap_%s_%s_%d.png" % (aspect, clip, int(t0 * 10)))
    caption_layer(aspect, caption, w, h, small, colour).save(layer)
    pw, ph = int(w * k), int(h * k)
    f = ("[0:v]scale=%d:%d:flags=neighbor,pad=%d:%d:%d:%d:black[b];[b][1:v]overlay,"
         "fade=t=in:d=%.2f,fade=t=out:st=%.2f:d=%.2f,format=yuv420p[v]" % (pw, ph, W, H, x, y, FADE, dur - FADE, FADE))
    ff("-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "%dx%d" % (w, h), "-r", str(FPS), "-ss", "%.3f" % t0, "-t", "%.3f" % dur,
       "-i", os.path.join(RAW, clip + ".rgb"), "-i", layer, "-filter_complex", f, "-map", "[v]", "-r", str(FPS),
       "-c:v", "libx264", "-crf", str(CRF), "-preset", "slow", "-tune", "animation", "-an", out)


def seg_card(aspect, dur, out, big, lines, cols):
    W, H = ASPECTS[aspect]
    p = os.path.join(TMP, "card_%s_%s.png" % (aspect, hashlib.md5(big.encode()).hexdigest()[:6]))
    card(aspect, big, lines, cols).save(p)
    ff("-loop", "1", "-framerate", str(FPS), "-t", "%.3f" % dur, "-i", p, "-vf",
       "fade=t=in:d=%.2f,fade=t=out:st=%.2f:d=%.2f,format=yuv420p" % (FADE, dur - FADE, FADE), "-c:v", "libx264", "-crf", str(CRF), "-an", out)


def trailer_audio(piece, total):
    """The first shots (the 486) keep the game's own sound; the crypt's piece takes over under the rest and rises as the
    fight does, then falls away at the card."""
    os.makedirs(TMP, exist_ok=True)
    parts, t = [], 0.0
    for i, s in enumerate(SHOTS):
        if s[0] in ("post", "virus", "pumpkin"):
            p = os.path.join(TMP, "a%d.wav" % i)
            ff("-i", os.path.join(RAW, s[0] + ".wav"), "-ss", "%.3f" % s[1], "-t", "%.3f" % s[2], p)
            parts.append(p); t += s[2]
    haunt_end = t
    rest = total - haunt_end
    mus = os.path.join(TMP, "music.wav")
    boss_at = sum(s[2] for s in SHOTS[3:6])
    run([HOST, "music", piece, "%.1f" % (rest + 2), mus, "fight1@%.1f" % (boss_at - 8), "fight2@%.1f" % (boss_at - 2),
         "fight3@%.1f" % boss_at, "fight0@%.1f" % (boss_at + SHOTS[6][2] - 1), "frail@%.1f" % (boss_at + SHOTS[6][2] + 1)])
    ff("-i", mus, "-af", "afade=t=in:d=1.0,afade=t=out:st=%.2f:d=2.5" % (rest - 2.5), os.path.join(TMP, "music_fx.wav"))
    ins = [a for p in parts for a in ("-i", p)]
    ff(*ins, "-filter_complex", "".join("[%d:a]" % i for i in range(len(parts))) + "concat=n=%d:v=0:a=1[o]" % len(parts), "-map", "[o]",
       os.path.join(TMP, "haunt_audio.wav"))
    out = os.path.join(TMP, "trailer_audio.wav")
    ff("-i", os.path.join(TMP, "haunt_audio.wav"), "-i", os.path.join(TMP, "music_fx.wav"), "-filter_complex",
       "[0:a]apad=pad_dur=0.1[a];[a][1:a]acrossfade=d=0.8:c1=tri:c2=tri,loudnorm=I=-16:TP=-1.5:LRA=11[o]", "-map", "[o]",
       "-t", "%.2f" % total, out)
    return out


def trailer(piece="crypts"):
    shutil.rmtree(TMP, ignore_errors=True); os.makedirs(TMP)
    os.makedirs(os.path.join(OUT, "video"), exist_ok=True)
    total = sum(s[2] for s in SHOTS) + CARD_SECS
    audio = trailer_audio(piece, total)
    for aspect in ASPECTS:
        segs = []
        for i, (clip, t0, dur, cap, small, col) in enumerate(SHOTS):
            p = os.path.join(TMP, "s_%s_%d.mp4" % (aspect, i))
            seg_video(aspect, clip, t0, dur, cap, small, col, p)
            segs.append(p)
        p = os.path.join(TMP, "s_%s_card.mp4" % aspect)
        seg_card(aspect, CARD_SECS, p, "HAUNTED.EXE", ["A HALLOWEEN SCREENSAVER FOR WINDOWS", "ASPDESIGNLABS.GITHUB.IO/HAUNTED.EXE",
                 "1.0.0B21.8  ·  PRE-RELEASE"], [GOLD, GREY, GREY])
        segs.append(p)
        lst = os.path.join(TMP, "list_%s.txt" % aspect)
        open(lst, "w").write("".join("file '%s'\n" % s for s in segs))
        dst = os.path.join(OUT, "video", "haunted-trailer-%s.mp4" % aspect)
        ff("-f", "concat", "-safe", "0", "-i", lst, "-i", audio, "-map", "0:v", "-map", "1:a", "-c:v", "copy", "-c:a", "aac",
           "-b:a", "160k", "-shortest", "-movflags", "+faststart", dst)
        print("trailer %s  %.1f s  %d KB" % (aspect, total, os.path.getsize(dst) // 1024))


# ------------------------------------------------------------------------------------------------------ clips and loops
CLIP_OUT = {   # name: caption-free 16:9 clip with the game's own sound
    "post": "haunt-post", "virus": "haunt-virus", "pumpkin": "haunt-pumpkin", "bbs": "haunt-bbs", "title": "title",
    "hall": "merchant-hall", "cast": "crypt-cast", "boss": "boss-knight", "mine": "charnel-mine",
}


def clips():
    os.makedirs(os.path.join(OUT, "clips"), exist_ok=True)
    for name, dst in CLIP_OUT.items():
        w, h, n = info(name)
        k, x, y = place("16x9", w, h)
        W, H = ASPECTS["16x9"]
        d = os.path.join(OUT, "clips", dst + ".mp4")
        ff("-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "%dx%d" % (w, h), "-r", str(FPS), "-i", os.path.join(RAW, name + ".rgb"),
           "-i", os.path.join(RAW, name + ".wav"), "-vf", "scale=%d:%d:flags=neighbor,pad=%d:%d:%d:%d:black,format=yuv420p" % (w * k, h * k, W, H, x, y),
           "-c:v", "libx264", "-crf", "23", "-preset", "slow", "-tune", "animation", "-c:a", "aac", "-b:a", "160k", "-af", "loudnorm=I=-16:TP=-1.5:LRA=11", "-shortest",
           "-movflags", "+faststart", d)
        print("clip %-16s %d KB" % (dst, os.path.getsize(d) // 1024))


GIFS = {   # name: (clip, from, seconds, width, fps)
    "haunt-virus": ("virus", 2.0, 8.0, 720, 15), "haunt-pumpkin": ("pumpkin", 3.0, 8.0, 720, 15),
    "title": ("title", 0.5, 8.0, 640, 15), "crypt-cast": ("cast", 1.0, 9.0, 640, 12), "boss-knight": ("boss", 7.0, 9.0, 640, 12),
    "merchant-hall": ("hall", 1.0, 9.0, 640, 15),
}


def gifs():
    os.makedirs(os.path.join(OUT, "gif"), exist_ok=True)
    for name, (clip, t0, dur, width, fps) in GIFS.items():
        w, h, n = info(clip)
        d = os.path.join(OUT, "gif", name + ".gif")
        flt = ("fps=%d,scale=%d:-1:flags=neighbor,split[a][b];[a]palettegen=stats_mode=diff:max_colors=112[p];"
               "[b][p]paletteuse=dither=bayer:bayer_scale=5:diff_mode=rectangle" % (fps, width))
        ff("-f", "rawvideo", "-pix_fmt", "rgb24", "-s", "%dx%d" % (w, h), "-r", str(FPS), "-ss", "%.3f" % t0, "-t", "%.3f" % dur,
           "-i", os.path.join(RAW, clip + ".rgb"), "-filter_complex", flt, "-loop", "0", d)
        print("gif %-16s %d KB" % (name, os.path.getsize(d) // 1024))


# ------------------------------------------------------------------------------------------------------ the stills
STILLS = [   # name, clip, second, caption, small line, colour
    ("title",   "title",   3.0, "Hold H for three seconds.", None, GOLD),
    ("virus",   "virus",   4.0, "Something has already been installed.", None, BLOOD),
    ("pumpkin", "pumpkin", 8.0, "It has been carved for you.", None, ORANGE),
    ("bbs",     "bbs",     8.0, "Someone is still on the line.", None, CYAN),
    ("merchant", "hall",   8.0, "The merchant will see you now.", None, GOLD),
    ("cast",    "cast",    9.0, "It plays itself, until you take over.", None, GOLD),
    ("knight",  "boss",    9.0, "The Ill-Fated Skeleton Knight.", None, BLOOD),
    ("fireball", "boss",  22.0, "Spells cast weak when the spirit is low.", None, ORANGE),
    ("perished", "mine",  12.0, "You have perished. Level 21 begins again.", None, BLOOD),
]


def still_image(aspect, clip, t, caption, small, colour):
    w, h, n = info(clip)
    k, x, y = place(aspect, w, h)
    W, H = ASPECTS[aspect]
    img = Image.new("RGBA", (W, H), (0, 0, 0, 255))
    fr = frame(clip, t).resize((int(w * k), int(h * k)), Image.NEAREST).convert("RGBA")
    img.paste(fr, (x, y))
    img.alpha_composite(caption_layer(aspect, caption, w, h, small or "ASPDESIGNLABS.GITHUB.IO/HAUNTED.EXE", colour))
    return img.convert("RGB")


def images():
    for aspect in ASPECTS:
        os.makedirs(os.path.join(OUT, "images", aspect), exist_ok=True)
    for name, clip, t, cap, small, col in STILLS:
        for aspect in ASPECTS:
            p = os.path.join(OUT, "images", aspect, "%s-%s.png" % (name, aspect))
            still_image(aspect, clip, t, cap, small, col).save(p, optimize=True)
    # the repository's social preview (Settings > Social preview; 1280x640, under 1 MB) and a header for the README
    W, H = 1280, 640
    img = Image.new("RGB", (W, H), (0, 0, 0))
    tf = frame("title", 3.0)
    g = tf.resize((1280, 800), Image.NEAREST).crop((0, 0, 1280, 480))
    img.paste(g, (0, 0))
    d = ImageDraw.Draw(img); d.fontmode = "1"
    f = font(2)
    msg = "A HALLOWEEN SCREENSAVER FOR WINDOWS"
    text_px(d, ((W - len(msg) * 16) // 2, 500), msg, f, GOLD, scale=1)
    msg2 = "A POSSESSED 486 AND A CRYPT YOU CAN TAKE OVER"
    text_px(d, ((W - len(msg2) * 16) // 2, 550), msg2, f, GREY, scale=1)
    msg3 = "HOLD H FOR THREE SECONDS"
    text_px(d, ((W - len(msg3) * 16) // 2, 592), msg3, f, ORANGE, scale=1)
    os.makedirs(os.path.join(OUT, "images", "github"), exist_ok=True)
    p = os.path.join(OUT, "images", "github", "social-preview.png")
    img.save(p, optimize=True)
    print("images: %d stills in three shapes, and the social preview (%d KB)" % (len(STILLS), os.path.getsize(p) // 1024))


# ------------------------------------------------------------------------------------------------------ the music
PIECES = ["crypts", "catacombs", "hollows", "deep", "title", "office", "ossuary", "merchant"]


def music():
    os.makedirs(os.path.join(OUT, "music-candidates"), exist_ok=True)
    os.makedirs(TMP, exist_ok=True)
    for pc in PIECES:
        wav = os.path.join(TMP, pc + ".wav")
        env = dict(os.environ, HOST_DRIPS="0")
        subprocess.run([HOST, "music", pc, "60", wav, "fight1@14", "fight2@26", "fight3@38", "fight0@50", "frail@54"], env=env,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        dst = os.path.join(OUT, "music-candidates", "%s.mp3" % pc)
        ff("-i", wav, "-af", "afade=t=in:d=0.5,afade=t=out:st=57:d=3,loudnorm=I=-16:TP=-1.5:LRA=11", "-codec:a", "libmp3lame", "-b:a", "128k", dst)
        print("music %-10s %d KB" % (pc, os.path.getsize(dst) // 1024))


def main():
    a = sys.argv[1:]
    cmd = a[0] if a else "all"
    piece = a[a.index("--music") + 1] if "--music" in a else "crypts"
    if cmd in ("footage", "all"):
        for n in CLIPS:
            film(n)
    if cmd in ("trailer", "all"): trailer(piece)
    if cmd in ("clips", "all"): clips()
    if cmd in ("gifs", "all"): gifs()
    if cmd in ("images", "all"): images()
    if cmd in ("music", "all"): music()
    if cmd not in ("footage", "trailer", "clips", "gifs", "images", "music", "all"):
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
