"""The title cards X and other sites show for the project page: five 1200x630 PNGs built from real
frames and the VGA fonts, drawn at the game's own pixel size and scaled by whole numbers.

    python3 tools/make_cards.py <frames> <out>      (make cards renders the frames and runs this; needs Pillow)
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

SRC, OUT = sys.argv[1], sys.argv[2]
FONTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "third_party", "font") + "/"
VGA = ImageFont.truetype(FONTS + "Px437_IBM_VGA_8x16.ttf", 16)    # 8x16 cells
BIOS = ImageFont.truetype(FONTS + "Px437_IBM_BIOS.ttf", 8)        # 8x8 cells
W, H = 1200, 630

BLACK = (0, 0, 0); GREY = (170, 170, 170); WHITE = (255, 255, 255); DIM = (110, 110, 118)
ORANGE = (255, 140, 36); GOLD = (232, 194, 90); BLOOD = (210, 58, 38); CYAN = (82, 151, 151)
STONE_HI = (111, 114, 131); STONE_MID = (55, 59, 76); STONE_LO = (41, 41, 55); STONE = (86, 89, 106)


def frame(name):
    return Image.open(f"{SRC}/{name}.png").convert("RGB")


def up(img, k):
    return img.resize((img.width * k, img.height * k), Image.NEAREST)


def glyphs(s, font=VGA, scale=1):
    """A string as a 1-bit mask at the font's own pixel size, scaled by a whole number."""
    cell_h = 16 if font is VGA else 8
    m = Image.new("L", (8 * len(s), cell_h), 0)
    d = ImageDraw.Draw(m)
    d.fontmode = "1"
    d.text((0, 0), s, font=font, fill=255)
    return up(m, scale)


def text(card, xy, runs, font=VGA, scale=1, shadow=0, box=None, align="left"):
    """runs: [(string, colour), ...] drawn one after another. shadow: hard drop shadow in px.
    box: a colour to fill each text cell with first (the text-mode look). align: left or right of x."""
    total = sum(len(s) for s, _ in runs) * 8 * scale
    x, y = xy
    if align == "right":
        x -= total
    cell_h = (16 if font is VGA else 8) * scale
    if box:
        ImageDraw.Draw(card).rectangle([x, y, x + total - 1, y + cell_h - 1], fill=box)
    for s, col in runs:
        m = glyphs(s, font, scale)
        if shadow:
            card.paste(Image.new("RGB", m.size, BLACK), (x + shadow, y + shadow), m)
        card.paste(Image.new("RGB", m.size, col), (x, y), m)
        x += m.width
    return x


def title(card, xy, scale, shadow=0, box=None):
    return text(card, xy, [("HAUNTED", WHITE), (".EXE", ORANGE)], VGA, scale, shadow, box)


def cursor(card, x, y, scale, col=GREY):
    """The VGA underline cursor: two rows at the bottom of the 16-row cell."""
    ImageDraw.Draw(card).rectangle([x, y + 13 * scale, x + 8 * scale - 1, y + 15 * scale - 1], fill=col)


def bevel(card, box, k):
    """The crypt's panel: black, a light outer edge top-left, darker inner edge, at k px per game pixel."""
    x0, y0, x1, y1 = box
    d = ImageDraw.Draw(card)
    d.rectangle(box, fill=BLACK)
    d.rectangle([x0, y0, x1, y0 + k - 1], fill=STONE_HI); d.rectangle([x0, y0, x0 + k - 1, y1], fill=STONE_HI)
    d.rectangle([x0, y1 - k + 1, x1, y1], fill=STONE_MID); d.rectangle([x1 - k + 1, y0, x1, y1], fill=STONE_MID)
    i = 2 * k
    d.rectangle([x0 + i, y0 + i, x1 - i, y0 + i + k - 1], fill=STONE_MID); d.rectangle([x0 + i, y0 + i, x0 + i + k - 1, y1 - i], fill=STONE_MID)
    d.rectangle([x0 + i, y1 - i - k + 1, x1 - i, y1 - i], fill=STONE_LO); d.rectangle([x1 - i - k + 1, y0 + i, x1 - i, y1 - i], fill=STONE_LO)


def flame(card, cx, cy, k, on=True):
    """The candle-flame cursor from the crypt's menus, in game pixels of size k."""
    rows = ["..o..", ".ooo.", ".oyo.", "ooyoo", "oyyyo", ".oyo."]
    d = ImageDraw.Draw(card)
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch != ".":
                col = ORANGE if ch == "o" else (255, 230, 120)
                d.rectangle([cx + (c - 2) * k, cy + (r - 3) * k, cx + (c - 2) * k + k - 1, cy + (r - 3) * k + k - 1], fill=col)


def dim(img):
    """The crypt goes dim behind a menu: every other pixel black, as dim_view() does."""
    img = img.copy(); px = img.load()
    for y in range(img.height):
        for x in range(y & 1, img.width, 2):
            px[x, y] = BLACK
    return img


# --- A: BOOT. The BIOS screen, the command, and the pumpkin's face -------------------------------
def card_boot():
    card = Image.new("RGB", (W, H), BLACK)
    text(card, (64, 56), [("CRYPTEK 486 BIOS", WHITE), (" v13.0.31", GREY)], VGA, 2)
    text(card, (64, 88), [("FPU      : ", GREY), ("Present (and watching)", WHITE)], VGA, 2)
    text(card, (64, 120), [("Memory Test : ", GREY), ("16384K OK", WHITE)], VGA, 2)
    text(card, (64, 196), [("C:\\>", GREY)], VGA, 3)
    x = title(card, (64, 244), 8)
    cursor(card, x + 8, 244, 8, WHITE)
    text(card, (64, 404), [("A HALLOWEEN SCREENSAVER FOR WINDOWS", GREY)], VGA, 2)
    text(card, (64, 436), [("WITH A CRYPT YOU CAN PLAY", GREY)], VGA, 2)
    pk = frame("pk_014.00").crop((196, 146, 524, 352))               # the jack-o'-lantern's face, 1x
    card.paste(pk, (W - 56 - pk.width, 44))
    text(card, (W - 64, 548), [("HOLD H TO ENTER THE CRYPT", ORANGE)], VGA, 2, align="right")
    return card


# --- B: CRYPT. CATACOMB full bleed at 4x --------------------------------------------------------
def card_crypt():
    card = up(frame("c_110.00").crop((20, 20, 320, 178)), 4).crop((0, 0, W, H))
    x = title(card, (52, 44), 6, shadow=6)
    text(card, (56, 160), [("HOLD H. THE CRYPT IS YOURS.", GOLD)], VGA, 2, shadow=4)
    return card


# --- C: PUMPKIN. HAUNT.EXE's patch at 2x, captions in text-mode cells ----------------------------
def card_pumpkin():
    card = up(frame("pk_014.00").crop((56, 34, 656, 349)), 2)
    title(card, (40, 32), 5, box=BLACK)
    text(card, (W - 40, 496), [("happy halloween.", CYAN)], VGA, 2, box=BLACK, align="right")
    text(card, (W - 40, 528), [("i have been carved for you.", CYAN)], VGA, 2, box=BLACK, align="right")
    return card


# --- D: SPLIT. Both scenes side by side under the name -------------------------------------------
def card_split():
    card = Image.new("RGB", (W, H), BLACK)
    left = frame("h_025.00").crop((0, 0, 596, 400))                      # the BOOGEY error, 1x text mode
    right = up(frame("c_075.00").crop((22, 0, 320, 200)), 2)             # a skeleton and a fireball, 2x
    top = 144
    card.paste(left, (0, top)); card.paste(right, (W - right.width, top))
    ImageDraw.Draw(card).rectangle([596, top - 32, W - right.width - 1, top + 399], fill=(30, 28, 36))
    title(card, (40, 20), 5)
    text(card, (W - 40, 44), [("TWO SCENES. ONE IS PLAYABLE.", GREY)], VGA, 2, align="right")
    text(card, (0, top - 32), [(" HAUNT.EXE ", BLACK)], VGA, 2, box=GREY)
    text(card, (W - right.width, top - 32), [(" CATACOMB ", BLACK)], VGA, 2, box=GOLD)
    text(card, (W - 40, 566), [("A HALLOWEEN SCREENSAVER FOR WINDOWS", GREY)], VGA, 2, align="right")
    return card


# --- E: LEAVE THE CRYPT? The crypt's own menu -----------------------------------------------------
def card_quit():
    card = up(dim(frame("c_110.00").crop((10, 26, 310, 184))), 4).crop((0, 0, W, H))
    k = 4
    bevel(card, (120, 40, 1079, 579), k)
    def centre(runs, y, font=VGA, scale=4):
        n = sum(len(s) for s, _ in runs) * 8 * scale
        return text(card, ((W - n) // 2, y), runs, font, scale)
    centre([("LEAVE THE CRYPT?", BLOOD)], 76, VGA, 4)
    centre([("OUT THERE IT IS JUST EMAIL.", (217, 217, 217))], 172, BIOS, 3)
    centre([("DOWN HERE YOU ARE A WIZARD.", (217, 217, 217))], 208, BIOS, 3)
    centre([("FLEE TO THE SURFACE", STONE)], 276, VGA, 3)
    x0 = (W - 16 * 24) // 2
    centre([("STAY IN THE DARK", WHITE)], 340, VGA, 3)
    flame(card, x0 - 36, 362, 4); flame(card, x0 + 16 * 24 + 32, 362, 4)
    centre([("SLEEPWALK", STONE)], 404, VGA, 3)
    ImageDraw.Draw(card).rectangle([180, 476, 1019, 479], fill=STONE_MID)
    centre([("HAUNTED", WHITE), (".EXE", ORANGE)], 496, VGA, 3)
    return card


# alt text for each card, written beside it for the page's og:image:alt (no quotes, |, & or backslashes)
CARDS = [
    ("a-boot", card_boot, "A black DOS boot screen: CRYPTEK 486 BIOS, FPU present (and watching), memory test OK, "
     "then the prompt typing HAUNTED.EXE in large white and orange letters beside a pixel jack-o'-lantern. "
     "Hold H to enter the crypt."),
    ("b-crypt", card_crypt, "HAUNTED.EXE over a pixel-art crypt: a fireball strikes a shield-bearing skeleton while a "
     "hooded wraith looms and the caster's hand is raised. Hold H. The crypt is yours."),
    ("c-pumpkin", card_pumpkin, "HAUNTED.EXE over a pixel-art jack-o'-lantern grinning in a pumpkin patch under a "
     "full moon, captioned happy halloween, i have been carved for you."),
    ("d-split", card_split, "HAUNTED.EXE above its two scenes: a haunted DOS prompt that cannot find soul 13, and a "
     "skeleton in a crypt of skulls facing the caster's fireball."),
    ("e-leave", card_quit, "A crypt's menu asking LEAVE THE CRYPT? Out there it is just email. Down here you are a "
     "wizard. Flee to the surface, stay in the dark, or sleepwalk. HAUNTED.EXE."),
]
for name, fn, alt in CARDS:
    img = fn()
    assert img.size == (W, H), (name, img.size)
    assert not any(c in alt for c in '"|&\\'), name
    img.save(f"{OUT}/{name}.png", optimize=True)
    with open(f"{OUT}/{name}.txt", "w") as f:
        f.write(alt + "\n")
    print(name, img.size)
